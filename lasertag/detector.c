#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "detector.h"
#include "display.h"
#include "filter.h"
#include "hitLedTimer.h"
#include "interrupts.h"
#include "isr.h"
#include "lockoutTimer.h"
#include "queue.h"
#include "runningModes.h"

#define NUM_FREQUENCIES 10
#define NUM_FUDGE_FACTORS 3
#define MEDIAN 5

static uint16_t fudgeFactors[NUM_FUDGE_FACTORS] = {100, 1000, 10000};
static double sortedArray[NUM_FREQUENCIES];
static bool allIgnored;
static bool frequenciesIgnored[NUM_FREQUENCIES];
static detector_hitCount_t hitCount[NUM_FREQUENCIES];
static uint16_t filterAddCount;
static bool hitDetected;
static uint16_t fudgeFactorIndex = 0;
static uint16_t lastHitFrequency = 0;

// // Comparision function for sorting
// double detector_cmpfunc(void *a, void *b) {
//   // Comparison function
//   return (*(double *)a - *(double *)b);
// }

// void swap(double *xpointer, double *ypointer) {
//   uint16_t temp = *xpointer;
//   *xpointer = *ypointer;
//   *ypointer = temp;
// }

void selectionSort(double array[], uint8_t lengthArr) {
  uint16_t index1, index2, minIdx;

  // One by one move boundary of unsorted subarray
  for (index1 = 0; index1 < lengthArr - 1; index1++) {
    // Find the minimum element in unsorted array
    minIdx = index1;
    for (index2 = index1 + 1; index2 < lengthArr; index2++) {
      if (array[index2] < array[minIdx]) {
        minIdx = index2;
      }
    }

    // Swap the found minimum element with the first element
    // swap(&array[minIdx], &array[index1]);
    double temp = array[minIdx];
    array[minIdx] = array[index1];
    array[index1] = temp;
  }
}

// Median retriever function
double detector_getMedian() {
  // Iterate through and copy values from the get power array and assign them to
  // a new array.
  for (uint16_t i = 0; i < NUM_FREQUENCIES; i++) {
    // Stuff
    sortedArray[i] = filter_getCurrentPowerValue(i);
  }
  // qsort(sortedArray, NUM_FREQUENCIES, sizeof(double), detector_cmpfunc);
  selectionSort(sortedArray, NUM_FREQUENCIES);
  return sortedArray[MEDIAN];
}

// Always have to init things.
// bool array is indexed by frequency number, array
// location set for true to ignore, false otherwise.
// This way you can ignore multiple frequencies.
void detector_init(bool ignoredFrequencies[]) {
  // Go through and store the info about the frequencies to ignore
  for (uint16_t i = 0; i < NUM_FREQUENCIES; i++) {
    // Initialize
    frequenciesIgnored[i] = ignoredFrequencies[i];
  }
  filterAddCount = 0;
  hitDetected = false;
}

// Runs the entire detector: decimating fir-filter, iir-filters,
// power-computation, hit-detection. if interruptsNotEnabled = true,
// interrupts are not running. If interruptsNotEnabled = true you can pop
// values from the ADC queue without disabling interrupts. If
// interruptsNotEnabled = false, do the following:
// 1. disable interrupts.
// 2. pop the value from the ADC queue.
// 3. re-enable interrupts if interruptsNotEnabled was true.
// if ignoreSelf == true, ignore hits that are detected on your frequency.
// Your frequency is simply the frequency indicated by the slide switches
void detector(bool interruptsCurrentlyEnabled) {
  if (interruptsCurrentlyEnabled) {
    // Disable them first
    interrupts_disableArmInts();

    // Do stuff
    // Get the adc value
    uint32_t rawAdcValue = isr_removeDataFromAdcBuffer();
    // Map adc value to [-1.0, 1.0]
    double scaledAdcValue = detector_getScaledAdcValue(rawAdcValue);
    // Add to filter
    filter_addNewInput(scaledAdcValue);
    filterAddCount++;

    // Decimation - Only run these steps if it has been 10 adds since the last
    // time
    if (filterAddCount >= NUM_FREQUENCIES) {
      // Run FIR filter (not filter number-specific)
      filter_firFilter();
      // Run IIR and power computations for each frequency
      for (uint16_t i = 0; i < NUM_FREQUENCIES; i++) {
        // IIR
        filter_iirFilter(i);
        // Power
        filter_computePower(i, true,
                            false); // No need to compute from scratch or debug
      }
      // double powerVals[10];
      // filter_getCurrentPowerValues(powerVals);
      // for (int i = 0; i < 10; i++) {
      //   printf("value: %f\n", powerVals[i]);
      // }
      // Now check to see if a hit was detected (at a frequency we care about)
      if (!lockoutTimer_running() && detector_hitDetected() &&
          !(frequenciesIgnored[detector_getFrequencyNumberOfLastHit()])) {
        // Start the lockout timer and hitLed timer
        lockoutTimer_start();
        hitLedTimer_start();
        // Increment the hit count
        hitCount[detector_getFrequencyNumberOfLastHit()] =
            hitCount[detector_getFrequencyNumberOfLastHit()] + 1;
        // Raise the flag
        hitDetected = true;
      }
    }

    // Re-enable
    interrupts_enableArmInts();
  } else {
    // Proceed without needing to disable
    // Get the adc value
    uint32_t rawAdcValue = isr_removeDataFromAdcBuffer();
    // Map adc value to [-1.0, 1.0]
    double scaledAdcValue = 2 * (double)rawAdcValue / 4095 - 1.0;
    // Add to filter
    filter_addNewInput(scaledAdcValue);
    filterAddCount++;

    // Decimation - Only run these steps if it has been 10 adds since the last
    // time
    if (filterAddCount >= NUM_FREQUENCIES) {
      // Run FIR filter (not filter number-specific)
      filter_firFilter();
      // Run IIR and power computations for each frequency
      for (uint16_t i = 0; i < NUM_FREQUENCIES; i++) {
        // IIR
        filter_iirFilter(i);
        // Power
        filter_computePower(i, false,
                            false); // No need to compute from scratch or debug
      }
      // Now check to see if a hit was detected (at a frequency we care about)
      if (!lockoutTimer_running() && detector_hitDetected() &&
          !(frequenciesIgnored[detector_getFrequencyNumberOfLastHit()])) {
        // Start the lockout timer and hitLed timer
        lockoutTimer_start();
        hitLedTimer_start();
        // Increment the hit count
        hitCount[detector_getFrequencyNumberOfLastHit()] =
            hitCount[detector_getFrequencyNumberOfLastHit()] + 1;
        // Raise the flag
        hitDetected = true;
      }
    }
  }
}

bool hitDetector() {
  // Sort the power values to find the median
  double median = detector_getMedian();

  // Find index of max power value
  double max = filter_getCurrentPowerValue(0);
  uint16_t maxIndex = 0;
  for (uint16_t i = 0; i < NUM_FREQUENCIES; i++) {
    if (max < filter_getCurrentPowerValue(i)) {
      max = filter_getCurrentPowerValue(i);
      maxIndex = i;
    }
  }

  // printf("median: %le\n", median);
  // printf("max power: %le\n", max);
  // printf("fudgeFactors[fudgeFactorIndex] * median: %le\n",
  //        fudgeFactors[fudgeFactorIndex] * median);

  // See if max power value > fudgefactor*median
  if (max > (fudgeFactors[fudgeFactorIndex] * median)) {
    // If yes, return true AND set frequency number of last hit
    lastHitFrequency = maxIndex;
    return true;
  } else {
    return false;
  }
}

// Returns true if a hit was detected.
bool detector_hitDetected() {
  // First run our hitDetector function
  hitDetected = hitDetector();
  return hitDetected;
}

// Returns the frequency number that caused the hit.
uint16_t detector_getFrequencyNumberOfLastHit() {
  // Filler
  uint16_t filler;
  return filler;
}

// Clear the detected hit once you have accounted for it.
void detector_clearHit() {
  // are we clearing all hits? or Just one????
  hitDetected = false;
}

// Ignore all hits. Used to provide some limited invincibility in some game
// modes. The detector will ignore all hits if the flag is true, otherwise
// will respond to hits normally.
void detector_ignoreAllHits(bool flagValue) { allIgnored = flagValue; }

// Get the current hit counts.
// Copy the current hit counts into the user-provided hitArray
// using a for-loop.
void detector_getHitCounts(detector_hitCount_t hitArray[]) {
  for (uint16_t i = 0; i < NUM_FREQUENCIES; i++) {
    hitArray[i] = hitCount[i];
  }
}

// Allows the fudge-factor index to be set externally from the detector.
// The actual values for fudge-factors is stored in an array found in
// detector.c
void detector_setFudgeFactorIndex(uint32_t factor) { // Filler
  fudgeFactorIndex = factor;
}

// This function sorts the inputs in the unsortedArray and
// copies the sorted results into the sortedArray. It also
// finds the maximum power value and assigns the frequency
// number for that value to the maxPowerFreqNo argument.
// This function also ignores a single frequency as noted below.
// if ignoreFrequency is true, you must ignore any power from frequencyNumber.
// maxPowerFreqNo is the frequency number with the highest value contained in
// the unsortedValues. unsortedValues contains the unsorted values.
// sortedValues contains the sorted values. Note: it is assumed that the size
// of both of the array arguments is 10.
detector_status_t detector_sort(uint32_t *maxPowerFreqNo,
                                double unsortedValues[],
                                double sortedValues[]) {
  // Filler
  detector_status_t filler;
  return filler;
}

// Encapsulate ADC scaling for easier testing.
double detector_getScaledAdcValue(isr_AdcValue_t adcValue) {
  return 2 * (double)adcValue / 4095 - 1.0;
}

/*******************************************************
 ****************** Test Routines **********************
 ******************************************************/

// Students implement this as part of Milestone 3, Task 3.
void detector_runTest() {
  // filler
}

// Returns 0 if passes, non-zero otherwise.
// if printTestMessages is true, print out detailed status messages.
// detector_status_t detector_testSort(sortTestFunctionPtr testSortFunction,
// bool printTestMessages);

detector_status_t detector_testAdcScaling() {
  // fill
}