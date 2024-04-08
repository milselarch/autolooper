#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include "parse_wav.h"
#include "loop.h"
#include "autoloop.h"

/**
 * Finds the diff of the samples within the given window
 * Averaged to account for window size
 */
unsigned long find_difference(short* start_buf, short* end_buf, int window_size) {
    unsigned long diff = 0;
    unsigned long i;
    long d;

    for (i = 0; i < window_size; i += 100) {
        d = (long)(start_buf[i] - end_buf[i]);
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
                                fout->headers.data_chunk_size; /* CHANGE TO 28 */

    /* Clean up */
    free(start_buf.data);
    free(end_buf.data);
    free(intro_buf.data);
    free(loop_buf.data);
    free(ending_buf.data);
    return 0;
}

int auto_loop(char * read_file, char* write_file)
{
    clock_t t;
    sndbuf all_smpl_buf;
    unsigned long start_offset;
    unsigned long end_offset;
    int window_size_sec = 20;
    FILE *fp;
    FILE *file_p;
    WavFile file;
    WavFile loop_file;

    fp = fopen(read_file, "r");
    if (NULL == fp)
    {
        printf("ERROR: Failed to open %s!\n",read_file);
        return 0;
    }

    file = read_frames(fp);
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
