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
#define DECIMATION_FACTOR 10
#define NUM_FUDGE_FACTORS 5
#define MEDIAN 5
#define NOT_TEST false
#define ADC_RANGE_MAX 4095
#define TWICE 2
#define ONE_DOUBLE 1.0
#define TEST_MAX_LOCATION 4
#define TEST_FUDGE_INDEX 1
#define TEST_ARRAY_VALS                                                        \
  0.12, 0.15, 0.14, 0.30, 700, 0.22, 0.21, 0.19, 0.14, 0.15

static const uint16_t fudgeFactors[NUM_FUDGE_FACTORS] = {25, 60, 125, 250,
                                                         500};
static double testArray[NUM_FREQUENCIES] = {TEST_ARRAY_VALS};
static double sortedArray[NUM_FREQUENCIES];
static bool allIgnored;
static bool frequenciesIgnored[NUM_FREQUENCIES];
static detector_hitCount_t hitCount[NUM_FREQUENCIES];
static uint16_t filterAddCount;
static bool hitDetected;
static uint16_t fudgeFactorIndex = TEST_FUDGE_INDEX;
static uint16_t lastHitFrequency = 0;

// Sorting function - takes in an array and a length and changes the original
// array to be sorted
void selectionSort(double array[], uint8_t lengthArr) {
  // Initialize indices to use
  uint16_t index1, index2, minIdx;
  // One by one move boundary of unsorted subarray
  for (index1 = 0; index1 < lengthArr - 1; index1++) {
    // Find the minimum element in unsorted array
    minIdx = index1;
    // Sort the elements in the subarray
    for (index2 = index1 + 1; index2 < lengthArr; index2++) {
      // Find the new minimum index
      if (array[index2] < array[minIdx]) {
        minIdx = index2; // Set to be new min
      }
    }

    // Swap the found minimum element with the first element
    double tempElement = array[minIdx];
    array[minIdx] = array[index1];
    array[index1] = tempElement;
  }
}

// Median retriever function - pass in the power array you want to find the
// median of and it will return a new
double detector_getMedian(double powerArr[]) {
  // Iterate through and copy values from the get power array and assign them to
  // a new array.
  for (uint16_t i = 0; i < NUM_FREQUENCIES; i++) {
    // Copy
    sortedArray[i] = powerArr[i];
  }
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

  // Enable hitLedTimer
  hitLedTimer_enable();
}
// returns true if the detector has recieved a hit... false otherwise.
bool hitDetector(bool testMode) {
  // Sort the power values to find the median
  double powerArr[NUM_FREQUENCIES];

  // Check if currently testing or actually running
  if (testMode) {
    // If yes, copy test data
    for (uint16_t i = 0; i < NUM_FREQUENCIES; i++) {
      powerArr[i] = testArray[i];
    }
  } // If not testing, copy actual power values
  else {
    filter_getCurrentPowerValues(powerArr); // Real power values
  }

  double median =
      detector_getMedian(powerArr); // Pass them into the get median function
  double max = powerArr[0];
  uint16_t maxIndex = 0;
  // Iterate through and get the max values
  for (uint16_t i = 0; i < NUM_FREQUENCIES; i++) {
    // if bigger than the max then assign it as the new max
    if (max < powerArr[i]) {
      max = powerArr[i];
      maxIndex = i;
    }
  }

  // See if max power value > fudgefactor*median
  if (max > (fudgeFactors[fudgeFactorIndex] * median)) {
    // If yes, return true AND set frequency number of last hit
    lastHitFrequency = maxIndex;
    return true;
  } // return false otherwise
  else {
    return false;
  }
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
  uint32_t elementCount = isr_adcBufferElementCount();
  // iterate through all of the elements  and collect data from the ADC
  for (uint32_t i = 0; i < elementCount; i++) {
    uint32_t rawAdcValue = 0;
    // Only disable is the interrupts are enabled
    if (interruptsCurrentlyEnabled) {
      // Disable them first
      interrupts_disableArmInts();

      // Do stuff
      // Get the adc value
      rawAdcValue = isr_removeDataFromAdcBuffer();

      // Re-enable
      interrupts_enableArmInts();
    } // Get the adc value without needing to disable interrupts
    else {
      rawAdcValue = isr_removeDataFromAdcBuffer();
    }

    // Map adc value to [-1.0, 1.0]
    double scaledAdcValue = detector_getScaledAdcValue(rawAdcValue);
    // Add to filter
    filter_addNewInput(scaledAdcValue);
    filterAddCount++;

    // Decimation - Only run these steps if it has been 10 adds since the last
    // time
    if (filterAddCount >= DECIMATION_FACTOR) {
      // Reset
      filterAddCount = 0;
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
      if (!lockoutTimer_running() && hitDetector(NOT_TEST) &&
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

// Returns true if a hit was detected.
bool detector_hitDetected() {
  // First run our hitDetector function
  return hitDetected;
}

// Returns the frequency number that caused the hit.
uint16_t detector_getFrequencyNumberOfLastHit() {
  // Get value from global var
  return lastHitFrequency;
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
  // Copy counts into hitArray
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
  detector_status_t filler;
  return filler;
}

// Encapsulate ADC scaling for easier testing.
double detector_getScaledAdcValue(isr_AdcValue_t adcValue) {
  return TWICE * (double)adcValue / ADC_RANGE_MAX - ONE_DOUBLE;
}

/*******************************************************
 ****************** Test Routines **********************
 ******************************************************/

// Students implement this as part of Milestone 3, Task 3.
void detector_runTest() {
  uint16_t fakeFudgeFactorInd = TEST_FUDGE_INDEX;
  detector_setFudgeFactorIndex(fakeFudgeFactorInd);
  // Init
  // Don't ignore any frequencies
  bool noIgnore[NUM_FREQUENCIES] = {false, false, false, false, false,
                                    false, false, false, false, false};
  detector_init(noIgnore);
  bool testing = true; // Set testing to true
  // Run detector
  printf("Expected: Hit detected = 1 (aka true) at frequency 4.\n Actual: Hit "
         "detected = %d at frequency %d\n",
         hitDetector(testing), detector_getFrequencyNumberOfLastHit());
  // Make it so a hit should not be detected
  testArray[TEST_MAX_LOCATION] = 1;
  // hitDetector(testing);
  printf(
      "Expected: Hit detected = 0 (aka false).\n Actual: Hit detected = %d\n",
      hitDetector(testing));
}

// Returns 0 if passes, non-zero otherwise.
// if printTestMessages is true, print out detailed status messages.
// detector_status_t detector_testSort(sortTestFunctionPtr testSortFunction,
// bool printTestMessages);
detector_status_t detector_testAdcScaling() {
  // fill
}