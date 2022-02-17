#include "hitLedTimer.h "
#include <stdbool.h>

// States for the controller state machine.
enum hitLedTimer_st_t {
  init_st, // Start here, transition out of this state on the first tick.

};
static enum hitLedTimer_st_t currentState;

// Calling this starts the timer.
void hitLedTimer_start();

// Returns true if the timer is currently running.
bool hitLedTimer_running();

// Standard tick function.
void hitLedTimer_tick() {
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

// Need to init things.
void hitLedTimer_init() { currentState = init_st; }

// Turns the gun's hit-LED on.
void hitLedTimer_turnLedOn();

// Turns the gun's hit-LED off.
void hitLedTimer_turnLedOff();

// Disables the hitLedTimer.
void hitLedTimer_disable();

// Enables the hitLedTimer.
void hitLedTimer_enable();

// Runs a visual test of the hit LED.
// The test continuously blinks the hit-led on and off.
void hitLedTimer_runTest();