#include "hitLedTimer.h"
#include "buttons.h"
#include "display.h"
#include "leds.h"
#include "mio.h"
#include "switches.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// SM debugging messages
#define HL_INIT_ST_MSG "hl_init_st\n"
#define HL_WAIT_FOR_HIT_ST_MSG "hl_waitForHit_st\n"
#define HL_LIGHT_ST_MSG "hl_light_st\n"

#define ERROR_MESSAGE_HL "You fell through all the HL states\n"
#define END_TEST_MESSAGE "exiting test\n"
#define START_HL_TEST_MESSAGE "starting hitLedTimer_runTest()\n"

#define TICK_RATE 100000
#define LIGHT_TIME 500 // Time the LED should be lit in ms
#define LIGHT_COUNT                                                            \
  (LIGHT_TIME * TICK_RATE / 1000) // Length of light counter (ms * Hz / 1000)
#define HIT_LED_HIGH_VALUE 1
#define HIT_LED_LOW_VALUE 0
#define LIGHT_LED0 8 // Binary for bit pattern 1000
#define LED0_OFF 0   // All zeroes
#define HL_TEST_TICK_PERIOD_MS 10
#define TEST_TICK_RATE 100
#define TEST_LIGHT_COUNT (LIGHT_TIME * TEST_TICK_RATE / 1000)
#define LED_DELAY_TIME 300

#define BOUNCE_DELAY 5

// Static variables
volatile static bool receivedHit = false;
volatile static bool hlEnable = false;

// States for the controller state machine.
enum hitLedTimer_st_t {
  init_st,       // Start here, transition out of this state on the first tick.
  waitForHit_st, // Wait here till we are hit
  light_st,      // Keep the LED on in this state
  final_st
};
static enum hitLedTimer_st_t currentState;

// Debugging function. No parameters or return. It will print the names of the
// states each time tick() is called. It only prints states if they are
// different than the previous state.
void hlDebugStatePrint() {
  static enum hitLedTimer_st_t previousState;
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
      printf(HL_INIT_ST_MSG);
      break;
    case waitForHit_st:
      printf(HL_WAIT_FOR_HIT_ST_MSG);
      break;
    case light_st:
      printf(HL_LIGHT_ST_MSG);
      break;
    default:
      printf(ERROR_MESSAGE_HL);
      break;
    }
  }
}

// Calling this starts the timer.
void hitLedTimer_start() { receivedHit = true; }

// Returns true if the timer is currently running.
bool hitLedTimer_running() { return receivedHit; }

// Standard tick function.
void hitLedTimer_tick() {
  // Declare static variables unique to tick fct
  static uint32_t ledCtr;

  // Debugging
  //hlDebugStatePrint();

  // Perform state update first.
  switch (currentState) {
  case init_st:
    if (hlEnable) { // If SM is enabled, move to next state
      // Set counter to zero
      ledCtr = 0;

      // State update
      currentState = waitForHit_st;
    } else { // Otherwise wait till it's enabled
      // State update
      currentState = init_st;
    }
    break;
  case waitForHit_st:
    if (receivedHit) { // If we are hit, go to light state
      // Turn the led on
      hitLedTimer_turnLedOn();

      // State update
      currentState = light_st;
    } else { // Otherwise keep waiting for hit
      // State update
      currentState = waitForHit_st;
    }
    break;
  case light_st:
    // conter is still small so stay here
    if (ledCtr < LIGHT_COUNT) { // Stay in this state for 1/2 second
      // State update
      currentState = light_st;
    } //// Once it has been long enough, turn the light off and go back to
      /// waiting
    else {
      // We have finished with the current received hit
      receivedHit = false;
      ledCtr = 0;
      hitLedTimer_turnLedOff();

      // State update
      currentState = waitForHit_st;
    }
    break;
  case final_st:
    // Return to init once enable is set to false
    if (!hlEnable) {
      // State update
      currentState = init_st;
    } // Otherwise keep waiting in final state
    else {
      // State update
      currentState = final_st;
    }
    break;
  default:
    //printf(ERROR_MESSAGE_HL); // print an error message here.
    break;
  }

  // Perform state action next.
  switch (currentState) {
  case init_st:
    break;
  case waitForHit_st:
    break;
  case light_st:
    ledCtr++; // increment ledCtr
    break;
  default:
    //printf(ERROR_MESSAGE_HL); // print an error message here.
    break;
  }
}

// Need to init things.
void hitLedTimer_init() {
  currentState = init_st;
  leds_init(false);
  mio_setPinAsOutput(HIT_LED_TIMER_OUTPUT_PIN);
}

// Turns the gun's hit-LED on.
void hitLedTimer_turnLedOn() {
  mio_writePin(HIT_LED_TIMER_OUTPUT_PIN,
               HIT_LED_HIGH_VALUE); // Write a '1' to JF-3.
  leds_write(HIT_LED_HIGH_VALUE);
}

// Turns the gun's hit-LED off.
void hitLedTimer_turnLedOff() {
  mio_writePin(HIT_LED_TIMER_OUTPUT_PIN,
               HIT_LED_LOW_VALUE); // Write a '0' to JF-3.
  leds_write(HIT_LED_LOW_VALUE);   // Light up LED0
}

// Disables the hitLedTimer.
void hitLedTimer_disable() { hlEnable = false; }

// Enables the hitLedTimer.
void hitLedTimer_enable() { hlEnable = true; }

// Runs a visual test of the hit LED.
// The test continuously blinks the hit-led on and off.
void hitLedTimer_runTest() {
  // Initialize everything first
  printf(START_HL_TEST_MESSAGE);
  mio_init(false);
  buttons_init();     // Using buttons
  switches_init();    // and switches.
  hitLedTimer_init(); // init the hitLedTimer
                      // Run continuously until BTN1
  while (!(buttons_read() & BUTTONS_BTN1_MASK)) {
    // is pressed
    hitLedTimer_start();
    hitLedTimer_enable();
    // Start the transmitter.
    while (hitLedTimer_running()) { // Keep ticking until it is done.
    }
    hitLedTimer_disable();
    utils_msDelay(LED_DELAY_TIME);
  }
  // Insert a delay
  do {
    utils_msDelay(BOUNCE_DELAY);
  }
  // once the buttons gets pressed then we read our print statement.
  while (buttons_read());
  printf(END_TEST_MESSAGE);
}