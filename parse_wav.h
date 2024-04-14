typedef struct {
    char * chunk_id; 
    long chunk_size;
    char * format; 
    /* long filesize; */

    char * sub_chunk_id; 
    long sub_chunk1_size;
    long audio_format; 
    long num_channels; 
    long sample_rate; 
    long byte_rate; 
    long block_align;
    /*
    number of bits needed to represent
    each floating point number amplitude sample
     */
    long bits_per_sample;
    long extra_params_size;
    /* contains data for all sub-chunks between "fmt " and "data" sub-chunks */
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


