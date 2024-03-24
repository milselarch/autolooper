typedef struct {
    char * chunk_id; //same
    long chunk_size;
    char * format; //same
    // long filesize;

    char * sub_chunk_id; //same
    long sub_chunk1_size;
    long audio_format; //same
    long num_channels; //same
    long sample_rate; //same
    long byte_rate; //same
    long block_align;
    // number of bits needed to represent
    // each floating point number amplitude sample
    long bits_per_sample;
    // long extra_params_size;
    char * extra_params;

    long sub_chunk2_size;
    char * list_chunk_data;
    char * data_header;
    long header_size;
    long data_chunk_size;
} WavHeaders;

typedef struct {
    WavHeaders headers;
    double * frames;
    unsigned long num_frames;
    double scale;
    short * unscaled_frames;
} WavFile;

WavFile read_frames(FILE * fp);

void write_wav(FILE * fp, WavFile file);


