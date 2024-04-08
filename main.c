/**
 * @file main.c
 * @brief Entry point for the audio extension program
 */
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

    /* Perform checks on input */
    if (argc != 3 && argc != 6) {
        printf("ERROR: Insufficient number of arguments!\n");
        printf("Usage: ./main INPUT_FILE OUTPUT_FILE [START_TIME] [END_TIME] [MIN_LENGTH]\n");
        printf("START_TIME, END_TIME and MIN_LENGTH should be provided in seconds\n");
        return 1;
    }

    /* Check read file */
    initFileExtFSM(&fileExtFsm);
    res = runFileExtFsm(&fileExtFsm, argv[1]);
    if (!res) {
        printf("ERROR: File extension of %s is not .wav!\n", argv[1]);
        return 1;
    }

    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("ERROR: Failed to open %s!\n", argv[1]);
        return 1;
    }

    /* Check write file */
    initFileExtFSM(&fileExtFsm);
    res = runFileExtFsm(&fileExtFsm, argv[2]);
    if (!res) {
        printf("ERROR: File extension of %s is not .wav!\n", argv[2]);
        fclose(fp);
        return 1;
    }

    fpout = fopen(argv[2], "w");
    if (fpout == NULL) {
        printf("ERROR: Failed to open %s!\n", argv[2]);
        fclose(fp);
        return 1;
    }

    if (argc == 3) {
        res = auto_loop(fp, fpout);
        return res;
    }

    /* Check start time */
    initNumFSM(&numFsm);
    res=runNumFsm(&numFsm, argv[3]);
    if (!res) {
        printf("ERROR: Invalid start time!\n");
        return 1;
    }
    start_time = strtoul(argv[3], &end_ptr, 10);

    /* Check end time */
    initNumFSM(&numFsm);
    res = runNumFsm(&numFsm, argv[4]);
    if (!res) {
        printf("ERROR: Invalid end time!\n");
        return 1;
    }

    if (start_time > end_time) {
        printf("ERROR: Start time is after end time!\n");
        return 1;
    }
    end_time = strtoul(argv[4], &end_ptr, 10);

    /* Check min length */
    initNumFSM(&numFsm);
    res = runNumFsm(&numFsm, argv[5]);
    if (!res) {
        printf("ERROR: Invalid min length!\n");
        return 1;
    }
    min_length = strtoul(argv[5], &end_ptr, 10);

    /* Extend audio and write to new file */
    f = read_frames(fp);
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
