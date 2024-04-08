#include <stdio.h>
#include <stdlib.h>
#include "parse_wav.h"
#include "loop.h"
#include "autoloop.h"
#include "fsm.h"

int main (int argc, char** argv) {
    unsigned long start_time, end_time, min_length;
    int res;
    char* end_ptr;
    FILE* fp;
    FILE* fpout;
    WavFile f, fout;
    FileExtFSM fileExtFsm;
    NumFSM numFsm;
    int result;

    if ((argc>3 && argc < 6) || argc<3) {
        printf("ERROR: Insufficient number of arguments!\n");
        printf("For regular loop usage: ./test INPUT_FILE OUTPUT_FILE [START_TIME] [END_TIME] [MIN_LENGTH] \nFor auto loop usage: ./test INPUT_FILE OUTPUT_FILE\n");
        printf("START_TIME, END_TIME and MIN_LENGTH should be provided in seconds for regular looping\n");
        return 1;
    }



    /* Perform checks on input */

    /* check read file input*/
    initFileExtFSM(&fileExtFsm);
    result = runFileExtFsm(&fileExtFsm,argv[1]);
    if(!result){
        printf("ERROR: File extension is not .wav: %s\n",argv[1]);
        return 1;
    }

    /* check write file input*/
    initFileExtFSM(&fileExtFsm);
    result=runFileExtFsm(&fileExtFsm,argv[2]);
    if(!result){
        printf("ERROR: File extension is not .wav: %s\n",argv[2]);
        return 1;
    }

    if(argc <= 3){

        int res=auto_loop(argv[1],argv[2]);
        return res;

    }

    /* check start time input*/
    initNumFSM(&numFsm);
    result=runNumFsm(&numFsm,argv[3]);
    if(!result){
        printf("ERROR: Invalid start time! : %s\n",argv[3]);
        return 1;
    }
    start_time = strtoul(argv[3], &end_ptr, 10);

    /* check end time input*/
    initNumFSM(&numFsm);
    result=runNumFsm(&numFsm,argv[4]);
    if(!result){
        printf("ERROR: Invalid end time! : %s\n",argv[4]);
        return 1;
    }
    end_time = strtoul(argv[4], &end_ptr, 10);

    if (start_time > end_time) {
        printf("ERROR: Start time is after end time!\n");
        return 1;
    }

    /* check min length input*/
    initNumFSM(&numFsm);
    result=runNumFsm(&numFsm,argv[5]);
    if(!result){
        printf("ERROR: Invalid min length! : %s\n",argv[5]);
        return 1;
    }
    min_length = strtoul(argv[5], &end_ptr, 10);


    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("ERROR: Failed to open %s!\n", argv[1]);
        return 1;
    }





    f = read_frames(fp);


    fpout = fopen(argv[2], "w");
    if (fpout == NULL) {
        printf("ERROR: Failed to open %s!\n", argv[2]);
        fclose(fp);
        return 1;
    }

    /* Extend audio and write to new file */
    fout.headers = f.headers;
    res = loop(&f, start_time, end_time, min_length, &fout);
    if (!res) {
        write_wav(fpout, fout);
    }

    /* Clean up */
    fclose(fp);
    fclose(fpout);
    return res;
}
