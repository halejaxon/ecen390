#include "livesCounter.h"
#include <stdint.h>
#include "sound.h"


//states in state machine
enum lives_st_t {
    init_st,
    idle_st,
    hit_st,
    gameOverReturn_st,
    gameOverSilence_st
};

lives_st_t currentState;

//internal variables
int16_t numLives;
int16_t numHits;
bool isHit;

//init function for lives counter
void lives_init(){
    currentState = init_st;
    numLives = NUM_LIVES;
    numHits = MAX_HITS_PER_LIFE;
    isHit = false;
}

//standard tick function
void lives_tick(){
    //state update
    switch (currState){
        case init_st:
            currState = idle_st
            break;

        case idle_st:
            //if hit go to hit state
            if (isHit) {
                currState = hit_st;
                --numHits;
                isHit = false;

                //check if lost life
                if (numHits == 0) {
                    --numLives;
                    numHits = MAX_HITS_PER_LIFE;

                    //if not dead, play loose life sound
                    if (numLives > 0){
                        sound_stopSound();
                        sound_playSound(sound_loseLife_e);
                    }
                }
                else { 
                    //hit sound
                    sound_stopSound();
                    sound_playSound(sound_hit_e);
                }
            }
            else{
                currState = idle_st;
            }
            break;

        case hit_st:
            //if out of lives, sadness
            if (numLives == 0) {
                currState = gameOverReturn_st;
                sound_stopSound();
                sound_playSound(sound_gameOver_e);
            }
            else{
                currState = idle_st;
            }
            break;

        case gameOverReturn_st:
            //if done playing voice, go to silence
            if (sound_isSoundComplete()){
                currState = gameOverSilence_st;
                sound_playOneSecondSilence();
            }
            break;

        case gameOverSilence_st:
            //if silence done, tell player to return again
            if (sound_isSoundComplete()){
                currState = gameOverReturn_st;
                sound_playSound(sound_returnToBase_e);
            }
            break;

        default:
            /*How?*/
    }
}

//player is "hit"
void lives_hit(){
    isHit = true;
}

//returns true if the player is dead
bool lives_outOfLives(){
    return (numLives == 0);
}

//get the number of hits left
int16_t lives_getHits(){
    return numHits;
}

//get the number of lives left
int16_t lives_getLives(){
    return numLives;
}

//resets the live for next game
void lives_reset(){
    lives_init();
}