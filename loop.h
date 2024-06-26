/**
 * Buffer for audio samples with size tracking
 */
typedef struct sound_buffer {
    short* data;
    unsigned long size;
} sndbuf;

int read_samples (WavFile* wavfile, sndbuf* buf, int channels, unsigned long offset, unsigned long duration);

unsigned long find_loop_end (sndbuf* start_buf, sndbuf* end_buf, int channels);

unsigned long find_loop_end_short_arr (short* start_buf, unsigned long start_buf_size, short* end_buf, unsigned long end_buf_size, int channels);

void copy_samples (sndbuf* src_buf, short* dst);

void crossfade_samples (sndbuf* src_buf, short* dst, sndbuf* crossfade_buf, unsigned long crossfade_dist);

void extend_audio (sndbuf* extended_buf, sndbuf* intro_buf, sndbuf* loop_buf, sndbuf* ending_buf, unsigned int num_loops);

int loop (WavFile* f, unsigned int start_time, unsigned int end_time, unsigned int min_length, WavFile* fout);
