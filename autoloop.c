#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include "parse_wav.h"
#include "loop.h"
#include "autoloop.h"

/* #include <fftw3.h> */
#include <math.h>

/**
 * Simple, fast algorithm to find the mean squares of the samples within the given window
 * @param buf1 - First buffer compared  
 * @param buf2 - Second buffer compared
 * @param window_size - Size of compared window
 * @param step_size - Step size of comparison. Higher means more precise
 */
unsigned long find_difference(short* buf1, short* buf2, int window_size, unsigned long step_size) 
{
    unsigned long diff = 0;
    unsigned long i;
    long d;
    for (i = 0; i < window_size; i += step_size) {
        d = (long)(buf1[i] - buf2[i]);
        if (d != 0) {
            diff += sqrt(d * d);
            i ++;
        }
    }
    return (unsigned long)((float)diff / (float)i);
}

/*
// Alternative slow precise scorer, using FFT. Requires #include <fftw3.h>
unsigned long find_frequency_difference(short *start_buf, short *end_buf, int buf_size)
{
    unsigned long diff = 0;

    // buf size is divided by 2 due to fft symmetry across x axis
    fftw_complex *start_seg_freq = fftw_alloc_complex(buf_size / 2 + 1);
    fftw_complex *end_seg_freq = fftw_alloc_complex(buf_size / 2 + 1);
    double *start_buf_dbl = (double *)fftw_alloc_real(buf_size);
    double *end_buf_dbl = (double *)fftw_alloc_real(buf_size);

    // Convert short to dbl
    for (int i = 0; i < buf_size; ++i)
    {
        start_buf_dbl[i] = start_buf[i];
        end_buf_dbl[i] = end_buf[i];
    }

    fftw_plan start_p = fftw_plan_dft_r2c_1d(buf_size, start_buf_dbl, start_seg_freq, FFTW_ESTIMATE);
    fftw_plan end_p = fftw_plan_dft_r2c_1d(buf_size, end_buf_dbl, end_seg_freq, FFTW_ESTIMATE);

    fftw_execute(start_p);
    fftw_execute(end_p);

    for (int i = 0; i <= buf_size / 2; ++i)
    {
        double real_diff = start_seg_freq[i][0] - end_seg_freq[i][0];
        double imag_diff = start_seg_freq[i][1] - end_seg_freq[i][1];
        double mag_squared = real_diff * real_diff + imag_diff * imag_diff;

        diff += sqrt(mag_squared);
    }

    unsigned long average_diff = diff / (buf_size / 2 + 1);

    fftw_destroy_plan(start_p);
    fftw_destroy_plan(end_p);
    fftw_free(start_seg_freq);
    fftw_free(end_seg_freq);
    fftw_free(start_buf_dbl);
    fftw_free(end_buf_dbl);

    return average_diff;
}
*/


/**
 * Returns the best score, with the start and end offsets identified throughout buf,
 * with a given sliding window size
 * @param buf - Buffer of samples
 * @param start_offset_buf - Long buffer in which optimal start offset is returned
 * @param end_offset_buf - Long buffer in which optimal end offset is returned
 * @param num_channels - Number of channels for this audio track
 * @param sample_rate - Sample rate of this audio track
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

    for (start = 0; start <= buf->size - window_size * num_channels; start += step_size * num_channels) {
        printf("\rTesting window size %d -- %f%%", (int)window_size, (float)start * 100 / (float)(buf->size - window_size));
        fflush(stdout);

        for (end = start + window_size * num_channels; end <= buf->size - window_size * num_channels; end += step_size * num_channels) {

            score = find_difference(
                sample_data + start,
                sample_data + end,
                window_size * num_channels,
                100);

            /* Check if this is the best score found so far
            Equality to detect furthest loop (else may detect similar sections of same verse) */
            if (score <= best_score) {
                best_score = score;
                best_start = start;
                best_end = end;
            }
        }
    }
    printf("\rTesting window size %d -- 100.00000%%     \n", (int)window_size);
    *start_offset_buf = best_start;
    *end_offset_buf = best_end;
    return best_score;
}

/**
 * Finds the best loop start and end integer timings (rounded down) throughout a given sndbuf.
 * @param buf - The buffer for the samples to search
 * @param start_offset_buf - Int buffer in which optimal start offset is returned
 * @param end_offset_buf - Int buffer in which optimal end offset is returned
 * @param num_channels - Number of channels for this audio track
 * @param sample_rate - Sample rate of this audio track
 */
int find_loop_points_auto(sndbuf* buf, unsigned int* start_time_buf, unsigned int* end_time_buf, int num_channels, int sample_rate) 
{
    unsigned long start_offset;
    unsigned long end_offset;

    find_loop_points_auto_offsets(buf, &start_offset, &end_offset, num_channels, sample_rate);

    *start_time_buf = (unsigned int)(start_offset / (sample_rate * num_channels));
    *end_time_buf = (unsigned int)(end_offset / (sample_rate * num_channels));

    return 0;
}

/**
 * Finds the best loop start and end offsets throughout a given sndbuf.
 * @param buf - The buffer for the samples to search
 * @param start_offset_buf - Long buffer in which optimal start offset is returned
 * @param end_offset_buf - Long buffer in which optimal end offset is returned
 * @param num_channels - Number of channels for this audio track
 * @param sample_rate - Sample rate of this audio track
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

    printf("LOOP FINDING START ==============\n");

    /* Find the best candidate for each window size */
    for (win_size = min_window_size; win_size <= max_window_size; win_size += wind_step)
    {
        /* Preliminary offset selection */
        get_window_score(buf, start_offset_buf, end_offset_buf, num_channels, sample_rate, win_size * sample_rate, step_size);

        curr_start_offset = *start_offset_buf;
        curr_end_offset = *end_offset_buf;

        /* 
        Take half a step back for possibility that match point occurs before the offset, 
        since find_loop_end looks forward only 
        */
        curr_end_offset = (curr_end_offset < step_size / 2) ? curr_end_offset : curr_end_offset - step_size / 2;

        /* Find the optimal end_offset, assuming start_offset is correct, within a 1 second duration. */
        curr_end_offset = curr_end_offset + find_loop_end_short_arr(
                                                sample_data + curr_start_offset, sample_rate * num_channels,
                                                sample_data + curr_end_offset, sample_rate * num_channels,
                                                num_channels);

        /* Score the found offsets */
        score = find_difference(
            sample_data + curr_start_offset, 
            sample_data + curr_end_offset, 
            sample_rate * num_channels,
            best_diff_step_size
            );
        
        /* // Alternative scorer
        score = find_frequency_difference(
            sample_data + curr_start_offset, 
            sample_data + curr_end_offset, 
            sample_rate * num_channels
            );
        */
        

        if (score <= best_score) 
        {
            printf("\tNew best start time: %f\n", (float)curr_start_offset / (float)sample_rate / num_channels);
            printf("\tNew best end time: %f\n", (float)curr_end_offset / (float)sample_rate / num_channels);
            best_score = score;
            best_end = curr_end_offset;
            best_start = curr_start_offset;
            best_win_size = win_size; /* Reporting purpose */
        }
    }

    *start_offset_buf = best_start;
    *end_offset_buf = best_end;

    printf("\rLoop finding completed -------------------------\n");
    printf("\tBest approx start time: %f\n", (float)best_start / sample_rate / num_channels);
    printf("\tBest approx end time: %f\n", (float)best_end / sample_rate / num_channels);
    printf("\tBest window size: %d\n", best_win_size);
    
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
    fout->headers.chunk_size = 28 + fout->headers.sub_chunk1_size + fout->headers.sub_chunk2_size +
                                fout->headers.data_chunk_size;

    /* Clean up */
    free(start_buf.data);
    free(end_buf.data);
    free(intro_buf.data);
    free(loop_buf.data);
    free(ending_buf.data);
    return 0;
}

/* TODO: add docs, add loop length*/
int auto_loop(FILE* fp, FILE* fpout, unsigned long min_length)
{
    clock_t t;
    sndbuf all_smpl_buf;
    unsigned long start_offset;
    unsigned long end_offset;
    WavFile file;
    WavFile loop_file;

    file = read_frames(fp);
    loop_file.headers = file.headers;

    /* Auto looping*/
    read_samples(&file, &all_smpl_buf, file.headers.num_channels, 0uL, file.num_frames / file.headers.num_channels);

    t = clock();
    find_loop_points_auto_offsets(&all_smpl_buf, &start_offset, &end_offset, file.headers.num_channels, file.headers.sample_rate);
    t = clock() - t;
    printf("Loop finding Time taken: %fs\n", ((double)t) / CLOCKS_PER_SEC);

    t = clock();
    loop_with_offsets(&file, start_offset / file.headers.num_channels, end_offset / file.headers.num_channels, min_length, &loop_file);
    t = clock() - t;
    printf("Looping Time taken: %fs\n", ((double)t) / CLOCKS_PER_SEC);

    write_wav(fpout, loop_file);
    fclose(fpout);
    fclose(fp);

    return 0;
}
