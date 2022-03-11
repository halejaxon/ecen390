/*
This software is provided for student assignment use in the Department of
Electrical and Computer Engineering, Brigham Young University, Utah, USA.
Users agree to not re-host, or redistribute the software, in source or binary
form, to other persons or other institutions. Users may modify and use the
source code for personal or educational use.
For questions, contact Brad Hutchings or Jeff Goeders, https://ece.byu.edu/
*/

// Uncomment to run tests, various Milestones
#define RUNNING_MODE_TESTS

// Uncomment to run Milestone 3, Task 2
//#define RUNNING_MODE_M3_T2

// Uncomment to run continuous/shooter mode, Milestone 3, Task 3
#define RUNNING_MODE_M3_T3

// Uncomment to run two-player mode, Milestone 5
// #define RUNNING_MODE_M5

// The following line enables the main() contained in laserTagMain.c
// Leave this line uncommented unless you want to run some other special test
// main().
#define LASER_TAG_MAIN

#ifdef LASER_TAG_MAIN

#include <assert.h>
#include <stdio.h>

#include "buttons.h"
#include "detector.h"
#include "filter.h"
#include "filterTest.h"
#include "hitLedTimer.h"
#include "interrupts.h"
#include "isr.h"
#include "lockoutTimer.h"
#include "runningModes.h"
#include "sound.h"
#include "switches.h"
#include "transmitter.h"
#include "trigger.h"

#define TEN_SECONDS 10000
#define TWO_SECONDS 2000
#define FOUR_SECONDS 4000
#define PRINT_INFO_MESSAGES true
#define DONT_PRINT_INFO_MESSAGES false
#define PLOT_INPUT false
#define TEST_IIR_FILTER_NUMBER 0

int main() {

#ifdef RUNNING_MODE_TESTS
  // queue_runTest();      // M1
  // filterTest_runTest(); // M3 T1
  detector_runTest(); // M3 T3
// sound_runTest(); // M4
#endif

#ifdef RUNNING_MODE_M3_T2
  buttons_init();
  switches_init();
  isr_init();

  interrupts_initAll(true);           // main interrupt init function.
  interrupts_enableTimerGlobalInts(); // enable global interrupts.
  interrupts_startArmPrivateTimer();  // start the main timer.
  interrupts_enableArmInts(); // now the ARM processor can see interrupts.

  // transmitter_runTest();
  transmitter_runNoncontinuousTest();
  transmitter_runContinuousTest();
  trigger_runTest();
  hitLedTimer_runTest();
  lockoutTimer_runTest();
  while (1)
    ;
    // Forever-while loop. Modify as you see fit.
#endif

#ifdef RUNNING_MODE_M3_T3
  // ADC testing

  // isr_init();

  // isr_addDataToAdcBuffer(11);
  // isr_addDataToAdcBuffer(12);
  // isr_addDataToAdcBuffer(13);
  // isr_addDataToAdcBuffer(14);

  // printf("Num elements: %d\n", isr_adcBufferElementCount());
  // printf("Popped element: %d\n", isr_removeDataFromAdcBuffer());
  // printf("Popped element: %d\n", isr_removeDataFromAdcBuffer());
  // printf("Popped element: %d\n", isr_removeDataFromAdcBuffer());
  // printf("Popped element: %d\n", isr_removeDataFromAdcBuffer());
  // printf("Popped element: %d\n", isr_removeDataFromAdcBuffer());
  // printf("Popped element: %d\n", isr_removeDataFromAdcBuffer());
  // printf("Num elements: %d\n", isr_adcBufferElementCount());

  // End test
  // The program comes up in continuous mode.
  // Hold BTN2 while the program starts to come up in shooter mode.

  // void selectionSort(double array[], uint8_t lengthArr) {
  //   uint16_t index1, index2, minIdx;

  //   // One by one move boundary of unsorted subarray
  //   for (index1 = 0; index1 < lengthArr - 1; index1++) {
  //     // Find the minimum element in unsorted array
  //     minIdx = index1;
  //     for (index2 = index1 + 1; index2 < lengthArr; index2++) {
  //       if (array[index2] < array[minIdx]) {
  //         minIdx = index2;
  //       }
  //     }

  //     // Swap the found minimum element with the first element
  //     // swap(&array[minIdx], &array[index1]);
  //     double temp = array[minIdx];
  //     array[minIdx] = array[index1];
  //     array[index1] = temp;
  //   }
  // }

  // // // sde
  // double arr[6] = {2.2, 1.6, 1.1, 3.3, 5.5, 4.4};

  // // /* Function to print an array */
  // // void printArray(double arr[], double size) {
  // //   int i;
  // //   for (i = 0; i < size; i++)
  // //     printf(" %e\n", arr[i]);
  // // }

  // double max = arr[0];
  // uint16_t maxIndex = 0;
  // for (uint16_t i = 0; i < 6; i++) {
  //   if (max < arr[i]) {
  //     max = arr[i];
  //     maxIndex = i;
  //   }
  // }

  // printf("max: %le\n", max);

  // int n = 6;
  // selectionSort(arr, n);
  // printArray(arr, n);

  // printf("%le\n", arr[3]);

  buttons_init(); // Init the buttons.
  if (buttons_read() &
      BUTTONS_BTN2_MASK) { // Read the buttons to see if BTN2 is depressed.
    printf("Starting shooter mode\n");
    runningModes_shooter(); // Run shooter mode if BTN2 is depressed.
  } else {
    printf("Starting continuous mode\n");
    runningModes_continuous(); // Otherwise, go to continuous mode.
  }
#endif

#ifdef RUNNING_MODE_M5
  printf("Starting two team mode\n");
  runningModes_twoTeams();
#endif

  return 0;
}

#endif // LASER_TAG_MAIN
