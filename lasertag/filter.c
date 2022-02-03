#include "filter.h"

#define IIR_A_COEFFICIENT_COUNT 10 // IS THIS SUPPOSED TO BE 10??
#define IIR_B_COEFFICIENT_COUNT 11
#define FILTER_IIR_FILTER_COUNT 10
#define QUEUE_INIT_VALUE 0.0
#define X_QUEUE_SIZE 10
#define Y_QUEUE_SIZE 10
#define Z_QUEUE_SIZE IIR_A_COEFFICIENT_COUNT
#define FIR_FILTER_TAP_COUNT 81
#define POWER_QUEUE_SIZE 2000
#define NUM_POW_QUEUES 10
#define POW_ARRAY_SIZE 10

// Static variables
static queue_t xQueue;
static queue_t yQueue;
static queue_t zQueueArr[FILTER_IIR_FILTER_COUNT];
static queue_t outputQueueArr[FILTER_IIR_FILTER_COUNT];
static queue_t powerOutputArr[NUM_POW_QUEUES];
static double currentPowerValue[POW_ARRAY_SIZE];
static double oldestValues[FILTER_IIR_FILTER_COUNT];

const static double firCoefficients[FIR_FILTER_TAP_COUNT] = {
-6.0546138291252586e-04, 
-6.5927707843654155e-04, 
-6.0586073290698971e-04, 
-4.2736469953868247e-04, 
-1.1360489934931769e-04, 
3.2323657622985996e-04, 
8.3401783160565320e-04, 
1.3253990136045740e-03, 
1.6663618496969908e-03, 
1.7115332283306618e-03, 
1.3391895850032674e-03, 
4.9619541794118714e-04, 
-7.6225514924622582e-04, 
-2.2518009934394279e-03, 
-3.6684783549916964e-03, 
-4.6341542172396834e-03, 
-4.7744814146589076e-03, 
-3.8163869519466371e-03, 
-1.6841031105183668e-03, 
1.4312288122698509e-03, 
5.0516421324586537e-03, 
8.4575364447581295e-03, 
1.0800471789680084e-02, 
1.1270477287000728e-02, 
9.2899200683230678e-03, 
4.6954271507319619e-03, 
-2.1339037972576192e-03, 
-1.0238121277430458e-02, 
-1.8149697899123553e-02, 
-2.4079631819971622e-02, 
-2.6194351879530921e-02, 
-2.2940474010799447e-02, 
-1.3360421141947862e-02, 
2.6617177832199256e-03, 
2.4276433492163109e-02, 
4.9700200797613801e-02, 
7.6408062643700300e-02, 
1.0146367706285764e-01, 
1.2193648744671268e-01, 
1.3533757305167757e-01, 
1.4000000000000001e-01, 
1.3533757305167757e-01, 
1.2193648744671268e-01, 
1.0146367706285764e-01, 
7.6408062643700300e-02, 
4.9700200797613801e-02, 
2.4276433492163109e-02, 
2.6617177832199256e-03, 
-1.3360421141947862e-02, 
-2.2940474010799447e-02, 
-2.6194351879530921e-02, 
-2.4079631819971622e-02, 
-1.8149697899123553e-02, 
-1.0238121277430458e-02, 
-2.1339037972576192e-03, 
4.6954271507319619e-03, 
9.2899200683230678e-03, 
1.1270477287000728e-02, 
1.0800471789680084e-02, 
8.4575364447581295e-03, 
5.0516421324586537e-03, 
1.4312288122698509e-03, 
-1.6841031105183668e-03, 
-3.8163869519466371e-03, 
-4.7744814146589076e-03, 
-4.6341542172396834e-03, 
-3.6684783549916964e-03, 
-2.2518009934394279e-03, 
-7.6225514924622582e-04, 
4.9619541794118714e-04, 
1.3391895850032674e-03, 
1.7115332283306618e-03, 
1.6663618496969908e-03, 
1.3253990136045740e-03, 
8.3401783160565320e-04, 
3.2323657622985996e-04, 
-1.1360489934931769e-04, 
-4.2736469953868247e-04, 
-6.0586073290698971e-04, 
-6.5927707843654155e-04, 
-6.0546138291252586e-04};

const static double iirACoefficientConstants[FILTER_FREQUENCY_COUNT][IIR_A_COEFFICIENT_COUNT] = {
{-5.9637727070164033e+00, 1.9125339333078266e+01, -4.0341474540744230e+01, 6.1537466875368942e+01, -7.0019717951472359e+01, 6.0298814235239050e+01, -3.8733792862566432e+01, 1.7993533279581129e+01, -5.4979061224867900e+00, 9.0332828533800036e-01},
{-4.6377947119071479e+00, 1.3502215749461584e+01, -2.6155952405269787e+01, 3.8589668330738398e+01, -4.3038990303252703e+01, 3.7812927599537183e+01, -2.5113598088113825e+01, 1.2703182701888103e+01, -4.2755083391143529e+00, 9.0332828533800225e-01},
{-3.0591317915750951e+00, 8.6417489609637510e+00, -1.4278790253808843e+01, 2.1302268283304294e+01, -2.2193853972079218e+01, 2.0873499791105424e+01, -1.3709764520609381e+01, 8.1303553577931567e+00, -2.8201643879900473e+00, 9.0332828533799825e-01},
{-1.4071749185996774e+00, 5.6904141470697587e+00, -5.7374718273676422e+00, 1.1958028362868923e+01, -8.5435280598354844e+00, 1.1717345583835989e+01, -5.5088290876998807e+00, 5.3536787286077780e+00, -1.2972519209655635e+00, 9.0332828533800225e-01},
{8.2010906117760585e-01, 5.1673756579268533e+00, 3.2580350909220979e+00, 1.0392903763919163e+01, 4.8101776408669075e+00, 1.0183724507092460e+01, 3.1282000712126696e+00, 4.8615933365571671e+00, 7.5604535083144631e-01, 9.0332828533799225e-01},
{2.7080869856154477e+00, 7.8319071217995528e+00, 1.2201607990980708e+01, 1.8651500443681559e+01, 1.8758157568004471e+01, 1.8276088095998947e+01, 1.1715361303018838e+01, 7.3684394621253126e+00, 2.4965418284511749e+00, 9.0332828533799858e-01},
{4.9479835250075901e+00, 1.4691607003177602e+01, 2.9082414772101060e+01, 4.3179839108869331e+01, 4.8440791644688872e+01, 4.2310703962394328e+01, 2.7923434247706425e+01, 1.3822186510471004e+01, 4.5614664160654339e+00, 9.0332828533799869e-01},
{6.1701893352279811e+00, 2.0127225876810304e+01, 4.2974193398071584e+01, 6.5958045321253252e+01, 7.5230437667866312e+01, 6.4630411355739554e+01, 4.1261591079243900e+01, 1.8936128791950424e+01, 5.6881982915179901e+00, 9.0332828533799114e-01},
{7.4092912870072389e+00, 2.6857944460290138e+01, 6.1578787811202268e+01, 9.8258255839887383e+01, 1.1359460153696310e+02, 9.6280452143026224e+01, 5.9124742025776499e+01, 2.5268527576524267e+01, 6.8305064480743294e+00, 9.0332828533800336e-01},
{8.5743055776347745e+00, 3.4306584753117924e+01, 8.4035290411037209e+01, 1.3928510844056848e+02, 1.6305115418161665e+02, 1.3648147221895832e+02, 8.0686288623300072e+01, 3.2276361903872257e+01, 7.9045143816245078e+00, 9.0332828533800080e-01}
};

const static double iirBCoefficientConstants[FILTER_FREQUENCY_COUNT][IIR_B_COEFFICIENT_COUNT] = {
{9.0928731266278516e-10, -0.0000000000000000e+00, -4.5464365633139253e-09, -0.0000000000000000e+00, 9.0928731266278505e-09, -0.0000000000000000e+00, -9.0928731266278505e-09, -0.0000000000000000e+00, 4.5464365633139253e-09, -0.0000000000000000e+00, -9.0928731266278516e-10},
{9.0928720950257747e-10, 0.0000000000000000e+00, -4.5464360475128878e-09, 0.0000000000000000e+00, 9.0928720950257755e-09, 0.0000000000000000e+00, -9.0928720950257755e-09, 0.0000000000000000e+00, 4.5464360475128878e-09, 0.0000000000000000e+00, -9.0928720950257747e-10},
{9.0928689295236753e-10, 0.0000000000000000e+00, -4.5464344647618381e-09, 0.0000000000000000e+00, 9.0928689295236762e-09, 0.0000000000000000e+00, -9.0928689295236762e-09, 0.0000000000000000e+00, 4.5464344647618381e-09, 0.0000000000000000e+00, -9.0928689295236753e-10},
{9.0928657585690288e-10, 0.0000000000000000e+00, -4.5464328792845144e-09, 0.0000000000000000e+00, 9.0928657585690288e-09, 0.0000000000000000e+00, -9.0928657585690288e-09, 0.0000000000000000e+00, 4.5464328792845144e-09, 0.0000000000000000e+00, -9.0928657585690288e-10},
{9.0928684679133363e-10, 0.0000000000000000e+00, -4.5464342339566683e-09, 0.0000000000000000e+00, 9.0928684679133367e-09, 0.0000000000000000e+00, -9.0928684679133367e-09, 0.0000000000000000e+00, 4.5464342339566683e-09, 0.0000000000000000e+00, -9.0928684679133363e-10},
{9.0928629381056898e-10, -0.0000000000000000e+00, -4.5464314690528456e-09, -0.0000000000000000e+00, 9.0928629381056913e-09, -0.0000000000000000e+00, -9.0928629381056913e-09, -0.0000000000000000e+00, 4.5464314690528456e-09, -0.0000000000000000e+00, -9.0928629381056898e-10},
{9.0928394215999147e-10, -0.0000000000000000e+00, -4.5464197107999567e-09, -0.0000000000000000e+00, 9.0928394215999134e-09, -0.0000000000000000e+00, -9.0928394215999134e-09, -0.0000000000000000e+00, 4.5464197107999567e-09, -0.0000000000000000e+00, -9.0928394215999147e-10},
{9.0929542362086042e-10, 0.0000000000000000e+00, -4.5464771181043024e-09, 0.0000000000000000e+00, 9.0929542362086048e-09, 0.0000000000000000e+00, -9.0929542362086048e-09, 0.0000000000000000e+00, 4.5464771181043024e-09, 0.0000000000000000e+00, -9.0929542362086042e-10},
{9.0926195433045638e-10, 0.0000000000000000e+00, -4.5463097716522815e-09, 0.0000000000000000e+00, 9.0926195433045630e-09, 0.0000000000000000e+00, -9.0926195433045630e-09, 0.0000000000000000e+00, 4.5463097716522815e-09, 0.0000000000000000e+00, -9.0926195433045638e-10},
{9.0910102251894290e-10, 0.0000000000000000e+00, -4.5455051125947149e-09, 0.0000000000000000e+00, 9.0910102251894298e-09, 0.0000000000000000e+00, -9.0910102251894298e-09, 0.0000000000000000e+00, 4.5455051125947149e-09, 0.0000000000000000e+00, -9.0910102251894290e-10}
};

// Must call this prior to using any filter functions.
void filter_init() {
    printf("check 3\n");
    // Init queues and fill them with 0s.
    initXQueue();  // Call queue_init() on xQueue and fill it with zeros.
    initYQueue();  // Call queue_init() on yQueue and fill it with zeros.
    initZQueues(); // Call queue_init() on all of the zQueues and fill each z queue with zeros.
    initOutputQueues();  // Call queue_init() all of the outputQueues and fill each outputQueue with zeros.
}

// Use this to copy an input into the input queue of the FIR-filter (xQueue).
void filter_addNewInput(double x) {
    queue_push(&xQueue, x);
}

// Fills a queue with the given fillValue. For example,
// if the queue is of size 10, and the fillValue = 1.0,
// after executing this function, the queue will contain 10 values
// all of them 1.0.
void filter_fillQueue(queue_t *q, double fillValue) {
    for (uint32_t j=0; j < q->size; j++) {
        queue_overwritePush(&(xQueue), fillValue);
    }
}

// Invokes the FIR-filter. Input is contents of xQueue.
// Output is returned and is also pushed on to yQueue.
double filter_firFilter() {
    return 0; // FILLER
}

// Use this to invoke a single iir filter. Input comes from yQueue.
// Output is returned and is also pushed onto zQueue[filterNumber].
double filter_iirFilter(uint16_t filterNumber) {
    return 0; // FILLER
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
double filter_computePower(uint16_t filterNumber, bool forceComputeFromScratch, bool debugPrint) {
    double prev_power = currentPowerValue[filterNumber];
    double oldestValues[filterNumber] = queue_readElementAt(outputQueueArr[filterNumber]->size - 1);
    double newest_value = queue_readElementAt(0);
    
    if(forceComputeFromScratch){
        currentPowerValue[filterNumber] = 0;
        for (int16_t j = 0; j < POWER_QUEUE_SIZE; j++) {
            currentPowerValue[filterNumber] = currentPowerValue[filterNumber] + outputQueueArr[filterNumber]*outputQueueArr[filterNumber];
        }
    }
    else{
        currentPowerValue[filterNumber] = prev_power - (oldest-value * oldest-value) + (newest-value * newest-value)
    }
    
    return currentPowerValue[filterNumber]; // FILLER
}

// Returns the last-computed output power value for the IIR filter
// [filterNumber].
double filter_getCurrentPowerValue(uint16_t filterNumber) {
    return currentPowerValue[filterNumber]; // FILLER
}

// Get a copy of the current power values.
// This function copies the already computed values into a previously-declared
// array so that they can be accessed from outside the filter software by the
// detector. Remember that when you pass an array into a C function, changes to
// the array within that function are reflected in the returned array.
void filter_getCurrentPowerValues(double powerValues[]) {
    // FILLER
}

// Using the previously-computed power values that are current stored in
// currentPowerValue[] array, Copy these values into the normalizedArray[]
// argument and then normalize them by dividing all of the values in
// normalizedArray by the maximum power value contained in currentPowerValue[].
void filter_getNormalizedPowerValues(double normalizedArray[],
                                     uint16_t *indexOfMaxValue) {
                                         // FILLER
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
const double *filter_getFirCoefficientArray() {
    return 0; // FILLER
}

// Returns the number of FIR coefficients.
uint32_t filter_getFirCoefficientCount() {
    return 0; // FILLER
}

// Returns the array of coefficients for a particular filter number.
const double *filter_getIirACoefficientArray(uint16_t filterNumber) {
    return 0; // FILLER
}

// Returns the number of A coefficients.
uint32_t filter_getIirACoefficientCount() {
    return 0;// FILLER
}

// Returns the array of coefficients for a particular filter number.
const double *filter_getIirBCoefficientArray(uint16_t filterNumber) {
    return 0;// FILLER
}

// Returns the number of B coefficients.
uint32_t filter_getIirBCoefficientCount() {
    return 0; // FILLER
}

// Returns the size of the yQueue.
uint32_t filter_getYQueueSize() {
    return 0; // FILLER
}

// Returns the decimation value.
uint16_t filter_getDecimationValue() {
    return 0; // FILLER
}

// Returns the address of xQueue.
queue_t *filter_getXQueue() {
    queue_t * myQ;
    return myQ; // FILLER
}

// Returns the address of yQueue.
queue_t *filter_getYQueue() {
    queue_t * myQ;
    return myQ; // FILLER
}

// Returns the address of zQueue for a specific filter number.
queue_t *filter_getZQueue(uint16_t filterNumber) {
    queue_t * myQ;
    return myQ; // FILLER
}

// Returns the address of the IIR output-queue for a specific filter-number.
queue_t *filter_getIirOutputQueue(uint16_t filterNumber) {
    queue_t * myQ;
    return myQ; // FILLER
}



//--------------------------------MY ADDED FUNCTIONS------------------------
void initZQueues() {
    for (uint32_t i=0; i<FILTER_IIR_FILTER_COUNT; i++) {
        queue_init(&(zQueueArr[i]), Z_QUEUE_SIZE, itoa(i));
        for (uint32_t j=0; j<Z_QUEUE_SIZE; j++) {
            queue_overwritePush(&(zQueueArr[i]), QUEUE_INIT_VALUE);
        }
    }
}

void initOutputQueues() {
    for (uint32_t i=0; i<FILTER_IIR_FILTER_COUNT; i++) {
        queue_init(&(outputQueueArr[i]), Z_QUEUE_SIZE, itoa(i));
        for (uint32_t j=0; j<Z_QUEUE_SIZE; j++) {
            queue_overwritePush(&(outputQueueArr[i]), QUEUE_INIT_VALUE);
        }
    }
}

void initXQueue() {
    queue_init(&(xQueue), X_QUEUE_SIZE, "x queue");
    for (uint32_t j=0; j<X_QUEUE_SIZE; j++) {
        queue_overwritePush(&(xQueue), QUEUE_INIT_VALUE);
    }
}

void initYQueue() {
    queue_init(&(yQueue), Y_QUEUE_SIZE, "y queue");
    for (uint32_t i=0; i<Y_QUEUE_SIZE; i++) {
        queue_overwritePush(&(yQueue), QUEUE_INIT_VALUE);
    }
}
