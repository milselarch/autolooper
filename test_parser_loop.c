#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sndfile.h>
#include "test.h"
#include "parse_wav.c"

typedef struct {
    short * frames;
    long int frame_len;
} Frames;

short* read_samples (SNDFILE* f, int channels, sf_count_t offset, sf_count_t duration) {
    sf_count_t res;
    short* data;

    /* Seek to offset */
    res = sf_seek(f, offset, SEEK_SET);
    if (res == -1) {
        return NULL;
    }

    /* Save a section of the audio of the specified duration */
    data = (short*) malloc(duration * channels * sizeof(short));

    res = sf_readf_short(f, data, duration);
    if (res < duration) {
        printf("WARNING: Reached end of file before reading all samples. Number of samples read: %i\n", res);
    }

    return data;
}

short* read_samples2 ( int channels, sf_count_t offset, sf_count_t duration, WavFile wavfile) {
    sf_count_t res;
    short* data;

   
    /* Save a section of the audio of the specified duration */
    data = (short*) malloc(duration * channels * sizeof(short));
    int j=offset*channels;

    for(int i=0;i<duration*channels;i++){

        data[i]=wavfile.unscaled_frames[i+j];
        
    }
    return data;
}


unsigned int find_loop_end (short* start_sample, short* end_sample, int channels, int range) {
    unsigned int best_offset = 0;
    unsigned long best_score = ULONG_MAX;
    unsigned long score;
    unsigned int diff;
    unsigned int i;
    unsigned int j;

    for (i = 0; i < range; i++) {
        score = 0;
        for (j = 0; j < range * channels; j += 100) {
            diff = start_sample[j] - end_sample[i * channels + j];
            score += diff * diff;
        }

        if (!score) {
            return i;
        }
        else if (score < best_score) {
            best_score = score;
            best_offset = i;
        }
    }

    printf("Best score: %lu\n", best_score);
    printf("Best offset: %d\n", best_offset);
    return best_offset;
}

void copy_samples (short* src, short* dst, unsigned int iter) {
    unsigned int i;
    for (i = 0; i < iter; i++) {
        dst[i] = src[i];
    }
}

void extend_audio ();

int loop (const char* input_filepath, unsigned int start_time, unsigned int end_time,
          unsigned int min_length, const char* output_filepath) {
    /* Used to store some file metadata, required by sf_open() */
    SF_INFO info = {0,0,0,0,0,0};

    /* Open the audio file */
    SNDFILE* f = sf_open(input_filepath, SFM_READ, &info);

    /* Save a section of the audio at the start time for comparison */
    sf_count_t start = start_time * info.samplerate;
    sf_count_t start_frames = info.samplerate;
    short* start_sample = read_samples(f, info.channels, start, start_frames);
    if (start_sample == NULL) {
        printf("ERROR: %i is an invalid timestamp!\n", start_time);
        return 1;
    }

    /* Save a section of end time audio */
    sf_count_t end = end_time * info.samplerate;
    short* end_sample = read_samples(f, info.channels, end, 2 * info.samplerate);
    if (end_sample == NULL) {
        printf("ERROR: %i is an invalid timestamp!\n", end_time);
        return 1;
    }

    /* Find looping point */
    unsigned int best_offset = find_loop_end(start_sample, end_sample, info.channels, start_frames);

    /* Store the data for a loop */
    sf_count_t loop_frames = end + best_offset - start;
    short* loop = read_samples(f, info.channels, start, loop_frames);

    /* Store the data for the intro */
    short* intro = read_samples(f, info.channels, 0, start);

    /* Store the data for the ending */
    sf_count_t ending_frames = info.frames - end - best_offset;
    short* ending = read_samples(f, info.channels, end + best_offset, ending_frames);

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
    copy_samples(intro, extended, start * info.channels);
    j += start * info.channels;

    /* Copy loops */
    for (int k = 0; k < num_loops; k++) {
        copy_samples(loop, extended + j, loop_frames * info.channels);
        j += loop_frames * info.channels;
    }

    /* Copy ending */
    copy_samples(ending, extended + j, ending_frames * info.channels);

    /* Write buffer into new file */
    SF_INFO output_info = {0, info.samplerate, info.channels, info.format, 0, 0};
    SNDFILE* fout = sf_open(output_filepath, SFM_WRITE, &output_info);
    sf_writef_short(fout, extended, total_len);

    /* Clean up */
    free(start_sample);
    free(end_sample);
    free(intro);
    free(loop);
    free(ending);
    free(extended);
    sf_close(f);
    sf_close(fout);
    return 0;
}

int check_frames(short * frames1, short* frames2, int channels,sf_count_t limit){
    for(int i=0;i<limit*channels;i++){
        if(frames1[i]!=frames2[i]){
            printf("CHECK WRONG: %d %d %d\n",i,frames1[i], frames2[i]);
            return 0;
        }
    }
    return 1;

}

// SAME AS LOOP FUNCTION ABOVE, diff is the below one returns a WavFile

WavFile loop_frames (const char* input_filepath, unsigned int start_time, unsigned int end_time,
          unsigned int min_length, const char* output_filepath, WavFile wavfile) {
    /* Used to store some file metadata, required by sf_open() */
    // SF_INFO info = {0,0,0,0,0,0};

    /* Open the audio file */
    // SNDFILE* f = sf_open(input_filepath, SFM_READ, &info);

    /* Save a section of the audio at the start time for comparison */
    sf_count_t start = start_time * wavfile.headers.sample_rate;
    sf_count_t start_frames = wavfile.headers.sample_rate;
    // short* start_sample = read_samples(f, info.channels, start, start_frames);
    short * start_sample2=read_samples2(wavfile.headers.num_channels,start,start_frames,wavfile);
    // int check = check_frames(start_sample,start_sample2,wavfile.headers.num_channels,start_frames);
    // if(check){
    //     printf("CHECK 1 IS OK\n");
    // }
    // if (start_sample == NULL) {
    //     printf("ERROR: %i is an invalid timestamp!\n", start_time);
    //     return wavfile;
    // }

    /* Save a section of end time audio */
    sf_count_t end = end_time * wavfile.headers.sample_rate;
    // short* end_sample = read_samples(f, info.channels, end, 2 * info.samplerate);
    short* end_sample2 = read_samples2(wavfile.headers.num_channels, end, 2 * wavfile.headers.sample_rate,wavfile);
    // int check2 = check_frames(end_sample,end_sample2,info.channels, 2 * info.samplerate);
    // if(check2){
    //     printf("CHECK 2 IS OK\n");
    // }
    // if (end_sample == NULL) {
    //     printf("ERROR: %i is an invalid timestamp!\n", end_time);
    //     return wavfile;
    // }

    /* Find looping point */
    // unsigned int best_offset = find_loop_end(start_sample, end_sample, info.channels, start_frames);
    unsigned int best_offset2 = find_loop_end(start_sample2, end_sample2, wavfile.headers.num_channels, start_frames);
    // if(best_offset!=best_offset2){
    //     printf("OFFSET: %d %d\n",best_offset,best_offset2);
    // }
    /* Store the data for a loop */
    sf_count_t loop_frames = end + best_offset2 - start;
    // short* loop = read_samples(f, info.channels, start, loop_frames);
    short* loop2 = read_samples2(wavfile.headers.num_channels,start,loop_frames,wavfile);
    // int check3 = check_frames(loop,loop2,wavfile.headers.num_channels,loop_frames);
    // if(check3){
    //     printf("CHECK 3 IS OK\n");
    // }
    /* Store the data for the intro */
    // short* intro = read_samples(f, info.channels, 0, start);
    short* intro2 = read_samples2(wavfile.headers.num_channels,0,start,wavfile);
    // int check4 = check_frames(intro,intro2,wavfile.headers.num_channels,start);
    // if(check4){
    //     printf("CHECK 4 IS OK\n");
    // }

    /* Store the data for the ending */

    sf_count_t ending_frames = wavfile.num_frames/2 - end - best_offset2;
    // short* ending = read_samples(f, info.channels, end + best_offset, ending_frames);
    short* ending2 = read_samples2(wavfile.headers.num_channels,end+best_offset2,ending_frames,wavfile);
    // int check5 = check_frames(ending,ending2,wavfile.headers.num_channels,ending_frames);
    // if(check5){
    //     printf("CHECK 5 IS OK\n");
    // }
    

    /* Compute number of loops */
    int num_loops = (min_length * wavfile.headers.sample_rate - start - ending_frames) / loop_frames + 1;
    printf("%d\n", num_loops);

    /* Create a buffer for the extended audio */
    sf_count_t total_len = start + loop_frames * num_loops + ending_frames;
    // short* extended = (short*) malloc(total_len * info.channels * sizeof(short));
    short* extended2 = (short*) malloc(total_len * wavfile.headers.num_channels * sizeof(short));
    printf("total_len: %lu\n", total_len);
    printf("frames: %lu\n", wavfile.num_frames/2);
    printf("loop frames: %lu\n", loop_frames);
    printf("ending frames: %lu\n", ending_frames);

    /* Copy intro */
    int j = 0;
    // copy_samples(intro, extended, start * info.channels);
    copy_samples(intro2, extended2, start * wavfile.headers.num_channels);
    j += start * wavfile.headers.num_channels;

    /* Copy loops */
    for (int k = 0; k < num_loops; k++) {
        // copy_samples(loop, extended + j, loop_frames * info.channels);
        copy_samples(loop2, extended2 + j, loop_frames * wavfile.headers.num_channels);
        j += loop_frames * wavfile.headers.num_channels;
    }

    /* Copy ending */
    // copy_samples(ending, extended + j, ending_frames * info.channels);
    copy_samples(ending2, extended2 + j, ending_frames * wavfile.headers.num_channels);

    // int check6 = check_frames(extended,extended2,wavfile.headers.num_channels,total_len);
    // if(check6){
    //     printf("CHECK 6 IS OK\n");
    // }

    WavFile loop_file=wavfile;
    loop_file.unscaled_frames=extended2;
    loop_file.num_frames=total_len*wavfile.headers.num_channels;
    loop_file.headers.data_chunk_size=loop_file.num_frames*2;
    long file_size=(
        4+
        4+
        4+
        4+
        4+
        loop_file.headers.sub_chunk1_size+
        4+
        4+
        loop_file.headers.sub_chunk2_size+
        4+
        4+
        loop_file.headers.data_chunk_size
    );
    loop_file.headers.chunk_size=file_size;

    /* Write buffer into new file */
    // SF_INFO output_info = {0, info.samplerate, info.channels, info.format, 0, 0};
    // SNDFILE* fout = sf_open(output_filepath, SFM_WRITE, &output_info);
    // sf_writef_short(fout, extended, total_len);


    /* Clean up */
    // free(start_sample);
   
    // free(end_sample);
    // free(intro);
    // free(loop);
    // free(ending);
    // free(extended);
    // sf_close(f);
    // sf_close(fout);
    free(start_sample2);
    free(end_sample2);
    free(intro2);
    free(loop2);
    free(ending2);

    return loop_file;
}


int main (int argc, char** argv) {
    // if (argc < 6) {
    //     /* TODO Print args required */
    //     printf("Insufficient number of arguments!\n");
    //     return 1;
    // }

    /* TODO Perform checks on input */
    int i;
    
    int res = loop("recycling.wav", 60,80 , 120, OUTPUT);
    /*

    int res = loop("INTERCEPTION_stereo_hard.flac", 9, 179, 1800, OUTPUT);
    int res = loop("02 SEGMENT1-2 BOSS.flac", 0, 98, 600, OUTPUT);
    int res = loop("Let Ass Kick Together!.flac", 1, 68, 600, OUTPUT);
    int res = loop("23 なにみてはねる.flac", 1, 80, 1800, OUTPUT);
    int res = loop("15 Expert Course Stage 1.flac", 14, 42, 1800, OUTPUT);
    int res = loop("2-06 Boss.flac", 7, 109, 1800, OUTPUT);
    */
    // int res = loop(argv[1], argv[2], argv[3], argv[4]);
    FILE *fp = fopen("recycling.wav", "r");
    WavFile file=read_frames(fp);
    WavFile loop_file=loop_frames("recycling.wav", 60,80 , 120, OUTPUT,file);
    FILE * file_p = fopen("./write2.wav", "w");
    if(NULL == file_p)
        {
            printf("fopen failed in main");
            return 0;
        }
    write_wav(file_p,loop_file);
    fclose(file_p);
    // for(i=100000;i<100200;i++){
    //     printf("%d\n",frames.frames[i]);
    // }

    
    
    // printf("num_frames:%ld",file.num_frames);
    // for(int i=100000;i<100200;i++){
    //     printf("%d\n",file.unscaled_frames[i]);
    // }
    if (res) {
        return 1;
    }

    return 0;
}
