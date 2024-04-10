/**
 * @file loop.c
 * @brief Algorithm to find the best loop point from user input and create an extended audio file
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "parse_wav.h"
#include "loop.h"

/**
 * Copies a number of samples from the wav file into a buffer
 * @param wavfile - The pointer to the wav file
 * @param buf - The pointer to the destination buffer
 * @param channels - The number of channels in the audio
 * @param offset - The number of samples offset to start reading the wav file
 * @param duration - The number of samples to be read
 * @return Whether the samples were read successfully (0 if success)
 */
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

/**
 * Finds the closest matching looping point from the end timestamp
 * @param start_buf - The buffer for the samples at the start of the loop
 * @param end_buf - The buffer for the samples at the end of the loop
 * @param channels - The number of channels in the audio
 * @return The optimal offset in end_buf
 */
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
    sndbuf start_sndbuf, end_sndbuf;
    start_sndbuf.size = size;
    start_sndbuf.data = start_buf;
    end_sndbuf.size = size;
    end_sndbuf.data = end_buf;
    
    return find_loop_end(&start_sndbuf, &end_sndbuf, channels);
}

/**
 * Copies samples from a sndbuf to a regular short buffer
 * @param src_buf - The pointer to the sndbuf to copy from
 * @param dst - The short buffer to copy into
 */
void copy_samples (sndbuf* src_buf, short* dst) {
    unsigned int i;
    for (i = 0; i < src_buf->size; i++) {
        dst[i] = src_buf->data[i];
    }
}

/**
 * Creates a buffer of the extended audio
 * @param extended_buf - The pointer to the buffer for the extended audio
 * @param intro_buf - The pointer to the buffer that contains all audio before the loop
 * @param loop_buf - The pointer to the buffer that contains the audio in the loop
 * @param ending_buf - The pointer to the buffer that contains all audio after the loop
 * @param num_loops - The number of loops in the extended audio
 */
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

/**
 * Finds the best loop point from user input and creates the extended audio
 * @param f - The pointer to the input wav file
 * @param start_time - The estimated timestamp for the start of the loop (in seconds)
 * @param end_time - The estimated timestamp for the end of the loop (in seconds)
 * @param min_length - The minimum length of the extended audio (in seconds)
 * @param fout - The pointer to the output wav file
 * @return Whether the audio extension is successful (0 if success)
 */
int loop (WavFile* f, unsigned int start_time, unsigned int end_time, unsigned int min_length, WavFile* fout) {
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
