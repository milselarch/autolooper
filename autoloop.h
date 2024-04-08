unsigned long find_difference(short* start_buf, short* end_buf, int window_size);

int find_loop_points_auto(sndbuf* buf, unsigned int* start_time_buf, unsigned int* end_time_buf, int num_channels, int sample_rate, unsigned long window_size);

int find_loop_points_auto_offsets(sndbuf* buf, unsigned long* start_offset_buf, unsigned long* end_offset_buf, int num_channels, int sample_rate, unsigned long window_size);

int loop_with_offsets (WavFile* f, unsigned long start_offset, unsigned long end_offset, unsigned int min_length, WavFile* fout);

int auto_loop (FILE* fp, FILE* fpout);
