typedef enum {
    STATE_0,
    STATE_1,
    STATE_2,
    STATE_3,
    STATE_4 /* ACCEPTING STATE*/

} FileExtState;

typedef enum {
    STATE_A,
    STATE_B /* REJECTING STATE*/
} NumState;

typedef struct {
    FileExtState currentState;
    char dot;
} FileExtFSM;

typedef struct {
    NumState currentState;
    int rangeStart;
    int rangeEnd;

} NumFSM;

void initFileExtFSM(FileExtFSM *fsm);

void initNumFSM(NumFSM *fsm);

void processCharFileExt(FileExtFSM *fsm, char input_char);

void processCharNum(NumFSM *fsm, char input_char);

int runFileExtFsm(FileExtFSM *fsm, const char *str);

int runNumFsm(NumFSM *fsm, const char *str);
