#include "transmitter.h"
#include "buttons.h"
#include "display.h"
#include "mio.h"
#include "switches.h"
#include "utils.h"
#include <stdio.h>

// SM debugging messages
#define TX_INIT_ST_MSG "tx_init_st\n"
#define TX_TRANSMIT_ST_MSG "tx_transmit_st\n"
#define TX_CONT_TRANSMIT_ST_MSG "tx_contTransmit_st\n"

#define ERROR_MESSAGE "You fell through all the TX states\n"

#define TRANSMIT_TIME 20000
#define TICK_RATE 100000
#define HALF 2
#define TRANSMITTER_OUTPUT_PIN 13
#define TRANSMITTER_HIGH_VALUE 1
#define TRANSMITTER_LOW_VALUE 0
#define FILTER_FREQUENCY_COUNT 10
#define TRANSMITTER_TEST_TICK_PERIOD_IN_MS 10
#define BOUNCE_DELAY 5

// Static variables
static uint16_t txFrequencyNumber;
static bool isRunning = false;
static bool isContinuous;
static bool pinInput;
static uint16_t frequencies[FILTER_FREQUENCY_COUNT] = {
    1471, 1724, 2000, 2273, 2632, 2941, 3333, 3571, 3846, 4167};

// The transmitter state machine generates a square wave output at the
// chosen
// frequency as set by transmitter_setFrequencyNumber(). The step counts for
// the frequencies are provided in filter.h

// States for the controller state machine.
enum transmitter_st_t {
  init_st, // Start here, transition out of this state on the first tick.

  transmit_st,     // Transmits frequency
  contTransmit_st, // transmits continuousfrequency
};
static enum transmitter_st_t currentState;

// Debugging function. No parameters or return. It will print the names of the
// states each time tick() is called. It only prints states if they are
// different than the previous state.
void txDebugStatePrint() {
  static enum transmitter_st_t previousState;
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
      printf(TX_INIT_ST_MSG);
      break;
    case transmit_st:
      printf(TX_TRANSMIT_ST_MSG);
      break;
    case contTransmit_st:
      printf(TX_CONT_TRANSMIT_ST_MSG);
      break;
    }
  }
}

uint16_t getPulseWidth(uint16_t frequencyNumber) {
  return TICK_RATE / (HALF * frequencies[frequencyNumber]);
}

void transmitter_set_jf1_to_one() {
  mio_writePin(TRANSMITTER_OUTPUT_PIN,
               TRANSMITTER_HIGH_VALUE); // Write a '1' to JF-1.
}

void transmitter_set_jf1_to_zero() {
  mio_writePin(TRANSMITTER_OUTPUT_PIN,
               TRANSMITTER_LOW_VALUE); // Write a '1' to JF-1.
}

// Standard init function.
void transmitter_init() {
  currentState = init_st;
  mio_init(false); // false disables any debug printing if there is a system
                   // failure during init.
  mio_setPinAsOutput(TRANSMITTER_OUTPUT_PIN); // Configure the signal direction
                                              // of the pin to be an output.
}

// Starts the transmitter.
void transmitter_run() { isRunning = true; }

// Returns true if the transmitter is still running.
bool transmitter_running() { return isRunning; }

// Sets the frequency number. If this function is called while the
// transmitter is running, the frequency will not be updated until the
// transmitter stops and transmitter_run() is called again.
void transmitter_setFrequencyNumber(uint16_t frequencyNumber) {
  txFrequencyNumber = frequencyNumber;
}

// Returns the current frequency setting.
uint16_t transmitter_getFrequencyNumber() { return txFrequencyNumber; }

// Standard tick function.
void transmitter_tick() {
  // Declare static variables unique to tick fct
  static uint32_t transCtr, pulseCtr;
  static uint16_t currFrequencyNumber;

  // Debugging
  txDebugStatePrint();

  // Perform state update first.
  switch (currentState) {
  case init_st:
    // Set counter to zero
    transCtr = 0;
    pinInput = false;

    if (isContinuous && isRunning) {
      pinInput = !pinInput;

      if (pinInput) {
        transmitter_set_jf1_to_one();
      } else {
        transmitter_set_jf1_to_zero();
      }

      // State update
      currentState = contTransmit_st;
    } else if (isRunning) {
      // Run the tx
      pinInput = !pinInput;

      if (pinInput) {
        transmitter_set_jf1_to_one();
      } else {
        transmitter_set_jf1_to_zero();
      }
      currFrequencyNumber = txFrequencyNumber;

      // State update
      currentState = transmit_st;
    } else { // No change
      // State update
      currentState = init_st;
    }
    break;
  case transmit_st:
    if (transCtr > TRANSMIT_TIME) {
      // Set counter to zero
      transCtr = 0;
      pinInput = false;

      // Lower running flag
      isRunning = false;

      // State update
      currentState = init_st;
    } else if (pulseCtr > getPulseWidth(currFrequencyNumber)) {
      // Run the tx
      pinInput = !pinInput;

      if (pinInput) {
        transmitter_set_jf1_to_one();
      } else {
        transmitter_set_jf1_to_zero();
      }

      // State update
      currentState = transmit_st;
    }
    break;
  case contTransmit_st:
    if (transCtr > TRANSMIT_TIME) {
      // Set counter to zero
      transCtr = 0;
      pinInput = false;

      // Lower running flag
      isRunning = false;

      // State update
      currentState = init_st;
    } else if (pulseCtr > getPulseWidth(txFrequencyNumber)) {
      // Run the tx
      pinInput = !pinInput;

      if (pinInput) {
        transmitter_set_jf1_to_one();
      } else {
        transmitter_set_jf1_to_zero();
      }

      // State update
      currentState = transmit_st;
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
  case transmit_st:
    transCtr++;
    break;
  case contTransmit_st:
    break;
  default:
    // print an error message here.
    break;
  }
}

// Tests the transmitter.
void transmitter_runTest() {
  printf("starting transmitter_runTest()\n");
  mio_init(false);
  buttons_init();     // Using buttons
  switches_init();    // and switches.
  transmitter_init(); // init the transmitter.
  // transmitter_enableTestMode(); // Prints diagnostics to stdio.
  // printf("check 4");
  while (!(buttons_read() &
           BUTTONS_BTN1_MASK)) { // Run continuously until BTN1 is pressed.
    // printf("check 5");
    uint16_t switchValue =
        switches_read() %
        FILTER_FREQUENCY_COUNT; // Compute a safe number from the switches.
    transmitter_setFrequencyNumber(
        switchValue); // set the frequency number based upon switch value.
    // printf("check 1");
    transmitter_run();
    // Start the transmitter.
    // printf("check 2");
    while (transmitter_running()) { // Keep ticking until it is done.
      // printf("check 3");
      transmitter_tick(); // tick.
      utils_msDelay(
          TRANSMITTER_TEST_TICK_PERIOD_IN_MS); // short delay between ticks.
    }
    printf("completed one test period.\n");
  }
  // transmitter_disableTestMode();
  do {
    utils_msDelay(BOUNCE_DELAY);
  } while (buttons_read());
  printf("exiting transmitter_runTest()\n");
}

// Runs the transmitter continuously.
// if continuousModeFlag == true, transmitter runs continuously, otherwise,
// transmits one pulse-width and stops. To set continuous mode, you must
// invoke this function prior to calling transmitter_run(). If the
// transmitter is in currently in continuous mode, it will stop running if
// this function is invoked with continuousModeFlag == false. It can stop
// immediately or wait until the last 200 ms pulse is complete. NOTE: while
// running continuously, the transmitter will change frequencies at the end
// of each 200 ms pulse.
void transmitter_setContinuousMode(bool continuousModeFlag) {
  isContinuous = continuousModeFlag;
}

// Tests the transmitter in non-continuous mode.
// The test runs until BTN1 is pressed.
// To perform the test, connect the oscilloscope probe
// to the transmitter and ground probes on the development board
// prior to running this test. You should see about a 300 ms dead
// spot between 200 ms pulses.
// Should change frequency in response to the slide switches.
void transmitter_runNoncontinuousTest() {
  // Filler
}

// Tests the transmitter in continuous mode.
// To perform the test, connect the oscilloscope probe
// to the transmitter and ground probes on the development board
// prior to running this test.
// Transmitter should continuously generate the proper waveform
// at the transmitter-probe pin and change frequencies
// in response to changes to the changes in the slide switches.
// Test runs until BTN1 is pressed.
void transmitter_runContinuousTest() {
  // Filler
}
