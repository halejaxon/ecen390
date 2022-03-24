#include "isr.h"
#include "hitLedTimer.h"
#include "interrupts.h"
#include "lockoutTimer.h"
#include "transmitter.h"
#include "trigger.h"
#include "livesCounter.h"
#include "ammoHandler.h"
#include <stdio.h>

// isr provides the isr_function() where you will place functions that require
// accurate timing. A buffer for storing values from the Analog to Digital
// Converter (ADC) is implemented in isr.c Values are added to this buffer by
// the code in isr.c. Values are removed from this queue by code in detector.c

#define ADC_BUFFER_SIZE 100000
#define INIT_VAL 0

// This implements a dedicated circular buffer for storing values
// from the ADC until they are read and processed by detector().
// adcBuffer_t is similar to a queue.
typedef struct {
  uint32_t indexIn;               // New values go here.
  uint32_t indexOut;              // Pull old values from here.
  uint32_t elementCount;          // Number of elements in the buffer.
  uint32_t data[ADC_BUFFER_SIZE]; // Values are stored here.
} adcBuffer_t;

// This is the instantiation of adcBuffer.
volatile static adcBuffer_t adcBuffer;

// Init adcBuffer.
void adcBufferInit() {
  // Set all element at indexOut to be zero
  adcBuffer.data[adcBuffer.indexOut] = INIT_VAL;
  // Set initial index values AND COUNT
  adcBuffer.indexIn = INIT_VAL;
  adcBuffer.indexOut = INIT_VAL;
  adcBuffer.elementCount = INIT_VAL;
}

// Performs inits for anything in isr.c
void isr_init() {
  adcBufferInit();    // Init the local adcBuffer.
  transmitter_init(); // Call state machine init functions
  lockoutTimer_init();
  hitLedTimer_init();
  trigger_init();
  sound_init();
  lives_init();
  ammoHandler_init();
}

// This function is invoked by the timer interrupt at 100 kHz.
void isr_function() {
  // Get ADC data once per tick
  isr_addDataToAdcBuffer(interrupts_getAdcData());

  // Tick all of the functions
  transmitter_tick();
  lockoutTimer_tick();
  hitLedTimer_tick();
  trigger_tick();
  sound_tick();
  lives_tick();
  ammoHandler_tick();
}

// This adds data to the ADC queue. Data are removed from this queue and used by
// the detector.
void isr_addDataToAdcBuffer(uint32_t adcData) {
  // Check if the buffer is full (i.e., if indexIn + 1 == indexOut)
  if (adcBuffer.elementCount >= ADC_BUFFER_SIZE) {
    // If full, we have to pop before we can push
    isr_removeDataFromAdcBuffer();
    // Now add an element
    adcBuffer.data[adcBuffer.indexIn] = adcData;
    // Adding a value means incrementing indexIn
    if (adcBuffer.indexIn <
        ADC_BUFFER_SIZE - 1) { // Most times this is straightforward
      adcBuffer.indexIn++;
    } // But if indexIn is pointing to the last element in the buffer,
      // we need to wrap back around to zero
    else {
      adcBuffer.indexIn = 0;
    }
    adcBuffer.elementCount++;
  } // If not full, just add an element
  else {
    adcBuffer.data[adcBuffer.indexIn] = adcData;
    // Adding a value means incrementing indexIn
    if (adcBuffer.indexIn <
        ADC_BUFFER_SIZE - 1) { // Most times this is straightforward
      adcBuffer.indexIn++;
    } // But if indexIn is pointing to the last element in the buffer,
      // we need to wrap back around to zero
    else {
      adcBuffer.indexIn = 0;
    }
    adcBuffer.elementCount++;
  }
}

// This removes a value from the ADC buffer.
uint32_t isr_removeDataFromAdcBuffer() {
  // Check if the buffer is empty
  if (adcBuffer.elementCount > 0) {
    // First get the value we will return
    uint32_t dataVal = adcBuffer.data[adcBuffer.indexOut];

    adcBuffer.indexOut++;
    // set the indexout to zero its greater than the buffer size
    if (adcBuffer.indexOut >= ADC_BUFFER_SIZE) {
      adcBuffer.indexOut = 0;
    }

    // And decrementing element count
    adcBuffer.elementCount--;

    // Finally, return the value
    return dataVal;
  } // Do nothing if queue is empty
  else {
    return 0;
  }
}

// This returns the number of values in the ADC buffer.
uint32_t isr_adcBufferElementCount() { return adcBuffer.elementCount; }