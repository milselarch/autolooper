typedef struct sound_buffer {
    short* data;
    sf_count_t size;
} sndbuf;

int read_samples (SNDFILE* f, sndbuf* buf, int channels, sf_count_t offset, sf_count_t duration);

sf_count_t find_loop_end (sndbuf* start_buf, sndbuf* end_buf, int channels);

void copy_samples (sndbuf* src_buf, short* dst);

void extend_audio (sndbuf* extended_buf, sndbuf* intro_buf, sndbuf* loop_buf, sndbuf* ending_buf, unsigned int num_loops);

int loop (const char* input_filepath, unsigned int start_time, unsigned int end_time,
          unsigned int min_length, const char* output_filepath);
