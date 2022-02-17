#include "filter.h"
#include <stdlib.h>

#define IIR_A_COEFFICIENT_COUNT 10 // IS THIS SUPPOSED TO BE 10??
#define IIR_B_COEFFICIENT_COUNT 11
#define FILTER_IIR_FILTER_COUNT 10
#define QUEUE_INIT_VALUE 0.0
#define Y_QUEUE_SIZE IIR_B_COEFFICIENT_COUNT
#define Z_QUEUE_SIZE IIR_A_COEFFICIENT_COUNT
#define OUTPUT_QUEUE_SIZE 2000
#define FIR_FILTER_TAP_COUNT 81
#define X_QUEUE_SIZE FIR_FILTER_TAP_COUNT
#define POWER_QUEUE_SIZE OUTPUT_QUEUE_SIZE
#define NUM_POW_QUEUES 10
#define POW_ARRAY_SIZE 10
#define DECIMATION_VALUE 10

// Static variables
static queue_t xQueue;
static queue_t yQueue;
static queue_t zQueueArr[FILTER_IIR_FILTER_COUNT];
static queue_t outputQueueArr[FILTER_IIR_FILTER_COUNT];
static queue_t powerOutputArr[NUM_POW_QUEUES];
static double currentPowerValue[POW_ARRAY_SIZE];
static double oldestValues[FILTER_IIR_FILTER_COUNT] = {0, 0, 0, 0, 0,
                                                       0, 0, 0, 0, 0};

// B coefficients (length 81)
const static double firCoefficients[FIR_FILTER_TAP_COUNT] = {
    5.3751585173668532e-04,  4.1057821244099187e-04,  2.3029811615433415e-04,
    -1.0022421255268634e-05, -3.1239220025873498e-04, -6.6675539469892989e-04,
    -1.0447674325821804e-03, -1.3967861519587053e-03, -1.6537303925483089e-03,
    -1.7346382015025667e-03, -1.5596484449522630e-03, -1.0669170920384048e-03,
    -2.3088291222664008e-04, 9.2146384892950099e-04,  2.2999023467067978e-03,
    3.7491095163012366e-03,  5.0578112742500408e-03,  5.9796411037508039e-03,
    6.2645305736409576e-03,  5.6975395969924630e-03,  4.1403678436423832e-03,
    1.5696670848600943e-03,  -1.8940691237627923e-03, -5.9726960144609528e-03,
    -1.0235869785695425e-02, -1.4127694707478473e-02, -1.7013744396319821e-02,
    -1.8244737113263923e-02, -1.7230507462687290e-02, -1.3515937014932726e-02,
    -6.8495092580338254e-03, 2.7646568253820039e-03,  1.5039019355364032e-02,
    2.9404269200373489e-02,  4.5042275861018187e-02,  6.0948410493762414e-02,
    7.6017645500231018e-02,  8.9145705550443280e-02,  9.9334411853457705e-02,
    1.0578952596388017e-01,  1.0800000000000000e-01,  1.0578952596388017e-01,
    9.9334411853457705e-02,  8.9145705550443280e-02,  7.6017645500231018e-02,
    6.0948410493762414e-02,  4.5042275861018187e-02,  2.9404269200373489e-02,
    1.5039019355364032e-02,  2.7646568253820039e-03,  -6.8495092580338254e-03,
    -1.3515937014932726e-02, -1.7230507462687290e-02, -1.8244737113263923e-02,
    -1.7013744396319821e-02, -1.4127694707478473e-02, -1.0235869785695425e-02,
    -5.9726960144609528e-03, -1.8940691237627923e-03, 1.5696670848600943e-03,
    4.1403678436423832e-03,  5.6975395969924630e-03,  6.2645305736409576e-03,
    5.9796411037508039e-03,  5.0578112742500408e-03,  3.7491095163012366e-03,
    2.2999023467067978e-03,  9.2146384892950099e-04,  -2.3088291222664008e-04,
    -1.0669170920384048e-03, -1.5596484449522630e-03, -1.7346382015025667e-03,
    -1.6537303925483089e-03, -1.3967861519587053e-03, -1.0447674325821804e-03,
    -6.6675539469892989e-04, -3.1239220025873498e-04, -1.0022421255268634e-05,
    2.3029811615433415e-04,  4.1057821244099187e-04,  5.3751585173668532e-04};

// IIR filter A coefficients
const static double iirACoefficientConstants
    [FILTER_FREQUENCY_COUNT][IIR_A_COEFFICIENT_COUNT] = {
        {-5.9637727070164033e+00, 1.9125339333078266e+01,
         -4.0341474540744230e+01, 6.1537466875368942e+01,
         -7.0019717951472359e+01, 6.0298814235239050e+01,
         -3.8733792862566432e+01, 1.7993533279581129e+01,
         -5.4979061224867900e+00, 9.0332828533800036e-01},
        {-4.6377947119071443e+00, 1.3502215749461573e+01,
         -2.6155952405269751e+01, 3.8589668330738334e+01,
         -4.3038990303252618e+01, 3.7812927599537112e+01,
         -2.5113598088113768e+01, 1.2703182701888078e+01,
         -4.2755083391143458e+00, 9.0332828533800102e-01},
        {-3.0591317915750929e+00, 8.6417489609637510e+00,
         -1.4278790253808840e+01, 2.1302268283304301e+01,
         -2.2193853972079225e+01, 2.0873499791105438e+01,
         -1.3709764520609394e+01, 8.1303553577931709e+00,
         -2.8201643879900526e+00, 9.0332828533800114e-01},
        {-1.4071749185996758e+00, 5.6904141470697542e+00,
         -5.7374718273676324e+00, 1.1958028362868898e+01,
         -8.5435280598354613e+00, 1.1717345583835954e+01,
         -5.5088290876998611e+00, 5.3536787286077576e+00,
         -1.2972519209655577e+00, 9.0332828533799836e-01},
        {8.2010906117760429e-01, 5.1673756579268675e+00, 3.2580350909221010e+00,
         1.0392903763919218e+01, 4.8101776408669252e+00, 1.0183724507092546e+01,
         3.1282000712126901e+00, 4.8615933365572221e+00, 7.5604535083145363e-01,
         9.0332828533800569e-01},
        {2.7080869856154477e+00, 7.8319071217995528e+00, 1.2201607990980708e+01,
         1.8651500443681559e+01, 1.8758157568004471e+01, 1.8276088095998947e+01,
         1.1715361303018838e+01, 7.3684394621253126e+00, 2.4965418284511749e+00,
         9.0332828533799858e-01},
        {4.9479835250075901e+00, 1.4691607003177602e+01, 2.9082414772101060e+01,
         4.3179839108869331e+01, 4.8440791644688872e+01, 4.2310703962394328e+01,
         2.7923434247706425e+01, 1.3822186510471004e+01, 4.5614664160654339e+00,
         9.0332828533799869e-01},
        {6.1701893352279811e+00, 2.0127225876810304e+01, 4.2974193398071584e+01,
         6.5958045321253252e+01, 7.5230437667866312e+01, 6.4630411355739554e+01,
         4.1261591079243900e+01, 1.8936128791950424e+01, 5.6881982915179901e+00,
         9.0332828533799114e-01},
        {7.4092912870072389e+00, 2.6857944460290138e+01, 6.1578787811202268e+01,
         9.8258255839887383e+01, 1.1359460153696310e+02, 9.6280452143026224e+01,
         5.9124742025776499e+01, 2.5268527576524267e+01, 6.8305064480743294e+00,
         9.0332828533800336e-01},
        {8.5743055776347745e+00, 3.4306584753117924e+01, 8.4035290411037209e+01,
         1.3928510844056848e+02, 1.6305115418161665e+02, 1.3648147221895832e+02,
         8.0686288623300072e+01, 3.2276361903872257e+01, 7.9045143816245078e+00,
         9.0332828533800080e-01}};

// IIR Filter B coefficients
const static double
    iirBCoefficientConstants[FILTER_FREQUENCY_COUNT][IIR_B_COEFFICIENT_COUNT] =
        {{9.0928661148190964e-10, 0.0000000000000000e+00,
          -4.5464330574095478e-09, 0.0000000000000000e+00,
          9.0928661148190956e-09, 0.0000000000000000e+00,
          -9.0928661148190956e-09, 0.0000000000000000e+00,
          4.5464330574095478e-09, 0.0000000000000000e+00,
          -9.0928661148190964e-10},
         {9.0928661148193953e-10, 0.0000000000000000e+00,
          -4.5464330574096975e-09, 0.0000000000000000e+00,
          9.0928661148193951e-09, 0.0000000000000000e+00,
          -9.0928661148193951e-09, 0.0000000000000000e+00,
          4.5464330574096975e-09, 0.0000000000000000e+00,
          -9.0928661148193953e-10},
         {9.0928661148192567e-10, 0.0000000000000000e+00,
          -4.5464330574096280e-09, 0.0000000000000000e+00,
          9.0928661148192561e-09, 0.0000000000000000e+00,
          -9.0928661148192561e-09, 0.0000000000000000e+00,
          4.5464330574096280e-09, 0.0000000000000000e+00,
          -9.0928661148192567e-10},
         {9.0928661148204282e-10, 0.0000000000000000e+00,
          -4.5464330574102145e-09, 0.0000000000000000e+00,
          9.0928661148204290e-09, 0.0000000000000000e+00,
          -9.0928661148204290e-09, 0.0000000000000000e+00,
          4.5464330574102145e-09, 0.0000000000000000e+00,
          -9.0928661148204282e-10},
         {9.0928661148189796e-10, 0.0000000000000000e+00,
          -4.5464330574094899e-09, 0.0000000000000000e+00,
          9.0928661148189798e-09, 0.0000000000000000e+00,
          -9.0928661148189798e-09, 0.0000000000000000e+00,
          4.5464330574094899e-09, 0.0000000000000000e+00,
          -9.0928661148189796e-10},
         {9.0928661148203186e-10, 0.0000000000000000e+00,
          -4.5464330574101591e-09, 0.0000000000000000e+00,
          9.0928661148203182e-09, 0.0000000000000000e+00,
          -9.0928661148203182e-09, 0.0000000000000000e+00,
          4.5464330574101591e-09, 0.0000000000000000e+00,
          -9.0928661148203186e-10},
         {9.0928661148195907e-10, 0.0000000000000000e+00,
          -4.5464330574097951e-09, 0.0000000000000000e+00,
          9.0928661148195903e-09, 0.0000000000000000e+00,
          -9.0928661148195903e-09, 0.0000000000000000e+00,
          4.5464330574097951e-09, 0.0000000000000000e+00,
          -9.0928661148195907e-10},
         {9.0928661148207932e-10, 0.0000000000000000e+00,
          -4.5464330574103965e-09, 0.0000000000000000e+00,
          9.0928661148207930e-09, 0.0000000000000000e+00,
          -9.0928661148207930e-09, 0.0000000000000000e+00,
          4.5464330574103965e-09, 0.0000000000000000e+00,
          -9.0928661148207932e-10},
         {9.0928661148198357e-10, 0.0000000000000000e+00,
          -4.5464330574099176e-09, 0.0000000000000000e+00,
          9.0928661148198351e-09, 0.0000000000000000e+00,
          -9.0928661148198351e-09, 0.0000000000000000e+00,
          4.5464330574099176e-09, 0.0000000000000000e+00,
          -9.0928661148198357e-10},
         {9.0928661148194956e-10, 0.0000000000000000e+00,
          -4.5464330574097480e-09, 0.0000000000000000e+00,
          9.0928661148194960e-09, 0.0000000000000000e+00,
          -9.0928661148194960e-09, 0.0000000000000000e+00,
          4.5464330574097480e-09, 0.0000000000000000e+00,
          -9.0928661148194956e-10}};
//--------------------------------MY ADDED
// FUNCTIONS------------------------

// Function to initialize each queue in the array of z queues
void initZQueues() {
  // For each queue in our z queue array
  for (uint32_t i = 0; i < FILTER_IIR_FILTER_COUNT; i++) {
    queue_init(&(zQueueArr[i]), Z_QUEUE_SIZE, "z");
    // Interating through z queue and pushing values
    for (uint32_t j = 0; j < Z_QUEUE_SIZE; j++) {
      queue_overwritePush(&(zQueueArr[i]), QUEUE_INIT_VALUE);
    }
  }
}

// Function to initialize Output queues
void initOutputQueues() {
  // For each queue in our output array
  for (uint32_t i = 0; i < FILTER_IIR_FILTER_COUNT; i++) {
    queue_init(&(outputQueueArr[i]), OUTPUT_QUEUE_SIZE, "output");
    // Interating through output queue and pushing values
    for (uint32_t j = 0; j < OUTPUT_QUEUE_SIZE; j++) {
      queue_overwritePush(&(outputQueueArr[i]), QUEUE_INIT_VALUE);
    }
  }
}

// Function to initialize x queue
void initXQueue() {
  queue_init(&(xQueue), X_QUEUE_SIZE, "x queue");
  // Iterate thorough x quene to push values
  for (uint32_t j = 0; j < X_QUEUE_SIZE; j++) {
    queue_overwritePush(&(xQueue), QUEUE_INIT_VALUE);
  }
}

// Function to initialize y queue
void initYQueue() {
  queue_init(&(yQueue), Y_QUEUE_SIZE, "y queue");
  // Iterate thorough y quene to push values
  for (uint32_t i = 0; i < Y_QUEUE_SIZE; i++) {
    queue_overwritePush(&(yQueue), QUEUE_INIT_VALUE);
  }
}
//------------------------------END MY FUNCTIONS------------------------
// Must call this prior to using any filter
// functions.
void filter_init() {
  // Init queues and fill them with 0s.
  initXQueue();       // Call queue_init() on xQueue and fill it with zeros.
  initYQueue();       // Call queue_init() on yQueue and fill it with zeros.
  initZQueues();      // Call queue_init() on all of the zQueues and fill each z
                      // queue with zeros.
  initOutputQueues(); // Call queue_init() all of the outputQueues and fill each
                      // outputQueue with zeros.
}

// Use this to copy an input into the input queue of the FIR-filter (xQueue).
void filter_addNewInput(double x) { queue_overwritePush(&xQueue, x); }

// Fills a queue with the given fillValue. For example,
// if the queue is of size 10, and the fillValue = 1.0,
// after executing this function, the queue will contain 10 values
// all of them 1.0.
void filter_fillQueue(queue_t *q, double fillValue) {
  // iterate through q and pushing value that is passed to the function
  for (uint32_t j = 0; j < q->size; j++) {
    queue_overwritePush(q, fillValue);
  }
}

// Invokes the FIR-filter. Input is contents of xQueue.
// Output is returned and is also pushed on to yQueue.
double filter_firFilter() {
  double y = 0;
  // Use for loop to make a sum of products of elements and FIR coefficients
  for (uint32_t i = 0; i < FIR_FILTER_TAP_COUNT; i++) {
    y += queue_readElementAt(&xQueue, FIR_FILTER_TAP_COUNT - 1 - i) *
         firCoefficients[i]; // iteratively adds the (firCoefficients * xQueue)
                             // products.
  }
  queue_overwritePush(&(yQueue), y);
  return y;
}

// Use this to invoke a single iir filter. Input comes from yQueue.
// Output is returned and is also pushed onto zQueue[filterNumber].
// filterNumber is the index of the corresponding channel
double filter_iirFilter(uint16_t filterNumber) {
  double b_sum = 0;
  double a_sum = 0;
  // To give B_sum a value of corresponding product of y queue and B coefficient
  for (uint32_t i = 0; i < IIR_B_COEFFICIENT_COUNT; i++) {
    b_sum += queue_readElementAt(&(yQueue), IIR_B_COEFFICIENT_COUNT - 1 - i) *
             iirBCoefficientConstants[filterNumber][i]; // iteratively adds the
                                                        // (firCoefficients *
                                                        // xQueue) products.
  }
  // To give A_sum a value of corresponding product of z queue and A coefficient
  for (uint32_t i = 0; i < IIR_A_COEFFICIENT_COUNT; i++) {
    a_sum += queue_readElementAt(&(zQueueArr[filterNumber]),
                                 IIR_A_COEFFICIENT_COUNT - 1 - i) *
             iirACoefficientConstants[filterNumber][i];
  }
  //"difference" is the value of the difference between the two sums
  double difference = b_sum - a_sum;
  queue_overwritePush(&(outputQueueArr[filterNumber]), difference);
  queue_overwritePush(&(zQueueArr[filterNumber]), difference);
  return difference;
}

// Use this to compute the power for values contained in an outputQueue.
// If force == true, then recompute power by using all values in the
// outputQueue. This option is necessary so that you can correctly compute
// power values the first time. After that, you can incrementally compute
// power values by:
// 1. Keeping track of the power computed in a previous run, call this
// prev-power.
// 2. Keeping track of the oldest outputQueue value used in a previous run,
// call this oldest-value.
// 3. Get the newest value from the power queue, call this newest-value.
// 4. Compute new power as: prev-power - (oldest-value * oldest-value) +
// (newest-value * newest-value). Note that this function will probably need
// an array to keep track of these values for each of the 10 output queues.
// filterNumber is the index of the corresponding channel
double filter_computePower(uint16_t filterNumber, bool forceComputeFromScratch,
                           bool debugPrint) {
  double prev_power = currentPowerValue[filterNumber];
  double oldest_value = oldestValues[filterNumber];
  double newest_value = queue_readElementAt(
      &(outputQueueArr[filterNumber]), outputQueueArr[filterNumber].size - 1);

  // if force is true then we will compute the power.
  if (forceComputeFromScratch) {
    currentPowerValue[filterNumber] = 0;
    // summing the power values.
    for (int16_t j = 0; j < POWER_QUEUE_SIZE; j++) {
      currentPowerValue[filterNumber] =
          currentPowerValue[filterNumber] +
          queue_readElementAt(&(outputQueueArr[filterNumber]), j) *
              queue_readElementAt(&(outputQueueArr[filterNumber]), j);
    }
  }
  // if forceComputeFromScratch is false assign the the powervalue the
  // previous power, but replacing the old values with the new values.
  else {
    currentPowerValue[filterNumber] = prev_power -
                                      (oldest_value * oldest_value) +
                                      (newest_value * newest_value);
  }

  oldestValues[filterNumber] =
      queue_readElementAt(&(outputQueueArr[filterNumber]), 0);
  return currentPowerValue[filterNumber];
}

// Returns the last-computed output power value for the IIR filter
// [filterNumber].
// filterNumber is the index of the corresponding channel
double filter_getCurrentPowerValue(uint16_t filterNumber) {
  return currentPowerValue[filterNumber];
}

// Get a copy of the current power values.
// This function copies the already computed values into a previously-declared
// array so that they can be accessed from outside the filter software by the
// detector. Remember that when you pass an array into a C function, changes
// to the array within that function are reflected in the returned array.
void filter_getCurrentPowerValues(double powerValues[]) {
  // Iterate though powerValues and assign it current power value
  for (uint16_t i = 0; i < POW_ARRAY_SIZE; i++) {
    powerValues[i] = currentPowerValue[i];
  }
}

// Using the previously-computed power values that are current stored in
// currentPowerValue[] array, Copy these values into the normalizedArray[]
// argument and then normalize them by dividing all of the values in
// normalizedArray by the maximum power value contained in
// currentPowerValue[].
void filter_getNormalizedPowerValues(double normalizedArray[],
                                     uint16_t *indexOfMaxValue) {
  // Iterate through noramlized array and assign values of power ratios
  for (uint16_t i = 0; i < POW_ARRAY_SIZE; i++) {
    normalizedArray[i] =
        currentPowerValue[i] / currentPowerValue[*indexOfMaxValue];
  }
}

/*********************************************************************************************************
********************************** Verification-assisting functions.
**************************************
********* Test functions access the internal data structures of the filter.c
*via these functions. ********
*********************** These functions are not used by the main filter
*functions. ***********************
**********************************************************************************************************/

// Returns the array of FIR coefficients.
const double *filter_getFirCoefficientArray() { return firCoefficients; }

// Returns the number of FIR coefficients.
uint32_t filter_getFirCoefficientCount() { return FIR_FILTER_TAP_COUNT; }

// Returns the array of coefficients for a particular filter number.
const double *filter_getIirACoefficientArray(uint16_t filterNumber) {
  return iirACoefficientConstants[filterNumber];
}

// Returns the number of A coefficients.
uint32_t filter_getIirACoefficientCount() { return IIR_A_COEFFICIENT_COUNT; }

// Returns the array of coefficients for a particular filter number.
const double *filter_getIirBCoefficientArray(uint16_t filterNumber) {
  return iirBCoefficientConstants[filterNumber];
}

// Returns the number of B coefficients.
uint32_t filter_getIirBCoefficientCount() { return IIR_B_COEFFICIENT_COUNT; }

// Returns the size of the yQueue.
uint32_t filter_getYQueueSize() { return Y_QUEUE_SIZE; }

// Returns the decimation value.
uint16_t filter_getDecimationValue() { return DECIMATION_VALUE; }

// Returns the address of xQueue.
queue_t *filter_getXQueue() { return &xQueue; }

// Returns the address of yQueue.
queue_t *filter_getYQueue() { return &yQueue; }

// Returns the address of zQueue for a specific filter number.
queue_t *filter_getZQueue(uint16_t filterNumber) {
  return &(zQueueArr[filterNumber]);
}

// Returns the address of the IIR output-queue for a specific filter-number.
queue_t *filter_getIirOutputQueue(uint16_t filterNumber) {
  return &outputQueueArr[filterNumber];
}
