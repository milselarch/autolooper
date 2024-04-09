#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "parse_wav.h"
#include <time.h>
#include "loop.h"

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


unsigned long find_loop_end_short_arr (short* start_buf, short* end_buf, unsigned long size, int channels) {
    unsigned long duration = size / channels;
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
            diff = start_buf[j] - end_buf[i * channels + j];
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
 * Simple, fast algorithm to find the diff of the samples within the given window
 * Averaged to account for window size
 */
unsigned long find_difference(short* start_buf, short* end_buf, int window_size, unsigned long step_size) 
{
    unsigned long diff = 0;
    unsigned long i;
    long d;
    for (i = 0; i < window_size; i += step_size) {
        d = (long)(start_buf[i] - end_buf[i]);
        diff += d * d;
    }
    return (unsigned long) ((float)diff /(float)window_size);
}

/**
 * Returns the best score, with the start and end offsets identified throughout buf,
 * with a given sliding window size
 * @param window_size size of the sliding window in offset
 * @param step_size step increment of sliding window for each comparison
*/
int get_window_score(sndbuf* buf, unsigned long* start_offset_buf, unsigned long* end_offset_buf, int num_channels, int sample_rate, unsigned long window_size, unsigned long step_size) 
{
    short *sample_data = buf->data;
    unsigned long start, end;
    unsigned long best_end = 0L;
    unsigned long best_start = 0L;
    unsigned long best_score = ULONG_MAX;
    unsigned long score;

    for (start = 0; start <= buf->size - window_size * num_channels; start += step_size) {
        printf("\r%f%% -- best segment score: %lu", (float)start * 100 / (float)(buf->size - window_size), best_score);
        fflush(stdout);

        for (end = start + window_size * num_channels; end <= buf->size - window_size * num_channels; end += step_size) {

            score = find_difference(
                sample_data + start,
                sample_data + end,
                window_size * num_channels,
                100);

            /* Check if this is the best score found so far
            Equality to detect furthest loop (else may detect similar sections of same verse) */
            if (score <= best_score) {
                best_score = score;
                best_start = start / num_channels;
                best_end = end / num_channels;
            }
        }
        printf("\r                                                                                          \r");
    }
    *start_offset_buf = best_start;
    *end_offset_buf = best_end;
    return best_score;
}

/**
 * Finds the best loop start and end offsets throughout buf
 */
int find_loop_points_auto_offsets(sndbuf* buf, unsigned long* start_offset_buf, unsigned long* end_offset_buf, int num_channels, int sample_rate) {
    short *sample_data = buf->data;

    /* Candidate-finding window settings in seconds */
    int best_win_size = -1;
    int min_window_size = 10;
    int max_window_size = 25;
    int wind_step = 5; 

    /* Output tracking */
    unsigned long best_end = 0L;
    unsigned long best_start = 0L;
    unsigned long best_score = ULONG_MAX;
    unsigned long score;

    /* step_size MUST be set to sample_rate or less to allow find_loop_end to successfully find the loop point */
    unsigned long step_size = (sample_rate / 6) * num_channels;
    int best_diff_step_size = 1;


    /* Iterator helpers */
    unsigned long curr_start_offset;
    unsigned long curr_end_offset;
    int win_size;
    int i;
    printf("LOOP FINDING START ==============\n");
    printf("Finding Loop candidates ...\n");

    /* Find the best candidate for each window size */
    for (win_size = min_window_size; win_size <= max_window_size; win_size += wind_step)
    {
        printf("\rWindow size: %d\n ----- ", win_size );
        get_window_score(buf, start_offset_buf, end_offset_buf, num_channels, sample_rate, win_size * sample_rate, step_size);


        curr_start_offset =  *start_offset_buf / num_channels ;
        curr_end_offset =  *end_offset_buf / num_channels - step_size/2;

        /* take half a step back for possibility that match point occurs before */
        curr_start_offset =  (curr_start_offset < step_size/2) ? curr_start_offset : curr_start_offset - step_size/2;
        curr_end_offset =  (curr_end_offset < step_size/2) ? curr_end_offset : curr_end_offset - step_size/2;
        
        printf("Best start time: %f\n", (float)*start_offset_buf / sample_rate);
        printf("Best end time: %f\n", (float)*end_offset_buf / sample_rate);

        curr_end_offset = curr_end_offset + find_loop_end_short_arr(
            sample_data + curr_start_offset, 
            sample_data + curr_end_offset, 
            sample_rate * num_channels, 
            sample_rate
            );

        score = find_difference(
            sample_data + curr_start_offset, 
            sample_data + curr_end_offset, 
            min_window_size * sample_rate * num_channels, 
            best_diff_step_size
            );

        printf("Score: %lu\n", score);
        if (score <= best_score) 
        {
            best_score = score;
            best_end = curr_end_offset / num_channels;
            best_start = curr_start_offset/ num_channels;
            best_win_size = win_size; /* Reporting purpose*/
        }
    }


    *start_offset_buf = best_start;
    *end_offset_buf = best_end;

    printf("\rLoop finding completed -------------------------\n");
    printf("Best approx start time: %f\n", (float)best_start / sample_rate);
    printf("Best approx end time: %f\n", (float)best_end / sample_rate);
    printf("Best window size: %d\n", best_win_size);
    
    return 0;
}

int find_loop_points_auto(sndbuf* buf, unsigned int* start_time_buf, unsigned int* end_time_buf, int num_channels, int sample_rate) 
{
    unsigned long *start_offset_buf;
    unsigned long *end_offset_buf;

    find_loop_points_auto_offsets(buf, start_offset_buf, end_offset_buf, num_channels, sample_rate);

    *start_time_buf = (unsigned int)(*start_offset_buf / (sample_rate * num_channels));
    *end_time_buf = (unsigned int)(*end_offset_buf / (sample_rate * num_channels));

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

/* TODO replace main() with this after testing is done */
int _main (int argc, char** argv) {
    unsigned long start_time, end_time, min_length;
    int res;
    char* end_ptr;
    FILE* fp;
    FILE* fpout;
    WavFile f, fout;

    if (argc < 6) {
        printf("ERROR: Insufficient number of arguments!\n");
        printf("Usage: ./test INPUT_FILE START_TIME END_TIME MIN_LENGTH OUTPUT_FILE\n");
        printf("START_TIME, END_TIME and MIN_LENGTH should be provided in seconds\n");
        return 1;
    }

    /* Perform checks on input */
    start_time = strtoul(argv[2], &end_ptr, 10);
    if (!start_time && end_ptr == argv[2]) {
        printf("ERROR: Invalid start time!\n");
        return 1;
    }

    end_time = strtoul(argv[3], &end_ptr, 10);
    if (!end_time && end_ptr == argv[3]) {
        printf("ERROR: Invalid end time!\n");
        return 1;
    }
    if (start_time > end_time) {
        printf("ERROR: Start time is after end time!\n");
        return 1;
    }

    min_length = strtoul(argv[4], &end_ptr, 10);
    if (!min_length && end_ptr == argv[4]) {
        printf("ERROR: Invalid minimum length!\n");
        return 1;
    }

    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("ERROR: Failed to open %s!\n", argv[1]);
        return 1;
    }
    f = read_frames(fp);

    fpout = fopen(argv[5], "w");
    if (fpout == NULL) {
        printf("ERROR: Failed to open %s!\n", argv[5]);
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


int __main (int argc, char** argv) {
    /* int i; */

    FILE *fp = fopen("recycling.wav", "r");
    // FILE *fp = fopen("write2.wav", "r");
    FILE* file_p;
    WavFile file = read_frames(fp);
    WavFile loop_file;
    loop_file.headers = file.headers;
    loop(&file, 0, 75, 120, &loop_file);
    file_p = fopen("./write2.wav", "w");
    // file_p = fopen("./write4.wav", "w");
    if(NULL == file_p)
        {
            printf("fopen failed in main");
            return 0;
        }
    write_wav(file_p, loop_file);
    fclose(file_p);
    fclose(fp);

    /*
    for(i=100000;i<100200;i++){
        printf("%d\n",frames.frames[i]);
    }



    printf("num_frames:%ld",file.num_frames);
    for(int i=100000;i<100200;i++){
        printf("%d\n",file.unscaled_frames[i]);
    }
    if (res) {
        return 1;
    }
    */

    return 0;
}



int main(int argc, char **argv)
{
    clock_t t;
    sndbuf all_smpl_buf;
    unsigned long start_offset;
    unsigned long end_offset;

    /* int i; */
    FILE *fp;
    fp = fopen("recycling_mono.wav", "r");

    FILE *file_p;
    WavFile file = read_frames(fp);
    WavFile loop_file;
    loop_file.headers = file.headers;

    /* Auto looping*/
    read_samples(&file, &all_smpl_buf, file.headers.num_channels, 0uL, file.num_frames);

    t = clock();
    find_loop_points_auto_offsets(&all_smpl_buf, &start_offset, &end_offset, file.headers.num_channels, file.headers.sample_rate);
    t = clock() - t;
    printf("Loop finding Time taken: %fs\n", ((double)t) / CLOCKS_PER_SEC);

    t = clock();
    loop_with_offsets(&file, start_offset, end_offset, 300, &loop_file);
    t = clock() - t;
    printf("Looping Time taken: %fs\n", ((double)t) / CLOCKS_PER_SEC);

    file_p = fopen("./write_recycling.wav", "w");
    if (NULL == file_p)
    {
        printf("fopen failed in main");
        return 0;
    }
    write_wav(file_p, loop_file);
    fclose(file_p);
    fclose(fp);
    
    return 0;
}
