#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
// #include <math.h>

const int DEBUG_INDEX = 31910;

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

typedef struct {
    double ** samples;
    unsigned long num_frames;
    unsigned long sample_rate;
    unsigned long num_channels;
    unsigned long num_samples;
} WavParseResult;

void free_wav_headers(WavHeaders headers) {
    free(headers.chunk_id);
    free(headers.format);
    free(headers.sub_chunk_id);
    free(headers.extra_params);
}

void free_wav_file(WavFile wav_file) {
    free_wav_headers(wav_file.headers);
    free(wav_file.frames);
}

void free_wav_parse_result(WavParseResult wav_parse_result) {
    for (int k=0; k<wav_parse_result.num_channels; k++) {
        free(wav_parse_result.samples[k]);
    }

    free(wav_parse_result.samples);
}

int starts_with_word(const char * text, const char * word) {
    // checks if the text starts with word
    int k = 0;

    while (1) {
        char text_char = text[k];
        char word_char = word[k];

        if (word_char == '\0') {
            // word has ended, all previous chars match
            return 1;
        } if (text_char == '\0') {
            // text has ended, word still has chars left
            return 0;
        } if (text_char != word_char) {
            // current text char and current word char mismatch
            return 0;
        }

        k++;
    }

    // the code should probably never reach here
    // return 0;
}

unsigned long byte_str_to_long(
    char * string, int is_little_endian, unsigned long length
) {
    if (length == -1) {
        length = strlen(string);
    }

    unsigned long result = 0;
    if (length > 4) {
        /*
        longs are guaranteed to be at least 32 bits long,
        so byte strings with more than 4 chars might overflow
        https://en.wikipedia.org/wiki/C_data_types
        */
        printf("BYTE_STR_TOO_LONG");
        exit(1);
    }

    for (int k=0; k<length; k++) {
        unsigned long current_char;
        // char values must be converted to unsigned
        // else they might be treated as negative in conversion
        if (is_little_endian) {
            current_char = (unsigned char) string[k];
        } else {
            current_char = (unsigned char) string[length-k-1];
        }

        result |= current_char << (8 * k);
    }
    return result;
}

int is_str_equal(const char * string1, const char * string2) {
    // checks if the text starts with word
    unsigned long length = strlen(string1);
    // printf("LENLEN %ld\n", length);

    for (unsigned long k=0; k<length; k++) {
        // printf("CHAR_CMP[%ld]: %c-%c\n", k, string1[k], string2[k]);

        if (string1[k] != string2[k]) {
            return 0;
        }

        if ((string1[k] == 0) && (string2[k] == 0)) {
            return 1;
        }
    }

    return 1;
}

char * read_str_slice(
    FILE *fp, unsigned long start_index, unsigned long end_index
) {
    if (end_index < start_index) {
        printf("END_INDEX IS LESS THAN START_INDEX");
        exit(1);
    }

    unsigned long str_size = end_index - start_index + 1;
    char * str_slice = (char *) malloc(str_size * sizeof(char));
    str_slice[str_size-1] = 0;

    for (unsigned long k=start_index; k<end_index; k++) {
        fseek(fp, (long) k, SEEK_SET);
        long str_slice_index = (long) k - (long) start_index;
        int fget_result = fgetc(fp);
        if (fget_result == EOF) {
            printf("END_OF_FILE REACHED");
            exit(1);
        }
        str_slice[str_slice_index] = (char) fget_result;
    }

    // fseek(fp, 0L, SEEK_SET);
    return str_slice;
}

unsigned long read_long_from_str_slice(
    FILE *fp, unsigned long start_index, unsigned long end_index,
    int is_little_endian
) {
    unsigned long length = end_index - start_index;
    char * raw_str_slice = read_str_slice(fp, start_index, end_index);
    if (start_index == DEBUG_INDEX) {
        for (int k=0; k<length; k++) {
            printf("CHAR[%d]: %d\n", (int) start_index + k, (unsigned char) raw_str_slice[k]);
        }
    }

    unsigned long value = byte_str_to_long(raw_str_slice, is_little_endian, length);
    free(raw_str_slice);
    return value;
}

// slice a substring from a source string
char * slice_str(const char * source_str, size_t start, size_t end) {
    if (end < start) {
        printf("END CANNOT BE BEFORE START");
        exit(1);
    }

    size_t length = end - start + 1;
    char * dest_str = (char *) malloc(length * sizeof(char));
    dest_str[length - 1] = 0;

    // https://stackoverflow.com/questions/26620388/
    strncpy(dest_str, source_str + start, end - start);
    return dest_str;
}

void print_wav_headers(WavHeaders headers) {
    printf("---- WAV FILE HEADERS ----\n");
    printf("CHUNK_ID: %s\n", headers.chunk_id);
    printf("CHUNK_SIZE: %ld\n", headers.chunk_size);
    printf("FORMAT: %s\n", headers.format);
    printf("SUB_CHUNK_ID: %s\n", headers.sub_chunk_id);
    printf("SUB_CHUNK1_SIZE: %ld\n", headers.sub_chunk1_size);
    printf("AUDIO_FORMAT: %ld\n", headers.audio_format);
    printf("NUM_CHANNELS: %ld\n", headers.num_channels);
    printf("SAMPLE_RATE: %ld\n", headers.sample_rate);
    printf("BYTE_RATE: %ld\n", headers.byte_rate);
    printf("BLOCK_ALIGN: %ld\n", headers.block_align);
    printf("BITS_PER_SAMPLE: %ld\n", headers.bits_per_sample);
    // printf("EXTRA_PARAMS_SIZE: %ld\n", headers.extra_params_size);
    printf("EXTRA_PARAMS: %s\n", headers.extra_params);
    printf("SUB_CHUNK2_SIZE: %ld\n", headers.sub_chunk2_size);
    printf("DATA_HEADER: %s\n", headers.data_header);
    printf("DATA_CHUNK_SIZE: %ld\n", headers.data_chunk_size);
    printf("---- WAV FILE HEADERS END ----\n");
}

WavHeaders read_wav_headers(FILE * fp) {
    if (fp == NULL) {
        printf("FILE_OPEN_FAILED");
        exit(1);
    }

    // https://stackoverflow.com/questions/238603/
    fseek(fp, 0L, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    char * chunk_id = read_str_slice(fp, 0, 4);
    printf("CHUNK_START_READ: %s\n", chunk_id);
    if (!is_str_equal(chunk_id, "RIFF")) {
        printf("INVALID_FILE_HEADER");
        exit(1);
    }

    unsigned long chunk_size = read_long_from_str_slice(fp, 4, 8, 1);
    printf("chunk size: %ld",chunk_size);
    char * format_str = read_str_slice(fp, 8, 12);
    if (!is_str_equal(format_str, "WAVE")) {
        printf("WAV FORMAT IS NOT WAVE: %s\n", format_str);
        exit(1);
    }

    char * sub_chunk_id = read_str_slice(fp, 12, 16);
    printf("sub chunk id: %s\n", sub_chunk_id);
    // size in bytes of initial fmt chunk
    unsigned long sub_chunk1_size = read_long_from_str_slice(fp, 16, 20, 1);
    unsigned long format = read_long_from_str_slice(fp, 20, 22, 1);
    unsigned long num_channels = read_long_from_str_slice(fp,22, 24, 1);
    unsigned long sample_rate = read_long_from_str_slice(fp, 24, 28, 1);
    unsigned long byte_rate = read_long_from_str_slice(fp, 28, 32, 1);
    unsigned long block_align = read_long_from_str_slice(fp, 32, 34, 1);
    unsigned long bits_per_sample = read_long_from_str_slice(fp, 34, 36, 1);

    unsigned long extra_params_size = 0;
    char * extra_params = 0;

    if (sub_chunk1_size != 16) {
        extra_params_size = read_long_from_str_slice(fp, 36, 38, 1);
        extra_params = read_str_slice(fp, 38, 38 + extra_params_size);
        printf("extra params: %s\n",extra_params);  
    }
    extra_params = read_str_slice(fp, 36, 40);
    printf("extra params: %s\n",read_str_slice(fp, 36, 40)); 

    // size in bytes of LIST chunk
    unsigned long sub_chunk2_size = read_long_from_str_slice(fp, 40, 44, 1);
    char * list_chunk = read_str_slice(fp, 44, 44+sub_chunk2_size); 
    printf("list chunk: %s\n",list_chunk); 
    unsigned long header_size = (
        4 + // for RIFF initial chunk header
        4 + // overall chunk size info
        4 + // extra 4 bytes of space to store WAVE (to declare file is wav?)
        4 + // fmt sub chunk header
        4 + // fmt sub chunk size info
        sub_chunk1_size + // WAVE sub chunk size
        4 + // LIST sub chunk header
        4 + // LIST sub chunk info size
        sub_chunk2_size // LIST sub chunk size
    );
    printf("HEADER_SIZE: %ld\n", header_size);

    char * data_header = read_str_slice(fp, header_size, header_size+4);
    int data_header_is_valid = starts_with_word(data_header, "data");
    printf("DATA_HEADER: %s\n", data_header);
    // free(data_header);

    if (!data_header_is_valid) {
        printf("DATA HEADER IS INVALID");
        exit(1);
    }

    unsigned long data_chunk_size = read_long_from_str_slice(
        fp, header_size+4, header_size+8, 1
    );

    WavHeaders headers;
    headers.chunk_id = chunk_id;
    headers.chunk_size = (int) chunk_size;
    headers.format = format_str;
    // headers.filesize = filesize;

    headers.sub_chunk_id = sub_chunk_id;
    headers.sub_chunk1_size = (int) sub_chunk1_size;
    headers.audio_format = (int) format;
    headers.num_channels = (int) num_channels;
    headers.sample_rate = (long) sample_rate;
    headers.byte_rate = (int) byte_rate;
    headers.block_align = (int) block_align;
    headers.bits_per_sample = (int) bits_per_sample;
    // headers.extra_params_size = (int) extra_params_size;
    headers.extra_params = extra_params;
    headers.sub_chunk2_size = (int) sub_chunk2_size;
    headers.list_chunk_data = list_chunk;
    headers.header_size=header_size;
    headers.data_header = data_header; //(int) header_size;
    headers.data_chunk_size = (int) data_chunk_size;
    print_wav_headers(headers);
    printf("%ld\n",data_chunk_size);
    return headers;
}

WavHeaders read_wav_headers2(FILE * fp) {
    if (fp == NULL) {
        printf("FILE_OPEN_FAILED");
        exit(1);
    }

    // https://stackoverflow.com/questions/238603/
    fseek(fp, 0L, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    char * chunk_id = read_str_slice(fp, 0, 4);
    printf("CHUNK_START_READ: %s\n", chunk_id);
    if (!is_str_equal(chunk_id, "RIFF")) {
        printf("INVALID_FILE_HEADER");
        exit(1);
    }

    unsigned long chunk_size = read_long_from_str_slice(fp, 4, 8, 1);
    printf("chunk size: %ld",chunk_size);
    char * format_str = read_str_slice(fp, 8, 12);
    if (!is_str_equal(format_str, "WAVE")) {
        printf("WAV FORMAT IS NOT WAVE: %s\n", format_str);
        exit(1);
    }

    char * sub_chunk_id = read_str_slice(fp, 12, 16);
    printf("sub chunk id: %s\n", sub_chunk_id);
    // size in bytes of initial fmt chunk
    unsigned long sub_chunk1_size = read_long_from_str_slice(fp, 16, 20, 1);
    unsigned long format = read_long_from_str_slice(fp, 20, 22, 1);
    unsigned long num_channels = read_long_from_str_slice(fp,22, 24, 1);
    unsigned long sample_rate = read_long_from_str_slice(fp, 24, 28, 1);
    unsigned long byte_rate = read_long_from_str_slice(fp, 29, 32, 1);
    unsigned long block_align = read_long_from_str_slice(fp, 33, 34, 1);
    unsigned long bits_per_sample = read_long_from_str_slice(fp, 34, 36, 1);

    unsigned long extra_params_size = 0;
    char * extra_params = 0;

    // if (sub_chunk1_size != 16) {
    //     extra_params_size = read_long_from_str_slice(fp, 36, 38, 1);
    //     extra_params = read_str_slice(fp, 38, 38 + extra_params_size);
    //     printf("extra params: %s\n",extra_params);  
    // }
    // extra_params = read_str_slice(fp, 36, 40);
    // printf("extra params: %s\n",read_str_slice(fp, 36, 40)); 

    // size in bytes of LIST chunk
    // unsigned long sub_chunk2_size = read_long_from_str_slice(fp, 40, 44, 1);
    // char * list_chunk = read_str_slice(fp, 44, 44+sub_chunk2_size); 
    // printf("list chunk: %s\n",list_chunk); 
    unsigned long header_size = (
        4 + // for RIFF initial chunk header
        4 + // overall chunk size info
        4 + // extra 4 bytes of space to store WAVE (to declare file is wav?)
        4 + // fmt sub chunk header
        4 + // fmt sub chunk size info
        sub_chunk1_size  // WAVE sub chunk size
        // 4 + // LIST sub chunk header
        // 4  // LIST sub chunk info size
        // sub_chunk2_size // LIST sub chunk size
    );
    printf("HEADER_SIZE: %ld\n", header_size);

    char * data_header = read_str_slice(fp, header_size, header_size+100);
    int data_header_is_valid = starts_with_word(data_header, "data");
    printf("DATA_HEADER: %s\n", data_header);
    // free(data_header);

    if (!data_header_is_valid) {
        printf("DATA HEADER IS INVALID");
        exit(1);
    }

    unsigned long data_chunk_size = read_long_from_str_slice(
        fp, header_size+4, header_size+8, 1
    );

    WavHeaders headers;
    headers.chunk_id = chunk_id;
    headers.chunk_size = (int) chunk_size;
    headers.format = format_str;
    // headers.filesize = filesize;

    headers.sub_chunk_id = sub_chunk_id;
    headers.sub_chunk1_size = (int) sub_chunk1_size;
    headers.audio_format = (int) format;
    headers.num_channels = (int) num_channels;
    headers.sample_rate = (long) sample_rate;
    headers.byte_rate = (int) byte_rate;
    headers.block_align = (int) block_align;
    headers.bits_per_sample = (int) bits_per_sample;
    // headers.extra_params_size = (int) extra_params_size;
    headers.extra_params = extra_params;
    // headers.sub_chunk2_size = (int) sub_chunk2_size;
    // headers.list_chunk_data = list_chunk;
    headers.header_size=header_size;
    headers.data_header = data_header; //(int) header_size;
    headers.data_chunk_size = (int) data_chunk_size;
    print_wav_headers(headers);
    printf("%ld\n",data_chunk_size);
    return headers;
}


long long get_max_int(unsigned int bits) {
    // get maximum positive integer with size bits
    long long result = 1;
    for (int k=1; k<bits; k++) {
        result *= 2;
    }

    result--;
    return result;
}

WavFile read_frames(FILE * fp) {
    WavHeaders headers = read_wav_headers(fp);
    // char * raw_audio_data = read_str_slice(fp, start_index + 8, headers.filesize);
    int sample_size = (int) headers.bits_per_sample / 8;
    // long long is at least 64 bits
    long long max_signed_int_val;

    switch (headers.bits_per_sample) {
        case 8:
            max_signed_int_val = get_max_int(8);
            break;
        case 16:
            max_signed_int_val = get_max_int(16);
            break;
        case 32:
            max_signed_int_val = get_max_int(32);
            break;
        case 64:
            max_signed_int_val = get_max_int(64);
            break;
        default:
            printf("INVALID_BITS_PER_SAMPLE");
            exit(1);
    }
    
    printf("MAX_INT_VAL: %lld\n", max_signed_int_val);
    printf("RAW_DATA_START_IDX: %d\n", 70);
    printf("RAW_DATA_CHUNK_SIZE: %ld\n", headers.chunk_size);

    // printf("FILE_SIZE: %ld\n", headers.filesize);
    unsigned long num_samples = headers.data_chunk_size / sample_size;
    printf("NUM_SAMPLES %ld\n", num_samples);

    double * frames = (double *) malloc((num_samples + 1) * sizeof(double));
    short * unscaled_frames = (short*) malloc((num_samples + 1) * sizeof(short));
    // the 8 is for the data chunk name ("data")
    // and the 4 bytes for data chunk size
    const unsigned long data_start_idx = headers.header_size + 8;
    frames[num_samples] = 0;
    unscaled_frames[num_samples]=0;
    /*
    // scan for junk chunks
     char * raw_audio_data = read_str_slice(fp, raw_audio_start_index, headers.filesize-1);
    for (int k=0; k<raw_data_length-4; k++) {
        if (starts_with_word(raw_audio_data + k, "JUNK")) {
            printf("JUNK FOUND\n");
            exit(1);
        }
    }
    */

    for (int k=0; k<num_samples; k++) {
        /*
        if (k % 10000 == 0) {
            printf("SAMPLE %d/%ld\n", k, num_samples);
        }
        */
        unsigned long slice_start_idx = data_start_idx + k * sample_size;
        unsigned long slice_end_idx = data_start_idx + (k + 1) * sample_size;
        if (k == 0) {
            printf("SLICE_START: %ld\n", slice_start_idx);
        }
        short unscaled_frame = (short) read_long_from_str_slice(
            fp, slice_start_idx, slice_end_idx, 1
        );
        double scaled_frame = (
            ((double) unscaled_frame) / ((double) max_signed_int_val)
        );

        if (scaled_frame > 1) {
            // value has under flowed due to being negative
            scaled_frame -= 2;
        }

        if (slice_start_idx == DEBUG_INDEX) {
            printf("DOUBLE %f, %d\n", scaled_frame, k);
        }

        frames[k] = scaled_frame;
        unscaled_frames[k]=unscaled_frame;
    }

    printf("SLICE_END\n");
    // free(raw_audio_data);
    // free_wav_headers(headers);
    WavFile wav_file;
    wav_file.scale=max_signed_int_val;
    wav_file.headers = headers;
    wav_file.frames = frames;
    wav_file.num_frames = num_samples;
    wav_file.unscaled_frames=unscaled_frames;
    return wav_file;
}

WavParseResult read_wav_file(const char * filepath) {
    FILE *fp = fopen(filepath, "r");
    WavFile read_result = read_frames(fp);
    printf("READ_FRAMES_COMPLETE\n");

    WavParseResult wav_parse_result;
    long num_channels = read_result.headers.num_channels;
    double ** samples = (double **) malloc(
        (num_channels + 1) * sizeof(double *)
    );

    samples[num_channels] = 0;
    unsigned long channel_length = read_result.num_frames / num_channels;
    printf("CHANNEL_LENGTH: %ld\n", channel_length);
    printf("NUM_CHANNELS: %ld\n", num_channels);

    for (int k = 0; k < num_channels; k++) {
        double * channel = (double *) malloc(
            (channel_length + 1) * sizeof(double)
        );

        channel[channel_length] = 0;
        samples[k] = channel;
    }

    printf("PRE_FRAME_REASSIGN\n");
    for (int k=0; k < read_result.num_frames; k++) {
        unsigned long step_idx = k / num_channels;
        unsigned long channel_idx = k % num_channels;
        /*
        if ((k == 0) || (k % 10000 == 0)) {
            printf("ASSIGN %ld-%ld-%d\n", step_idx, channel_idx, k);
        }
         */
        samples[channel_idx][step_idx] = read_result.frames[k];
    }

    wav_parse_result.num_samples = channel_length;
    wav_parse_result.num_channels = num_channels;
    wav_parse_result.samples = samples;
    wav_parse_result.sample_rate = read_result.headers.sample_rate;
    wav_parse_result.num_frames = read_result.num_frames;
    free_wav_file(read_result);
    return wav_parse_result;
}

void write_wav(FILE * fp, WavFile file){

    //// WAVE Header Data
    fwrite(file.headers.chunk_id, 1, 4, fp);
    fwrite(&file.headers.chunk_size, 4, 1, fp);
    fwrite(file.headers.format, 1, 4, fp);
    fwrite(file.headers.sub_chunk_id, 1, 4, fp);
    fwrite(&file.headers.sub_chunk1_size, 4, 1, fp);
    fwrite(&file.headers.audio_format, 2, 1, fp);
    fwrite(&file.headers.num_channels, 2, 1, fp);
    fwrite(&file.headers.sample_rate, 4, 1, fp);
    fwrite(&file.headers.byte_rate, 4, 1, fp);
    fwrite(&file.headers.block_align, 2, 1, fp);
    fwrite(&file.headers.bits_per_sample, 2, 1, fp);
    fwrite(file.headers.extra_params, 1, 4, fp);
    fwrite(&file.headers.sub_chunk2_size, 4, 1, fp);
    fwrite(file.headers.list_chunk_data, 1,file.headers.sub_chunk2_size , fp);


    // Marks the start of the data
    fwrite(file.headers.data_header, 1, 4, fp);
    fwrite(&file.headers.data_chunk_size, 4, 1, fp); 
    fwrite(file.unscaled_frames,2,file.num_frames,fp); // Data size
    // for (int i = 0; i < file.num_frames; i++)
    // {   
    //     fwrite(&file.unscaled_frames[i] , sizeof(int16_t), 1, fp);
    // }


    
}

// int main(int argc, char ** argv){
//     // WavParseResult result= read_wav_file("recycling.wav");
//     // FILE *fp = fopen("write.wav", "r");
//     // WavHeaders headers=read_wav_headers(fp);
//     FILE *fp = fopen("recycling.wav", "r");
//     WavFile file=read_frames(fp);
    
//     printf("num_frames:%ld",file.num_frames);
//     for(int i=100000;i<100200;i++){
//         printf("%d\n",file.unscaled_frames[i]);
//     }

//     FILE * file_p = fopen("./write1.wav", "w");
//     if(NULL == file_p)
//         {
//             printf("fopen failed in main");
//             return 0;
//         }
        

//     // size_t write_count = fwrite(  &file.headers, 
//     //                         78, 1,
//     //                         file_p);
//     // printf("write count: %ld\n", sizeof(WavHeaders));

//     // write_count = fwrite(  &file.frames, 
//     //                         2, file.num_frames,
//     //                         file_p);
                    
//     // printf("write count: %ld\n", write_count);
//     write_wav(file_p,file);
//     fclose(fp);
//     fclose(file_p);

//     return 0;
// }