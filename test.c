#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sndfile.h>

const char* OUTPUT = "output.flac";

int loop (const char* input_filepath, int start_time, int end_time, int min_length, const char* output_filepath) {
    /* Used to store some file metadata, required by sf_open() */
    SF_INFO info = {0,0,0,0,0,0};

    /* Open the audio file */
    SNDFILE* f = sf_open(input_filepath, SFM_READ, &info);

    /*
    info contains some useful data
    info.frames = length of file in frames
    info.channels = number of channels
    info.format = type of file (e.g. wav, flac) and bit depth (use masks)
    info.samplerate = number of samples per second
    */

    /* Convert start time in seconds to number of frames */
    sf_count_t start = start_time * info.samplerate;

    /* Seek to start time */
    sf_count_t res = sf_seek(f, start, SEEK_SET);
    if (res == -1) {
        /* error */
        return 1;
    }

    /* Save a section of the audio at the start time for comparison */
    sf_count_t start_frames = info.samplerate;
    short* start_sample = (short*) malloc(start_frames * info.channels * sizeof(short));
    sf_readf_short(f, start_sample, start_frames);

    /* Seek to end time */
    sf_count_t end = end_time * info.samplerate;
    res = sf_seek(f, end, SEEK_SET);
    if (res == -1) {
        /* error */
        return 1;
    }

    /* Save a section of end time audio */
    sf_count_t end_frames = 2 * info.samplerate;
    short* end_sample = (short*) malloc(end_frames * info.channels * sizeof(short));
    sf_readf_short(f, end_sample, end_frames);

    /* Find looping point */
    int best_offset = 0;
    unsigned long best_score = ULONG_MAX;
    for (int i = 0; i < start_frames; i++) {
        unsigned long score = 0;
        for (int j = 0; j < start_frames * info.channels; j += 100) {
            int diff = start_sample[j] - end_sample[i * info.channels + j];
            score += diff * diff;
        }
        if (!score) {
            best_offset = i;
            break;
        }
        if (score < best_score) {
            best_score = score;
            best_offset = i;
        }
    }
    printf("Best score: %lu\n", best_score);
    printf("Best offset: %d\n", best_offset);

    /* Store the data for a loop */
    sf_seek(f, start, SEEK_SET);
    sf_count_t loop_frames = end + best_offset - start;
    short* loop = (short*) malloc(loop_frames * info.channels * sizeof(short));
    sf_readf_short(f, loop, loop_frames);

    /* Store the data for the intro */
    short* intro;
    if (start) {
        sf_seek(f, 0, SEEK_SET);
        intro = (short*) malloc(start * info.channels * sizeof(short));
        sf_readf_short(f, intro, start);
    }

    /* Store the data for the ending */
    sf_seek(f, end + best_offset, SEEK_SET);
    sf_count_t ending_frames = info.frames - end - best_offset;
    short* ending = (short*) malloc(ending_frames * info.channels * sizeof(short));
    sf_readf_short(f, ending, ending_frames);

    /* Compute number of loops */
    int num_loops = (min_length * info.samplerate - start - ending_frames) / loop_frames + 1;
    printf("%d\n", num_loops);

    /* Create a buffer for the extended audio */
    sf_count_t total_len = start + loop_frames * num_loops + ending_frames;
    short* extended = (short*) malloc(total_len * info.channels * sizeof(short));
    printf("total_len: %lu\n", total_len);
    printf("frames: %lu\n", info.frames);
    printf("loop frames: %lu\n", loop_frames);
    printf("ending frames: %lu\n", ending_frames);

    /* Copy intro */
    int j = 0;
    if (start) {
        for (unsigned int i = 0; i < start * info.channels; i++) {
            extended[j] = intro[i];
            j++;
        }
    }

    /* Copy loops */
    printf("%lu\n", loop_frames * info.channels);
    printf("%lu\n", end * info.channels);
    for (int k = 0; k < num_loops; k++) {
        for (unsigned int i = 0; i < loop_frames * info.channels; i++) {
            extended[j] = loop[i];
            j++;
        }
    }

    /* Copy ending */
    for (unsigned int i = 0; i < ending_frames * info.channels; i++) {
        extended[j] = ending[i];
        j++;
    }

    /* Write buffer into new file */
    SF_INFO output_info = {0, info.samplerate, info.channels, info.format, 0, 0};
    SNDFILE* fout = sf_open(output_filepath, SFM_WRITE, &output_info);
    sf_writef_short(fout, extended, total_len);

    /* Clean up */
    free(start_sample);
    free(end_sample);
    if (start) {
        free(intro);
    }
    free(loop);
    free(ending);
    free(extended);
    sf_close(f);
    sf_close(fout);
    return 0;
}


int main () {
    // int res = loop("INORGANIC_BEAT_stereo.flac", 0, 77, 600, OUTPUT);
    // int res = loop("INTERCEPTION_stereo_hard.flac", 9, 179, 1800, OUTPUT);
    // int res = loop("02 SEGMENT1-2 BOSS.flac", 0, 98, 600, OUTPUT);
    // int res = loop("Let Ass Kick Together!.flac", 1, 68, 600, OUTPUT);
    // int res = loop("23 なにみてはねる.flac", 1, 80, 1800, OUTPUT);
    // int res = loop("15 Expert Course Stage 1.flac", 14, 42, 1800, OUTPUT);
    int res = loop("2-06 Boss.flac", 7, 109, 1800, OUTPUT);
    if (res) {
        return 1;
    }

    return 0;
}
