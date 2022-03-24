#include "trigger.h"
#include "buttons.h"
#include "mio.h"
#include "transmitter.h"
#include "ammoHandler.h"
#include <stdio.h>
#include "sound.h"

#define START_TRIGGER_TEST_MESSAGE "Starting Trigger Test\n"
#define END_TEST_MESSAGE "exiting test\n"
#define ERROR_MESSAGE_TRIGGER "ERROR\n"
#define TICK_RATE 100000
#define DEBOUNCE_MS (50 / 1000)
#define DEBOUNCE_COUNT (TICK_RATE * DEBOUNCE_MS)
#define TRIGGER_GUN_TRIGGER_MIO_PIN 10
#define GUN_TRIGGER_PRESSED 1
#define TRIGGER_PUSHED_MESSAGE "D\n"
#define TRIGGER_RELEASED_MESSAGE "U\n"
#define INIT_SHOTS 10

// Uncomment for debug prints
//#define DEBUG

#if defined(DEBUG)
#include "xil_printf.h"
#include <stdio.h>
#define DPRINTF(...) printf(__VA_ARGS__)
#define DPCHAR(ch) outbyte(ch)
#else
#define DPRINTF(...)
#define DPCHAR(ch)
#endif

// States for the controller state machine.
enum trigger_st_t {
  init_st,       // Start here, transition out of this state on the first tick.
  waitForHit_st, // Check if the trigger input is high
  debouncePress_st, // Wait 50ms to see if it is a real press
  transmitter_st    // Activate the transmitter state machine
};
static enum trigger_st_t currentState;

// Static variables
static bool triggerEnable = false;
static bool ignoreGunInput = false;

static trigger_shotsRemaining_t shotsRemaining;

// This function returns a true is the button is pressed
bool triggerPressed() {
  // Read from JF-2 and from BTN0
  return (buttons_read() & BUTTONS_BTN0_MASK) ||
         mio_readPin(TRIGGER_GUN_TRIGGER_MIO_PIN);
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
  shotsRemaining = INIT_SHOTS;
}

// Enable the trigger state machine. The trigger state-machine is inactive
// until this function is called. This allows you to ignore the trigger when
// helpful (mostly useful for testing).
void trigger_enable() { triggerEnable = true; }

// Disable the trigger state machine so that trigger presses are ignored.
void trigger_disable() { triggerEnable = false; }

// Returns the number of remaining shots.
trigger_shotsRemaining_t trigger_getRemainingShotCount() {
  return shotsRemaining;
}

// Sets the number of remaining shots.
void trigger_setRemainingShotCount(trigger_shotsRemaining_t count) {
  shotsRemaining = count;
}

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
    // if you sense a push then move onto the debounce state
    if (triggerPressed()) {
      DPRINTF(TRIGGER_PUSHED_MESSAGE);
      currentState = debouncePress_st;
    } // move onto the wait state if not
    else {
      currentState = waitForHit_st;
    }
    break;
  case debouncePress_st:
    // if the counter is big and the trigger is preesed still then run the
    // transmitter
    if ((debounceCtr > DEBOUNCE_COUNT) && (triggerPressed())) {
      debounceCtr = 0;

      // Only shoot if we actually have ammo
      if (trigger_getRemainingshotCount() > 0) {
        transmitter_run();
        shotsRemaining--; // We have used up one of our shots
      }
      
      // Tell the ammoHandler that the trigger was pulled
      ammoHandlerRun();
      currentState = transmitter_st;
    } // move onto the init state if the counter is big but it was a false alarm
    else if (debounceCtr > DEBOUNCE_COUNT && !(triggerPressed())) {
      debounceCtr = 0;
      currentState = init_st;
    } // Stay here if not
    else {
      currentState = debouncePress_st;
    }
    break;
  case transmitter_st:
    // debounce the release of the trigger
    if ((debounceCtr > DEBOUNCE_COUNT) && (!triggerPressed())) {
      debounceCtr = 0;
      DPRINTF(TRIGGER_RELEASED_MESSAGE);
      ammoHandler_stop(); // Tell the ammohandler when the trigger is released
      currentState = waitForHit_st;
    } /// stay in this transmitter state
    else {
      currentState = transmitter_st;
    }

    break;
  default:
    printf(ERROR_MESSAGE_TRIGGER); // print an error message here.
    break;
  }

  // Perform state action next.
  switch (currentState) {
  case init_st:
    break;
  case waitForHit_st:
    break;
  case debouncePress_st:
    debounceCtr++;
    break;
  case transmitter_st:
    debounceCtr++;
    break;
  default:
    printf(ERROR_MESSAGE_TRIGGER); // print an error message here.
    break;
  }
}

// Runs the test continuously until BTN1 is pressed.
// The test just prints out a 'D' when the trigger or BTN0
// is pressed, and a 'U' when the trigger or BTN0 is released.
void trigger_runTest() {
  printf(START_TRIGGER_TEST_MESSAGE);
  trigger_init();
  trigger_enable();
  buttons_init();
  // Stay her until button is pressed
  while (!(buttons_read() &
           BUTTONS_BTN2_MASK)) { // Run continuously until BTN2 is pressed.
  }

  printf(END_TEST_MESSAGE);
}