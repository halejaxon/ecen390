#ifndef LIVES_COUNTER_H
#define LIVES_COUNTER_H

/*
/   livesCounter.h  Ecen 390    Winter 2022
/   Handles decrementing hit & life counters  
/   Plays sounds corrosponding to getting hit, loosing a life, dying
/       and repeated game over reminder
/   Hit acts as main input to state machine, init and tick are normal state machine methods
*/


#define NUM_LIVES 3
#define MAX_HITS_PER_LIFE 5

//init function for lives counter
void lives_init();

//standard tick function
void lives_tick();

//player is "hit"
void lives_hit();

//returns true if the player is dead
bool lives_outOfLives();

//get the number of hits left
int16_t lives_getHits();

//get the number of lives left
int16_t lives_getLives();

//resets the lives for next game
void lives_reset();





#endif