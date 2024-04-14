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

typedef struct {
    double ** samples;
    unsigned long num_frames;
    unsigned long sample_rate;
    unsigned long num_channels;
    unsigned long num_samples;
} WavParseResult;

void free_wav_headers (WavHeaders headers);

void free_wav_file (WavFile wav_file);

void free_wav_parse_result (WavParseResult wav_parse_result);

int starts_with_word (const char * text, const char * word);

unsigned long byte_str_to_long (
    char * string, int is_little_endian, unsigned long length
);

int is_str_equal (const char * string1, const char * string2);

char * read_str_slice (
    FILE *fp, unsigned long start_index, unsigned long end_index
);

unsigned long read_long_from_str_slice (
    FILE *fp, unsigned long start_index, unsigned long end_index,
    int is_little_endian
);

char * slice_str (const char * source_str, size_t start, size_t end);

void print_wav_headers (WavHeaders headers);

WavHeaders read_wav_headers (FILE * fp);

long get_max_int (unsigned int bits);

WavFile read_frames (FILE * fp);

WavParseResult read_wav_file (const char * filepath);

void write_wav (FILE * fp, WavFile file);


