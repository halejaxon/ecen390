#include "trigger.h"
#include "buttons.h"
#include "mio.h"
#include "transmitter.h"

#define TICK_RATE 10000
#define DEBOUNCE_MS (50 / 1000)
#define DEBOUNCE_COUNT (TICK_RATE * DEBOUNCE_MS)
#define TRIGGER_GUN_TRIGGER_MIO_PIN 10
#define GUN_TRIGGER_PRESSED 1

// States for the controller state machine.
enum trigger_st_t {
  init_st,       // Start here, transition out of this state on the first tick.
  waitForHit_st, // Check if the trigger input is high
  debounce_st,   // Wait 50ms to see if it is a real press
  transmitter_st // Activate the transmitter state machine
};
static enum trigger_st_t currentState;

// Static variables
static bool triggerEnable = false;
static bool ignoreGunInput = false;

bool triggerPressed() {
  return false; // Filler
}

// Determines whether the trigger switch of the gun is connected (see
// discussion in lab web pages). Initializes the mio subsystem.
// Configure the trigger-pin as an input.
// Ignore the gun if the trigger is high at init (means that it is not
// connected).
void trigger_init() {
  mio_setPinAsInput(TRIGGER_GUN_TRIGGER_MIO_PIN);
  // If the trigger is pressed when trigger_init() is called, assume that the
  // gun is not connected and ignore it.
  if (triggerPressed()) {
    ignoreGunInput = true;
  }
}

// Enable the trigger state machine. The trigger state-machine is inactive
// until this function is called. This allows you to ignore the trigger when
// helpful (mostly useful for testing).
void trigger_enable() { triggerEnable = true; }

// Disable the trigger state machine so that trigger presses are ignored.
void trigger_disable() { triggerEnable = false; }

// Returns the number of remaining shots.
trigger_shotsRemaining_t trigger_getRemainingShotCount();

// Sets the number of remaining shots.
void trigger_setRemainingShotCount(trigger_shotsRemaining_t count);

// Standard tick function.
void trigger_tick() {
  // Declare static variables unique to tick fct
  static uint32_t debounceCtr;

  // Perform state update first.
  switch (currentState) {
  case init_st:
    currentState = waitForHit_st;
    break;
  case waitForHit_st:
    if (triggerPressed()) {
      currentState = debounce_st;
    } else {
      currentState = waitForHit_st;
    }
    break;
  case debounce_st:
    if ((debounceCtr > DEBOUNCE_COUNT) && (triggerPressed())) {
      debounceCtr = 0;
      transmitter_run();
      currentState = transmitter_st;
    } else if (debounceCtr > DEBOUNCE_COUNT && !(triggerPressed())) {
      debounceCtr = 0;
      currentState = init_st;
    } else {
      currentState = debounce_st;
    }
    break;
  case transmitter_st:
    if (!transmitter_running()) {
      currentState = waitForHit_st;
    } else {
      currentState = transmitter_st;
    }
    break;
  default:
    // print an error message here.
    break;
  }

  // Perform state action next.
  switch (currentState) {
  case init_st:
    break;
  case waitForHit_st:
    break;
  case debounce_st:
    debounceCtr++;
    break;
  case transmitter_st:

    break;
  default:
    // print an error message here.
    break;
  }
}

// Runs the test continuously until BTN1 is pressed.
// The test just prints out a 'D' when the trigger or BTN0
// is pressed, and a 'U' when the trigger or BTN0 is released.
void trigger_runTest() {}