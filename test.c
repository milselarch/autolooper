#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sndfile.h>
#include "test.h"

int read_samples (SNDFILE* f, sndbuf* buf, int channels, sf_count_t offset, sf_count_t duration) {
    /* Seek to offset */
    sf_count_t res = sf_seek(f, offset, SEEK_SET);
    if (res == -1) {
        return 1;
    }

    /* Save a section of the audio of the specified duration */
    buf->size = duration * channels;
    buf->data = (short*) malloc(buf->size * sizeof(short));
    res = sf_readf_short(f, buf->data, duration);
    if (res < duration) {
        printf("WARNING: Reached end of file before reading all samples. Number of samples read: %lu\n", res);
    }

    return 0;
}

sf_count_t find_loop_end (sndbuf* start_buf, sndbuf* end_buf, int channels) {
    sf_count_t duration = start_buf->size / channels;
    unsigned int best_offset = 0;
    unsigned long best_score = ULONG_MAX;
    unsigned long score;
    int diff;
    unsigned int i;
    unsigned int j;

    for (i = 0; i < duration; i++) {
        /* Calculate score for current offset */
        score = 0;
        for (j = 0; j < duration * channels; j += 100) {
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

int loop (const char* input_filepath, unsigned int start_time, unsigned int end_time,
          unsigned int min_length, const char* output_filepath) {
    sf_count_t intro_size, end_offset, loop_size, ending_size;
    sndbuf start_buf, end_buf, loop_buf, intro_buf, ending_buf, extended_buf;
    int res;
    unsigned int num_loops;
    SF_INFO output_info;
    SNDFILE* fout;

    /* Used to store some file metadata, required by sf_open() */
    SF_INFO info = {0, 0, 0, 0, 0, 0};

    /* Open the audio file */
    SNDFILE* f = sf_open(input_filepath, SFM_READ, &info);

    /* Save a section of the audio at the start time for comparison */
    intro_size = start_time * info.samplerate;
    res = read_samples(f, &start_buf, info.channels, intro_size, info.samplerate);
    if (res) {
        printf("ERROR: %i is an invalid timestamp!\n", start_time);
        sf_close(f);
        return 1;
    }

    /* Save a section of end time audio */
    end_offset = end_time * info.samplerate;
    res = read_samples(f, &end_buf, info.channels, end_offset, 2 * info.samplerate);
    if (res) {
        printf("ERROR: %i is an invalid timestamp!\n", end_time);
        free(start_buf.data);
        sf_close(f);
        return 1;
    }

    /* Find looping point */
    end_offset += find_loop_end(&start_buf, &end_buf, info.channels);

    /* Store the data for a loop */
    loop_size = end_offset - intro_size;
    read_samples(f, &loop_buf, info.channels, intro_size, loop_size);

    /* Store the data for the intro */
    read_samples(f, &intro_buf, info.channels, 0, intro_size);

    /* Store the data for the ending */
    ending_size = info.frames - end_offset;
    read_samples(f, &ending_buf, info.channels, end_offset, ending_size);

    /* Compute number of loops */
    num_loops = (min_length * info.samplerate - intro_size - ending_size) / loop_size + 1;
    printf("Number of loops: %d\n", num_loops);

    /* Create buffer for extended audio */
    extend_audio(&extended_buf, &intro_buf, &loop_buf, &ending_buf, num_loops);

    /* Write buffer into new file */
    output_info.samplerate = info.samplerate;
    output_info.channels = info.channels;
    output_info.format = info.format;
    fout = sf_open(output_filepath, SFM_WRITE, &output_info);
    sf_writef_short(fout, extended_buf.data, extended_buf.size / info.channels);

    /* Clean up */
    free(start_buf.data);
    free(end_buf.data);
    free(intro_buf.data);
    free(loop_buf.data);
    free(ending_buf.data);
    free(extended_buf.data);
    sf_close(f);
    sf_close(fout);
    return 0;
}


int main (int argc, char** argv) {
        /* TODO Print args required */
    /* if (argc < 6) {
        printf("Insufficient number of arguments!\n");
        return 1;
    }
    */

    /* TODO Perform checks on input */
    int res = loop("INORGANIC_BEAT_stereo.flac", 0, 77, 600, OUTPUT);
    /* int res = loop(argv[1], argv[2], argv[3], argv[4]); */
    if (res) {
        return 1;
    }

    return 0;
}
