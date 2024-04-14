#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parse_wav.h"

const int DEBUG_INDEX = 36;

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
    int k;
    for (k=0; k<wav_parse_result.num_channels; k++) {
        free(wav_parse_result.samples[k]);
    }

    free(wav_parse_result.samples);
}

int starts_with_word(const char * text, const char * word) {
    /* checks if the text starts with word */
    int k;
    k = 0;

    while (1) {
        char text_char = text[k];
        char word_char = word[k];

        if (word_char == '\0') {
            /* word has ended, all previous chars match */
            return 1;
        } if (text_char == '\0') {
            /* text has ended, word still has chars left */
            return 0;
        } if (text_char != word_char) {
            /* current text char and current word char mismatch */
            return 0;
        }

        k++;
    }

    /*
    the code should probably never reach here
    return 0;
    */
}

unsigned long byte_str_to_long(
    char * string, int is_little_endian, unsigned long length
) {
    /*
     * converts a byte string to an unsigned long,
     * assuming that the byte string
     * :param: is_little_endian
     * whether MSB is in the last char in the byte string
     * note that it seems that little endian is always
     * used for representing audio sample values
     */
    unsigned long result;
    int k;

    result = 0;
    if (length == -1) {
        length = strlen(string);
    }

    if (length > 4) {
        /*
        longs are guaranteed to be at least 32 bits long,
        so byte strings with more than 4 chars might overflow
        https://en.wikipedia.org/wiki/C_data_types
        */
        printf("BYTE_STR_TOO_LONG");
        exit(1);
    }

    for (k=0; k<length; k++) {
        unsigned long current_char;
        /*
        char values must be converted to unsigned
        else they might be treated as negative in conversion
        */
        if (is_little_endian) {
            current_char = (unsigned char) string[k];
        } else {
            current_char = (unsigned char) string[length-k-1];
        }

        /*
         * shift the current char by k bytes (8*k bits)
         * and add it to result
        */
        result |= current_char << (8 * k);
    }
    return result;
}

int is_str_equal(const char * string1, const char * string2) {
    /* checks if the text starts with word */
    unsigned long length = strlen(string1);
    /* printf("LENLEN %ld\n", length); */

    unsigned long k;
    for (k=0; k<length; k++) {
        /* printf("CHAR_CMP[%ld]: %c-%c\n", k, string1[k], string2[k]); */
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
    /* reads a string from a wav file stream */
    unsigned long str_size;
    unsigned long k;
    char * str_slice;

    if (end_index < start_index) {
        printf("END_INDEX IS LESS THAN START_INDEX");
        exit(1);
    }

    str_size = end_index - start_index + 1;
    str_slice = (char *) malloc(str_size * sizeof(char));
    str_slice[str_size-1] = 0;

    for (k=start_index; k<end_index; k++) {
        long str_slice_index;
        int fget_result;

        fseek(fp, (long) k, SEEK_SET);
        str_slice_index = (long) k - (long) start_index;
        fget_result = fgetc(fp);

        if (fget_result == EOF) {
            printf("END_OF_FILE REACHED");
            exit(1);
        }

        /* printf("DD %lu\n %c\n", k, (char) fget_result); */
        str_slice[str_slice_index] = (char) fget_result;
    }

    /* fseek(fp, 0L, SEEK_SET); */
    return str_slice;
}

unsigned long read_long_from_str_slice(
    FILE *fp, unsigned long start_index, unsigned long end_index,
    int is_little_endian
) {
    /* reads a long from the wav file stream */
    unsigned long length;
    char * raw_str_slice;
    unsigned long value;
    int k;

    length = end_index - start_index;
    raw_str_slice = read_str_slice(fp, start_index, end_index);

    if (start_index == DEBUG_INDEX) {
        for (k=0; k<length; k++) {
            printf(
                "CHAR[%d]: %d\n", (int) start_index + k,
                (unsigned char) raw_str_slice[k]
            );
        }
    }

    value = byte_str_to_long(
        raw_str_slice, is_little_endian, length
    );
    free(raw_str_slice);
    return value;
}

/* slice a substring from a source string */
char * slice_str(const char * source_str, size_t start, size_t end) {
    char * dest_str;
    size_t length;

    if (end < start) {
        printf("END CANNOT BE BEFORE START");
        exit(1);
    }

    length = end - start + 1;
    dest_str = (char *) malloc(length * sizeof(char));
    dest_str[length - 1] = 0;

    /* https://stackoverflow.com/questions/26620388/ */
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
    /* printf("EXTRA_PARAMS_SIZE: %ld\n", headers.extra_params_size); */
    printf("EXTRA_PARAMS: [%s]\n", headers.extra_params);
    printf("DATA_HEADER: %s\n", headers.data_header);
    printf("DATA_CHUNK_SIZE: %ld\n", headers.data_chunk_size);
    printf("---- WAV FILE HEADERS END ----\n");
}

WavHeaders read_wav_headers(FILE * fp) {
    /*
     * reads the header fields from the wav file
    */
    char * chunk_id;
    unsigned long chunk_size;
    char * sub_chunk_id;
    unsigned long sub_chunk1_size;
    unsigned long format;
    /* number of auto channels in the wav file */
    unsigned long num_channels;
    /* number of audio data samples per second of audio */
    unsigned long sample_rate;
    unsigned long byte_rate;
    unsigned long block_align;
    unsigned long bits_per_sample;
    unsigned long extra_params_size;
    unsigned long extra_params_index;
    char * extra_params;

    unsigned long header_size;
    char * data_header;
    int data_header_is_valid;
    unsigned long data_chunk_size;
    char * format_str;
    WavHeaders headers;

    if (fp == NULL) {
        printf("FILE_OPEN_FAILED");
        exit(1);
    }

    chunk_id = read_str_slice(fp, 0, 4);
    printf("CHUNK_START_READ: %s\n", chunk_id);
    if (!is_str_equal(chunk_id, "RIFF")) {
        printf("INVALID_FILE_HEADER");
        exit(1);
    }

    chunk_size = read_long_from_str_slice(fp, 4, 8, 1);
    printf("chunk size: %ld\n",chunk_size);
    format_str = read_str_slice(fp, 8, 12);
    if (!is_str_equal(format_str, "WAVE")) {
        printf("WAV FORMAT IS NOT WAVE: %s\n", format_str);
        exit(1);
    }

    sub_chunk_id = read_str_slice(fp, 12, 16);
    printf("sub chunk id: %s\n", sub_chunk_id);
    /* size in bytes of initial fmt chunk */
    sub_chunk1_size = read_long_from_str_slice(fp, 16, 20, 1);
    format = read_long_from_str_slice(fp, 20, 22, 1);
    num_channels = read_long_from_str_slice(fp,22, 24, 1);
    sample_rate = read_long_from_str_slice(fp, 24, 28, 1);
    byte_rate = read_long_from_str_slice(fp, 28, 32, 1);
    block_align = read_long_from_str_slice(fp, 32, 34, 1);
    bits_per_sample = read_long_from_str_slice(fp, 34, 36, 1);

    /*
     * retrieve size of all unnecessary chunks between the
     * "fmt " and "data" sub-chunks
    */
    /* length of data for all unnecessary chunks */
    extra_params_size = 0;
    /* data after "fmt " sub-chunk starts from index 36 */
    extra_params_index = 36;
    while (1) {
        char * header;
        int is_data_header;
        unsigned long sub_chunk_size;

        header = read_str_slice(fp, extra_params_index, extra_params_index+4);
        is_data_header = is_str_equal("data", header);
        /* printf("IDX %lu\n", extra_params_index); */
        /* printf("HEADER %s\n", header); */
        free(header);

        if (is_data_header) { break; }

        /* sub-chunk size starts 4 bytes from sub-chunk start, consumes 8 bytes */
        sub_chunk_size = read_long_from_str_slice(
            fp, extra_params_index+4, extra_params_index+8, 1
        );

        /* sub-chunk name + size info consumes 8 bytes */
        extra_params_size += 8 + sub_chunk_size;
        extra_params_index += 8 + sub_chunk_size;
    }

    extra_params = read_str_slice(fp, 36, extra_params_index);
    header_size = (
        4 + /* for RIFF initial chunk header */
        4 + /* overall chunk size info */
        4 + /* extra 4 bytes of space to store WAVE (to declare file is wav?) */
        4 + /* fmt sub chunk header */
        4 + /* fmt sub chunk size info */
        sub_chunk1_size + /* WAVE sub chunk size */
        extra_params_size
    );
    printf("HEADER_SIZE: %ld\n", header_size);

    data_header = read_str_slice(fp, header_size, header_size+4);
    data_header_is_valid = starts_with_word(data_header, "data");
    printf("DATA_HEADER: %s\n", data_header);
    /* free(data_header); */

    if (!data_header_is_valid) {
        printf("DATA HEADER IS INVALID");
        exit(1);
    }

    data_chunk_size = read_long_from_str_slice(
        fp, header_size+4, header_size+8, 1
    );

    headers.chunk_id = chunk_id;
    headers.chunk_size = (int) chunk_size;
    headers.format = format_str;
    /* headers.filesize = filesize; */

    headers.sub_chunk_id = sub_chunk_id;
    headers.sub_chunk1_size = (int) sub_chunk1_size;
    headers.audio_format = (int) format;
    headers.num_channels = (int) num_channels;
    headers.sample_rate = (long) sample_rate;
    headers.byte_rate = (int) byte_rate;
    headers.block_align = (int) block_align;
    headers.bits_per_sample = (int) bits_per_sample;
    /* headers.extra_params_size = (int) extra_params_size; */
    headers.extra_params = extra_params;
    headers.extra_params_size = (long) extra_params_size;
    headers.header_size = (long) header_size;
    headers.data_header = data_header; /* (int) header_size; */
    headers.data_chunk_size = (int) data_chunk_size;
    print_wav_headers(headers);
    printf("%ld\n",data_chunk_size);
    return headers;
}

long get_max_int(unsigned int bits) {
    /* get maximum positive integer with size bits */
    long result;
    int k;

    result = 1;

    for (k=1; k<bits; k++) {
        result *= 2;
    }

    result--;
    return result;
}

WavFile read_frames(FILE * fp) {
    /*
    * reads the wav file headers as well as
    * the raw audio data from the file
    */
    WavHeaders headers = read_wav_headers(fp);
    /*
    char * raw_audio_data = read_str_slice(fp, start_index + 8, headers.filesize);
    */
    /* long is at least 32 bits */
    long max_signed_int_val;
    int sample_size;
    unsigned long num_samples;
    double * frames;
    short * unscaled_frames;
    unsigned long data_start_idx;
    WavFile wav_file;
    int k;

    sample_size = (int) headers.bits_per_sample / 8;

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
        default:
            printf("INVALID_BITS_PER_SAMPLE");
            exit(1);
    }
    
    printf("MAX_INT_VAL: %ld\n", max_signed_int_val);
    printf("RAW_DATA_START_IDX: %d\n", 70);
    printf("RAW_DATA_CHUNK_SIZE: %ld\n", headers.chunk_size);

    /* printf("FILE_SIZE: %ld\n", headers.filesize); */
    num_samples = headers.data_chunk_size / sample_size;
    printf("NUM_SAMPLES %ld\n", num_samples);

    /* raw unscaled audio amplitude values */
    unscaled_frames = (short*) malloc((num_samples + 1) * sizeof(short));
    /* audio amplitude values scaled from -1 to 1 */
    frames = (double *) malloc((num_samples + 1) * sizeof(double));

    /*
    the 8 is for the data chunk name ("data")
    and the 4 bytes for data chunk size
    */
    data_start_idx = headers.header_size + 8;
    unscaled_frames[num_samples] = 0;
    frames[num_samples] = 0;

    for (k=0; k<num_samples; k++) {
        /*
         * Each audio sample is a contiguous sequence of
         * 1 / 2 / 4 bytes in the file containing an unsigned integer
         * representing the raw amplitude of the audio sample.
         * In librosa the values are scaled to a range of -1 to 1
         * so we do the same here as well
        */
        unsigned long slice_start_idx, slice_end_idx;
        short unscaled_frame;
        double scaled_frame;

        /*
        if (k % 10000 == 0) {
            printf("SAMPLE %d/%ld\n", k, num_samples);
        }
        */
        slice_start_idx = data_start_idx + k * sample_size;
        slice_end_idx = data_start_idx + (k + 1) * sample_size;
        if (k == 0) {
            printf("SLICE_START: %ld\n", slice_start_idx);
        }
        unscaled_frame = (short) read_long_from_str_slice(
            fp, slice_start_idx, slice_end_idx, 1
        );
        scaled_frame = (
            ((double) unscaled_frame) / ((double) max_signed_int_val)
        );

        if (scaled_frame > 1) {
            /* value has under flowed due to being negative */
            scaled_frame -= 2;
        }

        if (slice_start_idx == DEBUG_INDEX) {
            printf("DOUBLE %f, %d\n", scaled_frame, k);
        }

        frames[k] = scaled_frame;
        unscaled_frames[k] = unscaled_frame;
    }

    printf("SLICE_END\n");
    /* free(raw_audio_data); */
    /* free_wav_headers(headers); */
    wav_file.scale = (double) max_signed_int_val;
    wav_file.headers = headers;
    wav_file.frames = frames;
    wav_file.num_frames = num_samples;
    wav_file.unscaled_frames = unscaled_frames;
    printf("NUM_FRAMES %lu\n", wav_file.num_frames);
    return wav_file;
}

WavParseResult read_wav_file(const char * filepath) {
    /*
     * reads the wav file headers as well as
     * the raw audio data from the file. The difference between this
     * and the read_frames function is that the returned WavParseResult
     * here throws away most of the information that isn't needed in
     * auto loop computation, and that the audio data here is
     * formatted as a shape [num_channels, samples_per_channel]
     * array instead of a 1D array like in read_frames
     *
     * Note: right now this is only being used in the python testing
     * script
     */
    WavParseResult wav_parse_result;
    long num_channels;
    double ** samples;
    unsigned long channel_length;
    int k;

    FILE *fp = fopen(filepath, "r");
    WavFile read_result = read_frames(fp);
    printf("READ_FRAMES_COMPLETE\n");

    num_channels = read_result.headers.num_channels;
    samples = (double **) malloc(
        (num_channels + 1) * sizeof(double *)
    );

    samples[num_channels] = 0;
    channel_length = read_result.num_frames / num_channels;
    printf("CHANNEL_LENGTH: %ld\n", channel_length);
    printf("NUM_CHANNELS: %ld\n", num_channels);

    for (k = 0; k < num_channels; k++) {
        double * channel;
        channel = (double *) malloc(
            (channel_length + 1) * sizeof(double)
        );

        channel[channel_length] = 0;
        samples[k] = channel;
    }

    printf("PRE_FRAME_REASSIGN\n");
    for (k=0; k < read_result.num_frames; k++) {
        unsigned long step_idx;
        unsigned long channel_idx;

        step_idx = k / num_channels;
        channel_idx = k % num_channels;
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
    /* WAVE Header Data */
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
    fwrite(file.headers.extra_params, 1, file.headers.extra_params_size, fp);

    /* Marks the start of the data */
    fwrite(file.headers.data_header, 1, 4, fp);
    fwrite(&file.headers.data_chunk_size, 4, 1, fp); 
    fwrite(file.unscaled_frames,2,file.num_frames,fp); /* Data size */
    /*
    for (int i = 0; i < file.num_frames; i++) {
        fwrite(&file.unscaled_frames[i] , sizeof(int16_t), 1, fp);
    }
    */
}

/*
int main(int argc, char ** argv){
     // WavParseResult result= read_wav_file("recycling.wav");
     // FILE *fp = fopen("write.wav", "r");
     // WavHeaders headers=read_wav_headers(fp);
     FILE *fp = fopen("recycling.wav", "r");
     WavFile file=read_frames(fp);

     printf("num_frames:%ld",file.num_frames);
     for(int i=100000;i<100200;i++){
         printf("%d",file.unscaled_frames[i]);
     }

     FILE * file_p = fopen("./write1.wav", "w");
     if (NULL == file_p) {
         printf("fopen failed in main");
         return 0;
     }

     size_t write_count = fwrite(&file.headers, 78, 1, file_p);
     printf("write count: %ld", sizeof(WavHeaders));

     write_count = fwrite(&file.frames, 2, file.num_frames, file_p);
     printf("write count: %ld", write_count);
     write_wav(file_p,file);
     fclose(fp);
     fclose(file_p);

     return 0;
 }
 */