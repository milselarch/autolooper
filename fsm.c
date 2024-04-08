#include "fsm.h"

void initFileExtFSM(FileExtFSM *fsm) {
    fsm->currentState=STATE_0;
    fsm->dot='.';

}

void initNumFSM(NumFSM *fsm) {
    fsm->currentState=STATE_0;

}

void processCharFileExt(FileExtFSM *fsm, char input_char) {

    if(input_char==fsm->dot){
        fsm->currentState=STATE_1;
    }

    else if((fsm->currentState==STATE_1) && ((input_char=='w') || (input_char=='W')) ) {
        fsm->currentState=STATE_2;
    }

    else if((fsm->currentState==STATE_2) && ((input_char=='a') || (input_char=='A')) ){
        fsm->currentState=STATE_3;
    }

    else if((fsm->currentState==STATE_3) && ((input_char=='v') || (input_char=='V')) ){
        fsm->currentState=STATE_4;
    }

    else{
        fsm->currentState=STATE_0;
    }


}

void processCharNum(NumFSM *fsm, char input_char) {

    if(input_char<'0' || input_char>'9'){
        fsm->currentState=STATE_1;

    }


}



int runFileExtFsm(FileExtFSM *fsm, const char *str) {
    int i =0;


    for(i=0;str[i]!='\0';i++){
        processCharFileExt(fsm,str[i]);
    }

    if(fsm->currentState==STATE_4){
        return 1;
    }

    return 0;
}

int runNumFsm(NumFSM *fsm, const char *str) {
    int i =0;


    for(i=0;str[i]!='\0';i++){
        processCharNum(fsm,str[i]);
    }

    if(fsm->currentState == STATE_A){
        return 0;
    }

    return 1;
}
