#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "parse_wav.h"
#include <time.h>
#include "loop.h"



typedef enum {
    STATE_0,
    STATE_1,
    STATE_2,
    STATE_3,
    STATE_4, /* ACCEPTING STATE*/

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

    if(fsm->currentState==STATE_1){
        return 0;
    }

    return 1;
}


int read_samples (WavFile* wavfile, sndbuf* buf, int channels, unsigned long offset, unsigned long duration) {
    unsigned long res = 0;
    short* wav_ptr;
    int i;

    /* Seek to offset */
    if (offset > wavfile->num_frames) {
        return 1;
    }
    wav_ptr = wavfile->unscaled_frames + offset * channels;
   
    /* Save a section of the audio of the specified duration */
    buf->size = duration * channels;
    buf->data = (short*) malloc(buf->size * sizeof(short));
    for (i = 0; i < buf->size; i++) {
        buf->data[i] = wav_ptr[i];
        res++;
    }
    if (res < duration) {
        printf("WARNING: Reached end of file before reading all samples. Number of samples read: %lu\n", res);
    }

    return 0;
}

unsigned long find_loop_end (sndbuf* start_buf, sndbuf* end_buf, int channels) {
    unsigned long duration = start_buf->size / channels;
    unsigned int best_offset = 0;
    unsigned long best_score = ULONG_MAX;
    unsigned long score;
    int diff;
    unsigned int i;
    unsigned int j;

    for (i = 0; i < duration; i++) {
        /* Calculate score for current offset */
        score = 0;
        for (j = 0; j < duration * channels; j += 1) {
            diff = start_buf->data[j] - end_buf->data[i * channels + j];
            score += diff * diff;
        }

        /* Update best score */
        if (score < best_score) {
            best_score = score;
            best_offset = i;
        }
    }

    printf("Best score: %lu\n", best_score);
    printf("Best offset: %d\n", best_offset);
    return best_offset;
}

/**
 * Finds the diff of the samples within the given window
 * Averaged to account for window size
 */
unsigned long find_difference(short* start_buf, short* end_buf, int window_size) {
    unsigned long diff = 0;
    for (unsigned long i = 0; i < window_size; i += 100) {
        long d = (long)(start_buf[i] - end_buf[i]);
        diff += d * d;
    }
    return diff / window_size;
}

int find_loop_points_auto(sndbuf* buf, unsigned int* start_time_buf, unsigned int* end_time_buf, int num_channels, int sample_rate, unsigned long window_size) {
    unsigned long *start_offset_buf;
    unsigned long *end_offset_buf;

    find_loop_points_auto_offsets(buf, start_offset_buf, end_offset_buf, num_channels, sample_rate, window_size);

    *start_time_buf = (unsigned int)(*start_offset_buf / (sample_rate * num_channels));
    *end_time_buf = (unsigned int)(*end_offset_buf / (sample_rate * num_channels));

    return 0;
}

/**
 * @param window_size Determines how far the similarity "lookahead" is, set to larger time values to prevent
 * segments close together in time to be considered as a loop.
 * However, this causes a problem when the "end" of the loop is not <window_size> seconds causing a mis-classification
 */
int find_loop_points_auto_offsets(sndbuf* buf, unsigned long* start_offset_buf, unsigned long* end_offset_buf, int num_channels, int sample_rate, unsigned long window_size) {
    short *sample_data = buf->data;

    unsigned long best_end = 0L;
    unsigned long best_start = 0L;
    unsigned long start, end;
    unsigned long best_score = ULONG_MAX;
    unsigned long score;

    /* step_size MUST be set to sample_rate or less to allow find_loop_end to successfully find the loop point */
    unsigned long step_size = (sample_rate / 6) * num_channels;
    printf("Finding Loop...\n");

    for (start = 0; start <= buf->size - window_size * num_channels; start += step_size) {
        printf("\r%f%% -- best segment score: %lu", (float)start * 100 / (float)(buf->size - window_size), best_score);
        fflush(stdout);

        for (end = start + window_size * num_channels; end <= buf->size - window_size * num_channels; end += step_size) {

            score = find_difference(
                sample_data + start,
                sample_data + end,
                window_size * num_channels);

            /* Check if this is the best score found so far
            Equality to detect furthest loop (else may detect similar sections of same verse) */
            if (score <= best_score) {
                best_score = score;
                best_start = start / num_channels;
                best_end = end / num_channels;
            }
        }
        printf("\r                                                                                          ");
    }

    printf("\rLoop finding completed -------------------------\n");
    printf("Best start time: %f\n", (float)best_start / sample_rate);
    printf("Best end time: %f\n", (float)best_end / sample_rate);
    *start_offset_buf = best_start;
    *end_offset_buf = best_end;
    return 0;
}

void copy_samples (sndbuf* src_buf, short* dst) {
    unsigned int i;
    for (i = 0; i < src_buf->size; i++) {
        dst[i] = src_buf->data[i];
    }
}

void extend_audio (sndbuf* extended_buf, sndbuf* intro_buf, sndbuf* loop_buf, sndbuf* ending_buf, unsigned int num_loops) {
    short* seek_ptr;
    unsigned int loop_ctr;

    /* Create a buffer for the extended audio */
    extended_buf->size = intro_buf->size + loop_buf->size * num_loops + ending_buf->size;
    extended_buf->data = (short*) malloc(extended_buf->size * sizeof(short));
    seek_ptr = extended_buf->data;

    /* Copy intro */
    copy_samples(intro_buf, seek_ptr);
    seek_ptr += intro_buf->size;

    /* Copy loops */
    for (loop_ctr = 0; loop_ctr < num_loops; loop_ctr++) {
        copy_samples(loop_buf, seek_ptr);
        seek_ptr += loop_buf->size;
    }

    /* Copy ending */
    copy_samples(ending_buf, seek_ptr);
}

int loop (WavFile* f, unsigned int start_time, unsigned int end_time, unsigned int min_length, WavFile* fout) {
    /*
    unsigned long intro_size, end_offset, loop_size, ending_size;
    WavHeaders info = f->headers;
    intro_size = start_time * info.sample_rate;
    end_offset = end_time * info.sample_rate;

    return loop_with_offsets(f, intro_size, end_offset, min_length, fout);
    */

    unsigned long intro_size, end_offset, loop_size, ending_size;
    sndbuf start_buf, end_buf, loop_buf, intro_buf, ending_buf, extended_buf;
    int res;
    unsigned int num_loops;
    WavHeaders info = f->headers;

    /* Save a section of the audio at the start time for comparison */
    intro_size = start_time * info.sample_rate;
    res = read_samples(f, &start_buf, info.num_channels, intro_size, info.sample_rate);
    if (res) {
        printf("ERROR: %i is an invalid timestamp!\n", start_time);
        return 1;
    }

    /* Save a section of end time audio */
    end_offset = end_time * info.sample_rate;
    res = read_samples(f, &end_buf, info.num_channels, end_offset, 2 * info.sample_rate);
    if (res) {
        printf("ERROR: %i is an invalid timestamp!\n", end_time);
        free(start_buf.data);
        return 1;
    }

    /* Find looping point */
    end_offset += find_loop_end(&start_buf, &end_buf, info.num_channels);

    /* Store the data for a loop */
    loop_size = end_offset - intro_size;
    read_samples(f, &loop_buf, info.num_channels, intro_size, loop_size);

    /* Store the data for the intro */
    read_samples(f, &intro_buf, info.num_channels, 0, intro_size);

    /* Store the data for the ending */
    ending_size = f->num_frames / info.num_channels - end_offset;
    read_samples(f, &ending_buf, info.num_channels, end_offset, ending_size);

    /* Compute number of loops */
    num_loops = (min_length * info.sample_rate - intro_size - ending_size) / loop_size + 1;
    printf("Number of loops: %d\n", num_loops);

    /* Create buffer for extended audio */
    extend_audio(&extended_buf, &intro_buf, &loop_buf, &ending_buf, num_loops);

    /* Write buffer into new file */
    fout->unscaled_frames = extended_buf.data;
    fout->num_frames = extended_buf.size;
    fout->headers.data_chunk_size = extended_buf.size * 2;
    fout->headers.chunk_size = 36 + fout->headers.sub_chunk1_size + fout->headers.sub_chunk2_size +
                                fout->headers.data_chunk_size; //CHANGE TO 28

    /* Clean up */
    free(start_buf.data);
    free(end_buf.data);
    free(intro_buf.data);
    free(loop_buf.data);
    free(ending_buf.data);
    return 0;
}

int loop_with_offsets(WavFile* f, unsigned long start_offset, unsigned long end_offset, unsigned int min_length, WavFile* fout) {
    unsigned long loop_size, ending_size;
    sndbuf start_buf, end_buf, loop_buf, intro_buf, ending_buf, extended_buf;
    int res;
    unsigned int num_loops;
    WavHeaders info = f->headers;

    res = read_samples(f, &start_buf, info.num_channels, start_offset, info.sample_rate);
    if (res) {
        printf("ERROR: start offset is an invalid timestamp!\n");
        return 1;
    }
    res = read_samples(f, &end_buf, info.num_channels, end_offset, 2 * info.sample_rate);
    if (res) {
        printf("ERROR: end offset is an invalid timestamp!\n");
        free(start_buf.data);
        return 1;
    }

    /* Find looping point */
    end_offset += find_loop_end(&start_buf, &end_buf, info.num_channels);

    /* Store the data for a loop */
    loop_size = end_offset - start_offset;
    read_samples(f, &loop_buf, info.num_channels, start_offset, loop_size);

    /* Store the data for the intro */
    read_samples(f, &intro_buf, info.num_channels, 0, start_offset);

    /* Store the data for the ending */
    ending_size = f->num_frames / info.num_channels - end_offset;
    read_samples(f, &ending_buf, info.num_channels, end_offset, ending_size);

    /* Compute number of loops */
    num_loops = (min_length * info.sample_rate - start_offset - ending_size) / loop_size + 1;
    printf("Number of loops: %d\n", num_loops);

    /* Create buffer for extended audio */
    extend_audio(&extended_buf, &intro_buf, &loop_buf, &ending_buf, num_loops);

    /* Write buffer into new file */
    fout->unscaled_frames = extended_buf.data;
    fout->num_frames = extended_buf.size;
    fout->headers.data_chunk_size = extended_buf.size * 2;
    fout->headers.chunk_size = 36 + fout->headers.sub_chunk1_size + fout->headers.sub_chunk2_size +
                                fout->headers.data_chunk_size; //CHANGE TO 28

    /* Clean up */
    free(start_buf.data);
    free(end_buf.data);
    free(intro_buf.data);
    free(loop_buf.data);
    free(ending_buf.data);
    return 0;
}
/*
int check_frames(short * frames1, short* frames2, int channels, unsigned long limit){
    for(int i=0;i<limit*channels;i++){
        if(frames1[i]!=frames2[i]){
            printf("CHECK WRONG: %d %d %d\n",i,frames1[i], frames2[i]);
            return 0;
        }
    }
    return 1;

}
*/



// int main (int argc, char** argv) {
//     /* int i; */

//     FILE *fp = fopen("recycling.wav", "r");
//     // FILE *fp = fopen("write2.wav", "r");
//     FILE* file_p;
//     WavFile file = read_frames(fp);
//     WavFile loop_file;
//     loop_file.headers = file.headers;
//     loop(&file, 0, 75, 120, &loop_file);
//     file_p = fopen("./write2.wav", "w");
//     // file_p = fopen("./write4.wav", "w");
//     if(NULL == file_p)
//         {
//             printf("fopen failed in main");
//             return 0;
//         }
//     write_wav(file_p, loop_file);
//     fclose(file_p);
//     fclose(fp);

//     /*
//     for(i=100000;i<100200;i++){
//         printf("%d\n",frames.frames[i]);
//     }



//     printf("num_frames:%ld",file.num_frames);
//     for(int i=100000;i<100200;i++){
//         printf("%d\n",file.unscaled_frames[i]);
//     }
//     if (res) {
//         return 1;
//     }
//     */

//     return 0;
// }



int main_auto_loop(char * read_file, char* write_file)
{
    clock_t t;
    sndbuf all_smpl_buf;
    unsigned long start_offset;
    unsigned long end_offset;

    /* int i; */
    FILE *fp;
    fp = fopen(read_file, "r");
    if (NULL == fp)
    {
        printf("ERROR: Failed to open %s!\n",read_file);
        return 0;
    }
    int window_size_sec = 20;

    FILE *file_p;
    WavFile file = read_frames(fp);
    WavFile loop_file;
    loop_file.headers = file.headers;

    /* Auto looping*/
    read_samples(&file, &all_smpl_buf, file.headers.num_channels, 0uL, file.num_frames);

    t = clock();
    find_loop_points_auto_offsets(&all_smpl_buf, &start_offset, &end_offset, file.headers.num_channels, file.headers.sample_rate, file.headers.sample_rate * window_size_sec);
    t = clock() - t;
    printf("Loop finding Time taken: %fs\n", ((double)t) / CLOCKS_PER_SEC);

    t = clock();
    loop_with_offsets(&file, start_offset, end_offset, 300, &loop_file);
    t = clock() - t;
    printf("Looping Time taken: %fs\n", ((double)t) / CLOCKS_PER_SEC);

    file_p = fopen(write_file, "w");
    if (NULL == file_p)
    {
        printf("ERROR: Failed to open %s!\n",write_file);
        return 0;
    }
    write_wav(file_p, loop_file);
    fclose(file_p);
    fclose(fp);
    
    return 0;
}


/* TODO replace main() with this after testing is done */
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
        printf("For regular loop usage: ./test INPUT_FILE OUTPUT_FILE START_TIME END_TIME MIN_LENGTH \nFor auto loop usage: ./test INPUT_FILE OUTPUT_FILE\n");
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

    if(argc==3){

        int res=main_auto_loop(argv[1],argv[2]);
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