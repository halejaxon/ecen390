
#include "queue.h"
#include "filter.h"
#include <stdint.h>


// Must call this prior to using any filter functions.
void filter_init(){

}

// Use this to copy an input into the input queue of the FIR-filter (xQueue).
void filter_addNewInput(double x){

}

// Fills a queue with the given fillValue. For example,
// if the queue is of size 10, and the fillValue = 1.0,
// after executing this function, the queue will contain 10 values
// all of them 1.0.
void filter_fillQueue(queue_t *q, double fillValue){

}

// Invokes the FIR-filter. Input is contents of xQueue.
// Output is returned and is also pushed on to yQueue.
double filter_firFilter(){

}

// Use this to invoke a single iir filter. Input comes from yQueue.
// Output is returned and is also pushed onto zQueue[filterNumber].
double filter_iirFilter(uint16_t filterNumber){

}

// Use this to compute the power for values contained in an outputQueue.
// If force == true, then recompute power by using all values in the
// outputQueue. This option is necessary so that you can correctly compute power
// values the first time. After that, you can incrementally compute power values
// by:
// 1. Keeping track of the power computed in a previous run, call this
// prev-power.
// 2. Keeping track of the oldest outputQueue value used in a previous run, call
// this oldest-value.
// 3. Get the newest value from the power queue, call this newest-value.
// 4. Compute new power as: prev-power - (oldest-value * oldest-value) +
// (newest-value * newest-value). Note that this function will probably need an
// array to keep track of these values for each of the 10 output queues.
double filter_computePower(uint16_t filterNumber, bool forceComputeFromScratch,
                           bool debugPrint){

                           }

// Returns the last-computed output power value for the IIR filter
// [filterNumber].
double filter_getCurrentPowerValue(uint16_t filterNumber){

}

// Get a copy of the current power values.
// This function copies the already computed values into a previously-declared
// array so that they can be accessed from outside the filter software by the
// detector. Remember that when you pass an array into a C function, changes to
// the array within that function are reflected in the returned array.
void filter_getCurrentPowerValues(double powerValues[]){

}

// Using the previously-computed power values that are current stored in
// currentPowerValue[] array, Copy these values into the normalizedArray[]
// argument and then normalize them by dividing all of the values in
// normalizedArray by the maximum power value contained in currentPowerValue[].
void filter_getNormalizedPowerValues(double normalizedArray[],
                                     uint16_t *indexOfMaxValue){

                                     }

/*********************************************************************************************************
********************************** Verification-assisting functions.
**************************************
********* Test functions access the internal data structures of the filter.c via
*these functions. ********
*********************** These functions are not used by the main filter
*functions. ***********************
**********************************************************************************************************/

// Returns the array of FIR coefficients.
const double *filter_getFirCoefficientArray(){

}

// Returns the number of FIR coefficients.
uint32_t filter_getFirCoefficientCount(){

}

// Returns the array of coefficients for a particular filter number.
const double *filter_getIirACoefficientArray(uint16_t filterNumber){

}

// Returns the number of A coefficients.
uint32_t filter_getIirACoefficientCount(){

}

// Returns the array of coefficients for a particular filter number.
const double *filter_getIirBCoefficientArray(uint16_t filterNumber){

}

// Returns the number of B coefficients.
uint32_t filter_getIirBCoefficientCount(){

}

// Returns the size of the yQueue.
uint32_t filter_getYQueueSize(){

}

// Returns the decimation value.
uint16_t filter_getDecimationValue(){

}

// Returns the address of xQueue.
queue_t *filter_getXQueue(){

}

// Returns the address of yQueue.
queue_t *filter_getYQueue(){

}

// Returns the address of zQueue for a specific filter number.
queue_t *filter_getZQueue(uint16_t filterNumber){

}

// Returns the address of the IIR output-queue for a specific filter-number.
queue_t *filter_getIirOutputQueue(uint16_t filterNumber){

}

// void filter_runTest();

#endif /* FILTER_H_ */