#include "lockoutTimer.h"
#include "buttons.h"
#include "display.h"
#include "intervalTimer.h"
#include "leds.h"
#include "mio.h"
#include "switches.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// SM debugging messages
#define LO_INIT_ST_MSG "lo_init_st\n"
#define LO_WAIT_ST_MSG "lo_wait_st\n"
#define LO_COUNTER_ST_MSG "lo_counter_st\n"

#define ERROR_MESSAGE_LO "You fell through all the LO states\n"
#define END_TEST_MESSAGE "exiting test\n"
#define REPORT_DURATION "Duration: %f\n"
#define START_LOCKOUT_TEST_MESSAGE "starting lockOutTimer_runTest()\n"

#define TICK_RATE 100000
#define LOCKOUT_TIME 500 // Time the LED should be lit in ms
#define LOCKOUT_COUNT                                                          \
  (LOCKOUT_TIME * TICK_RATE /                                                  \
   1000) // Length of lockout counter (ms * Hz / 1000)
#define LOCKOUT_TEST_TICK_PERIOD_MS 10
#define TEST_TICK_RATE 100
#define TEST_COUNT (LOCKOUT_TIME * TEST_TICK_RATE / 1000)

#define BOUNCE_DELAY 5

// Static variables
volatile static bool counting = false;

// States for the controller state machine.
enum lockout_st_t {
  init_st,   // Start here, transition out of this state on the first tick.
  wait_st,   // Wait here till we are hit
  counter_st // Keep the LED on in this state
};
static enum lockout_st_t currentState;

// Debugging function. No parameters or return. It will print the names of the
// states each time tick() is called. It only prints states if they are
// different than the previous state.
void loDebugStatePrint() {
  static enum lockout_st_t previousState;
  static bool firstPass = true;

  // Only print the message if:
  // 1. This the first pass and the value for previousState is unknown.
  // 2. previousState != currentState - this prevents reprinting the same state
  // name over and over.
  if (previousState != currentState || firstPass) {
    firstPass = false; // previousState will be defined, firstPass is false.
    previousState =
        currentState;       // keep track of the last state that you were in.
    switch (currentState) { // This prints messages based upon the state that
                            // you were in.
    case init_st:
      printf(LO_INIT_ST_MSG);
      break;
    case wait_st:
      printf(LO_WAIT_ST_MSG);
      break;
    case counter_st:
      printf(LO_COUNTER_ST_MSG);
      break;
    default:
      printf(ERROR_MESSAGE_LO);
      break;
    }
  }
}

// Sets the counting flag to be true.
void lockoutTimer_start() { counting = true; }

// Perform any necessary inits for the lockout timer.
void lockoutTimer_init() { currentState = init_st; }

// Returns true if the timer is running.
bool lockoutTimer_running() { return counting; }

// Standard tick function.
void lockoutTimer_tick() {
  // Declare static variables unique to tick fct
  static uint32_t lockoutCtr;

  // Perform state update first.
  switch (currentState) {
  case init_st:
    // counting has to be true in order to get to the counter state
    if (counting) {
      // Set counter to zero
      lockoutCtr = 0;

      // State update
      currentState = counter_st;
    }
    // if not then stay in this state
    else {
      // State update
      currentState = init_st;
    }

    break;
  case counter_st:
    // Stay here till the counter is exceeded
    if (lockoutCtr < LOCKOUT_COUNT) {
      // State update
      currentState = counter_st;
    } // Go back and wait before counting again
    else {

      // Set counting to false
      counting = false;

      // State update
      currentState = init_st;
    }
    break;
  default:
    //printf(ERROR_MESSAGE_LO);
    break;
  }

  // Perform state action next.
  switch (currentState) {
  case init_st:

    break;
  case counter_st:
    lockoutCtr++; // Increment lockoutCtr
    break;
  default:
    break;
  }
}

// Test function assumes interrupts have been completely enabled and
// lockoutTimer_tick() function is invoked by isr_function().
// Prints out pass/fail status and other info to console.
// Returns true if passes, false otherwise.
// This test uses the interval timer to determine correct delay for
// the interval timer.
bool lockoutTimer_runTest() {
  printf(START_LOCKOUT_TEST_MESSAGE);
  mio_init(false);
  buttons_init();                              // Using buttons
  switches_init();                             // and switches.
  lockoutTimer_init();                         // init the hitLedTimer
  intervalTimer_init(INTERVAL_TIMER_TIMER_1);  // Init timer 0.
  intervalTimer_reset(INTERVAL_TIMER_TIMER_1); // Reset timer 0.

  // Start recording the time
  intervalTimer_start(INTERVAL_TIMER_TIMER_1);
  lockoutTimer_start();
  // Start the transmitter.
  while (lockoutTimer_running()) { // Keep ticking until it is done.
  }
  intervalTimer_stop(INTERVAL_TIMER_TIMER_1);

  // Report the duration
  printf(REPORT_DURATION,
         intervalTimer_getTotalDurationInSeconds(INTERVAL_TIMER_TIMER_1));
  printf(END_TEST_MESSAGE);

  return false;
}