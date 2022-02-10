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
// #define RUNNING_MODE_M3_T2

// Uncomment to run continuous/shooter mode, Milestone 3, Task 3
// #define RUNNING_MODE_M3_T3

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
  filterTest_runTest(); // M3 T1
  // printf("buns\n");

  // printf("******** filterTest_runTest() **********\n");
  // bool success = true; // Be optimistic.
  // filter_init();       // transmitter_runTest(); // M3 T2
  // filterTest_init();
  // filterTest_fillQueue(filter_getXQueue(), 0.0);
  // filter_addNewInput(1.0);

  // for (uint32_t i = 0; i < filter_getFirCoefficientCount();
  //      i++) { // Push the single 1.0 through the queue.
  //   printf("checkpoint 1\n");
  //   double firValue = filter_firFilter(); // Run the FIR filter.
  //   printf("checkpoint 2\n");
  //   double firGoldenOutput =
  //       filter_getFirCoefficientArray()[i]; // Golden output is simply the
  //       FIR
  //                                           // coefficient.
  //   printf("checkpoint 3\n");
  //   if (!filterTest_floatingPointEqual(
  //           firValue,
  //           firGoldenOutput)) { // If the output from the FIR filter does not
  //                               // match the golden value, print an error.
  //     printf("checkpoint 4\n");
  //     success = false; // The test failed.
  //     printf("filter_runAlignmentTest: Output from FIR Filter(%20.24le) does
  //     "
  //            "not match test-data(%20.24le).\n",
  //            firValue, firGoldenOutput);
  //   }
  //   printf("checkpoint 5\n");
  //   filter_addNewInput(
  //       0.0); // Shift the 1.0 value over one position in the queue.
  // }

  // detector_runTest(); // M3 T3
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

  transmitter_runNoncontinuousTest();
  transmitter_runContinuousTest();
  trigger_runTest();
  hitLedTimer_runTest();
  lockoutTimer_runTest();
  while (1)
    ; // Forever-while loop. Modify as you see fit.
#endif

#ifdef RUNNING_MODE_M3_T3
  // The program comes up in continuous mode.
  // Hold BTN2 while the program starts to come up in shooter mode.
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
