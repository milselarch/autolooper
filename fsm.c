/**
 * @file fsm.c
 * @brief Finite state machines to validate inputs to main.c
 */
#include "fsm.h"

/**
 * Initialises the FSM used for checking the file extension
 * @param fsm - The pointer to the FSM to be initialised
 */
void initFileExtFSM (FileExtFSM *fsm) {
    fsm->currentState = STATE_0;
    fsm->dot = '.';
}

/**
 * Initialises the FSM used for checking non-negative integers
 * @param fsm - The pointer to the FSM to be initialised
 */
void initNumFSM (NumFSM *fsm) {
    fsm->currentState = STATE_0;
}

/**
 * Runs the file extension checking FSM against a single input character
 * @param fsm - The pointer to the FSM
 * @param input_char - The input character for the FSM
 */
void processCharFileExt (FileExtFSM *fsm, char input_char) {
    if (input_char == fsm->dot) {
        fsm->currentState = STATE_1;
    }

    else if ((fsm->currentState == STATE_1) && ((input_char == 'w') || (input_char == 'W')) ) {
        fsm->currentState=STATE_2;
    }

    else if ((fsm->currentState == STATE_2) && ((input_char == 'a') || (input_char == 'A')) ) {
        fsm->currentState=STATE_3;
    }

    else if ((fsm->currentState == STATE_3) && ((input_char == 'v') || (input_char == 'V')) ) {
        fsm->currentState=STATE_4;
    }

    else {
        fsm->currentState=STATE_0;
    }
}

/**
 * Runs the integer checking FSM against a single input character
 * @param fsm - The pointer to the FSM
 * @param input_char - The input character for the FSM
 */
void processCharNum (NumFSM *fsm, char input_char) {
    if (input_char < '0' || input_char > '9') {
        fsm->currentState = STATE_B;
    }
}

/**
 * Checks if an input string contains a valid file extension
 * @param fsm - The pointer to the FSM
 * @param str - The input string
 * @return Whether the input string is valid (1 if valid)
 */
int runFileExtFsm (FileExtFSM *fsm, const char *str) {
    int i;

    for (i = 0; str[i] != '\0'; i++) {
        processCharFileExt(fsm, str[i]);
    }

    if (fsm->currentState == STATE_4){
        return 1;
    }

    return 0;
}

/**
 * Checks if an input string is a non-negative integer
 * @param fsm - The pointer to the FSM
 * @param str - The input string
 * @return Whether the input string is valid (1 if valid)
 */
int runNumFsm (NumFSM *fsm, const char *str) {
    int i;

    for (i = 0; str[i] != '\0'; i++) {
        processCharNum(fsm, str[i]);
    }

    if (fsm->currentState == STATE_B) {
        return 0;
    }

    return 1;
}
