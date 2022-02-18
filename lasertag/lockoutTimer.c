#include "lockoutTimer.h"

// States for the controller state machine.
enum lockout_st_t {
  init_st, // Start here, transition out of this state on the first tick.

};
static enum lockout_st_t currentState;

void lockoutTimer_start() {
  // filler
}

// Perform any necessary inits for the lockout timer.
void lockoutTimer_init() { currentState = init_st; }

// Returns true if the timer is running.
bool lockoutTimer_running() {
  return false; // FILLLER
}

// Standard tick function.
void lockoutTimer_tick() {
  // Perform state update first.
  switch (currentState) {
  case init_st:
    break;
  default:
    // print an error message here.
    break;
  }

  // Perform state action next.
  switch (currentState) {
  case init_st:
    break;
  default:
    // print an error message here.
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
  return false;
  // FILLER
}