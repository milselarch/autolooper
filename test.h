const char* OUTPUT = "output.flac";

short* read_samples (SNDFILE* f, int channels, sf_count_t offset, sf_count_t duration);

unsigned int find_loop_end (short* start_sample, short* end_sample, int channels, int range);

void copy_samples (short* src, short* dst, unsigned int iter);
