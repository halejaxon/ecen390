/*
This software is provided for student assignment use in the Department of
Electrical and Computer Engineering, Brigham Young University, Utah, USA.
Users agree to not re-host, or redistribute the software, in source or binary
form, to other persons or other institutions. Users may modify and use the
source code for personal or educational use.
For questions, contact Brad Hutchings or Jeff Goeders, https://ece.byu.edu/
*/

#include "interrupts.h"
#include "runningModes.h"
#include "livesCounter.h"
#include "buttons.h"
#include "detector.h"
#include "display.h"
#include "filter.h"
#include "histogram.h"
#include "hitLedTimer.h"
#include "intervalTimer.h"
#include "isr.h"
#include "ledTimer.h"
#include "leds.h"
#include "lockoutTimer.h"
#include "mio.h"
#include "queue.h"
#include "sound.h"
#include "switches.h"
#include "transmitter.h"
#include "trigger.h"
#include "utils.h"
#include "xparameters.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

/*
This file (runningModes2.c) is separated from runningModes.c so that
check_and_zip.py does not include provided code for grading. Code for
submission can be added to this file and will be graded. The code in
runningModes.c can give you some ideas about how to implement other
modes here.
*/

#define TEAM_A_FREQ 6
#define TEAM_B_FREQ 9
#define BIT_MASK_SW0 0x1
#define CLIP_SIZE 10

// Uncomment this code so that the code in the various modes will
// ignore your own frequency. You still must properly implement
// the ability to ignore frequencies in detector.c
#define IGNORE_OWN_FREQUENCY 1

#define MAX_HIT_COUNT 100000

#define MAX_BUFFER_SIZE 100 // Used for a generic message buffer.

#define DETECTOR_HIT_ARRAY_SIZE                                                \
  FILTER_FREQUENCY_COUNT // The array contains one location per user frequency.

#define HISTOGRAM_BAR_COUNT                                                    \
  FILTER_FREQUENCY_COUNT // As many histogram bars as user filter frequencies.

#define ISR_CUMULATIVE_TIMER INTERVAL_TIMER_TIMER_0 // Used by the ISR.
#define TOTAL_RUNTIME_TIMER                                                    \
  INTERVAL_TIMER_TIMER_1 // Used to compute total run-time.
#define MAIN_CUMULATIVE_TIMER                                                  \
  INTERVAL_TIMER_TIMER_2 // Used to compute cumulative run-time in main.

#define SYSTEM_TICKS_PER_HISTOGRAM_UPDATE                                      \
  30000 // Update the histogram about 3 times per second.

#define RUNNING_MODE_WARNING_TEXT_SIZE 2 // Upsize the text for visibility.
#define RUNNING_MODE_WARNING_TEXT_COLOR DISPLAY_RED // Red for more visibility.
#define RUNNING_MODE_NORMAL_TEXT_SIZE 1 // Normal size for reporting.
#define RUNNING_MODE_NORMAL_TEXT_COLOR DISPLAY_WHITE // White for reporting.
#define RUNNING_MODE_SCREEN_X_ORIGIN 0 // Origin for reporting text.
#define RUNNING_MODE_SCREEN_Y_ORIGIN 0 // Origin for reporting text.

// Detector should be invoked this often for good performance.
#define SUGGESTED_DETECTOR_INVOCATIONS_PER_SECOND 30000
// ADC queue should have no more than this number of unprocessed elements for
// good performance.
#define SUGGESTED_REMAINING_ELEMENT_COUNT 500

// Defined to make things more readable.
#define INTERRUPTS_CURRENTLY_ENABLED true
#define INTERRUPTS_CURRENTLY_DISABLE false

/* example play sound
sound_stopSound();
sound_playSound(/choose sound from sound.h/);
}
*/

void runningModes_twoTeams() {
  uint16_t hitCount = 0;
  runningModes_initAll();

  // Play start sound
  sound_stopSound();
  sound_playSound(sound_gameStart_e);

  // Init the ignored-frequencies so we ignore all but the other team's
  // frequency
  bool ignoredFrequencies[FILTER_FREQUENCY_COUNT];

  // First set it to ignore all frequencies
  for (uint16_t i = 0; i < FILTER_FREQUENCY_COUNT; i++) {
    ignoredFrequencies[i] = true;
  }

  // Read the switches to see which team we are on
  uint16_t switchSetting = switches_read() & BIT_MASK_SW0;
  // Team B = 1, Team A = 0
  if (switchSetting) { // If switch is 1, then you are part of team B. This
                       // means you should NOT ignore Team A's frequency
    ignoredFrequencies[TEAM_A_FREQ] = false;
  } else { // If switch is 0, then you are part of team A. This means you should
           // NOT ignore Team B's frequency
    ignoredFrequencies[TEAM_B_FREQ] = false;
  }

  // Init detector and hit info
  detector_init(ignoredFrequencies);
  uint16_t hitCount = 0;
  detectorInvocationCount = 0; // Keep track of detector invocations.
  trigger_enable();         // Makes the trigger state machine responsive to the
                            // trigger.
  interrupts_initAll(true); // Inits all interrupts but does not enable them.
  interrupts_enableTimerGlobalInts(); // Allows the timer to generate
                                      // interrupts.
  interrupts_startArmPrivateTimer();  // Start the private ARM timer running.
  uint16_t histogramSystemTicks =
      0; // Only update the histogram display every so many ticks.

  intervalTimer_reset(
      ISR_CUMULATIVE_TIMER); // Used to measure ISR execution time.
  intervalTimer_reset(
      TOTAL_RUNTIME_TIMER); // Used to measure total program execution time.
  intervalTimer_reset(
      MAIN_CUMULATIVE_TIMER); // Used to measure main-loop execution time.
  intervalTimer_start(
      TOTAL_RUNTIME_TIMER);   // Start measuring total execution time.
  interrupts_enableArmInts(); // The ARM will start seeing interrupts after
                              // this.
  lockoutTimer_start();       // Ignore erroneous hits at startup (when all power
  // values are essentially 0).

  // Begin keeping track of lives and ammo
  lives_init();
  ammoHandler_init();

  // uint16_t playerLives = NUM_LIVES;
  // uint16_t clipCount = CLIP_SIZE;

  // Game loop
  while ((!(buttons_read() & BUTTONS_BTN3_MASK)) &&
        hitCount < MAX_HIT_COUNT) { // Run until you detect btn3 pressed.

    transmitter_setFrequencyNumber(
        runningModes_getFrequencySetting());    // Read the switches and switch
                                                // frequency as required.
    intervalTimer_start(MAIN_CUMULATIVE_TIMER); // Measure run-time when you are
                                                // doing something.
    histogramSystemTicks++; // Keep track of ticks so you know when to update
                            // the histogram.
    // Run filters, compute power, run hit-detection.
    detectorInvocationCount++;              // Used for run-time statistics.
    detector(INTERRUPTS_CURRENTLY_ENABLED); // Interrupts are currently enabled.
    // Every time we have run the detector, check if it detected a hit
    if (detector_hitDetected()) {           // Hit detected
      hitCount++;                           // increment the hit count.
      lives_hit(); // Tell the Lives SM that it has received a hit
      detector_clearHit();                  // Clear the hit.
      detector_hitCount_t
          hitCounts[DETECTOR_HIT_ARRAY_SIZE]; // Store the hit-counts here.
      detector_getHitCounts(hitCounts);       // Get the current hit counts.
      histogram_plotUserHits(hitCounts);      // Plot the hit counts on the TFT.
    }
    intervalTimer_stop(MAIN_CUMULATIVE_TIMER); // All done with actual processing.
  }

  // Implement game loop...

  interrupts_disableArmInts(); // Done with game loop, disable the interrupts.
  hitLedTimer_turnLedOff();    // Save power :-)
  runningModes_printRunTimeStatistics(); // Print the run-time statistics to the
                                        // TFT.
  printf("Two-team mode terminated after detecting %d shots.\n", hitCount);
}
