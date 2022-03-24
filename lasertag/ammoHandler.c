#include "ammoHandler.h"

#define THREE_SEC 300000

#define NUM_SHOTS 10


// States for the controller state machine.
enum ammoHandler_st_t {
  init_st,       // Start here, transition out of this state on the first tick.
  idle_st, // Check if the trigger input is high
  outOfAmmo_st, // Wait 50ms to see if it is a real press
  manualReload_st    // Activate the transmitter state machine
};
static enum ammoHandler_st_t currentState;

static uint16_t stateCounter;

static bool triggerPulled;


void ammoHandlerRun(){
    triggerPulled = true;
}

void ammoHandlerStop(){
    triggerPulled = false;
}

void ammoHandler_init(){
    sound_init();
    trigger_init();
    stateCounter = 0;
    currentState = init_st;
}

void ammoHandler_tick(){
    // perform state transition first
   switch(currentState) {
    case init_st:
        currentState = idle_st;
      break;
    case idle_st:
    // if trigger pulled and no shots left go into out of ammo
        if(triggerPulled && (trigger_getRemainingShotCount() == 0)){
            sound_playSound(sound_gunClick_e);
            currentState = outOfAmmo_st;
        }
        // if trigger pulled enter manual reload
        else if (triggerPulled){ 
            sound_playSound(sound_gunFire_e);
            currentState = manualReload_st;
        }
        // else stay in this state
        else {
            currentState = idle_st;
        }
      break;
    case outOfAmmo_st:
        // if counter is up go back to idle
        if(stateCounter == THREE_SEC){
            currentState = idle_st;
        }
      break;
    case manualReload_st:
    // if trigger released go back to idle
        if(!triggerPulled){
            currentState = idle_st;
        }

        // if counter reaches 3 seconds reload
        else if(stateCounter == THREE_SEC){
            trigger_setRemainingShotCount(NUM_SHOTS);
            sound_playSound(sound_gunReload_e);
            currentState = idle_st;
        }
      break;
    default:
      // print an error message here.
      break;
  }

// perform state action next
  switch(currentState) {
    case init_st:
      break;
    case idle_st:
      break;
    case outOfAmmo_st:
    // if trigger pulled give a click
        if(triggerPulled){
            sound_playSound(sound_gunClick_e);
        }
        stateCounter++;
      break;
    case manualReload_st:
        stateCounter++;
      break;
    default:
      // print an error message here.
      break;
  }
}