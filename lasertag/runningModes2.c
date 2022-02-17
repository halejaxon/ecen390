/*
This software is provided for student assignment use in the Department of
Electrical and Computer Engineering, Brigham Young University, Utah, USA.
Users agree to not re-host, or redistribute the software, in source or binary
form, to other persons or other institutions. Users may modify and use the
source code for personal or educational use.
For questions, contact Brad Hutchings or Jeff Goeders, https://ece.byu.edu/
*/

#include "interrupts.h"
#include "runningModes.h"

#include <stdio.h>

/*
This file (runningModes2.c) is separated from runningModes.c so that
check_and_zip.py does not include provided code for grading. Code for
submission can be added to this file and will be graded. The code in
runningModes.c can give you some ideas about how to implement other
modes here.
*/

void runningModes_twoTeams() {
  uint16_t hitCount = 0;
  runningModes_initAll();
  // More initialization...

  // Implement game loop...

  interrupts_disableArmInts(); // Done with game loop, disable the interrupts.
  hitLedTimer_turnLedOff();    // Save power :-)
  runningModes_printRunTimeStatistics(); // Print the run-time statistics to the
                                         // TFT.
  printf("Two-team mode terminated after detecting %d shots.\n", hitCount);
}
