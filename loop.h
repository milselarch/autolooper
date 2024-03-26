typedef struct sound_buffer {
    short* data;
    unsigned long size;
} sndbuf;

int read_samples (WavFile* wavfile, sndbuf* buf, int channels, unsigned long offset, unsigned long duration);

unsigned long find_loop_end (sndbuf* start_buf, sndbuf* end_buf, int channels);

void copy_samples (sndbuf* src_buf, short* dst);

void extend_audio (sndbuf* extended_buf, sndbuf* intro_buf, sndbuf* loop_buf, sndbuf* ending_buf, unsigned int num_loops);

int loop (WavFile* f, unsigned int start_time, unsigned int end_time, unsigned int min_length, WavFile* fout);


unsigned long find_difference(short* start_buf, short* end_buf, int window_size);
int find_loop_points_auto(sndbuf* buf, unsigned int* start_time_buf, unsigned int* end_time_buf, int num_channels, int sample_rate, unsigned long window_size);
int find_loop_points_auto_offsets(sndbuf* buf, unsigned long* start_offset_buf, unsigned long* end_offset_buf, int num_channels, int sample_rate, unsigned long window_size);
int loop_with_offsets (WavFile* f, unsigned long start_offset, unsigned long end_offset, unsigned int min_length, WavFile* fout);