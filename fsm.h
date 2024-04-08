/**
 * States for the FSM that checks for file extensions
 */
typedef enum {
    STATE_0,
    STATE_1,
    STATE_2,
    STATE_3,
    STATE_4 /* ACCEPTING STATE */
} FileExtState;

/**
 * States for the FSM that checks for non-negative integers
 */
typedef enum {
    STATE_A,
    STATE_B /* REJECTING STATE */
} NumState;

/**
 * The FSM that checks for file extensions
 */
typedef struct {
    FileExtState currentState;
    char dot;
} FileExtFSM;

/**
 * The FSM that checks for non-negative integers
 */
typedef struct {
    NumState currentState;
} NumFSM;

void initFileExtFSM (FileExtFSM *fsm);

void initNumFSM (NumFSM *fsm);

void processCharFileExt (FileExtFSM *fsm, char input_char);

void processCharNum (NumFSM *fsm, char input_char);

int runFileExtFsm (FileExtFSM *fsm, const char *str);

int runNumFsm (NumFSM *fsm, const char *str);
