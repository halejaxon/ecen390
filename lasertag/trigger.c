#include "trigger.h"
#include "buttons.h"
#include "mio.h"
#include "transmitter.h"

#define TICK_RATE 100000
#define DEBOUNCE_MS (50 / 1000)
#define DEBOUNCE_COUNT (TICK_RATE * DEBOUNCE_MS)
#define TRIGGER_GUN_TRIGGER_MIO_PIN 10
#define GUN_TRIGGER_PRESSED 1

// Uncomment for debug prints
#define DEBUG

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
  debouncePress_st,   // Wait 50ms to see if it is a real press
  debounceRelease_st, //
  transmitter_st      // Activate the transmitter state machine
};
static enum trigger_st_t currentState;

// Static variables
static bool triggerEnable = false;
static bool ignoreGunInput = false;

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
    // printf("enter waitForHit_st\n");
    currentState = waitForHit_st;
    break;
  case waitForHit_st:
    if (triggerPressed()) {
      DPRINTF("D\n");
      // printf("enter debounce_st\n");
      currentState = debouncePress_st;
    } else {
      currentState = waitForHit_st;
    }
    break;
  case debouncePress_st:
    if ((debounceCtr > DEBOUNCE_COUNT) && (triggerPressed())) {
      debounceCtr = 0;
      transmitter_run();

      // printf("enter transmitter_st\n");
      currentState = transmitter_st;
    } else if (debounceCtr > DEBOUNCE_COUNT && !(triggerPressed())) {

      debounceCtr = 0;
      // printf("enter init_st\n");
      currentState = init_st;
    } else {
      currentState = debouncePress_st;
    }
    break;
  case transmitter_st:
    if ((debounceCtr > DEBOUNCE_COUNT) && (!triggerPressed())) {
      debounceCtr = 0;
      DPRINTF("U\n");
      currentState = waitForHit_st;
    } else {
      currentState = transmitter_st;
    }

    //   // printf("enter waitForHit_st\n");
    // if (!transmitter_running()) {
    //   DPRINTF("U\n");
    //   // printf("enter waitForHit_st\n");
    //   currentState = waitForHit_st;
    // } else {
    //   currentState = transmitter_st;
    // }
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
  case debouncePress_st:
    debounceCtr++;
    break;
  case transmitter_st:
    debounceCtr++;
    break;
  default:
    // print an error message here.
    break;
  }
}

// Runs the test continuously until BTN1 is pressed.
// The test just prints out a 'D' when the trigger or BTN0
// is pressed, and a 'U' when the trigger or BTN0 is released.
void trigger_runTest() {
  printf("starting trigger_runTest.\n");
  trigger_init();
  trigger_enable();
  buttons_init();

  while (!(buttons_read() &
           BUTTONS_BTN2_MASK)) { // Run continuously until BTN2 is pressed.
  }

  printf("exiting trigger_runTest.\n");
}