#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    char * chunk_id;
    long chunk_size;
    char * format;
    long filesize;

    char * sub_chunk_id;
    long sub_chunk_size;
    long audio_format;
    long num_channels;
    long sample_rate;
    long byte_rate;
    long block_align;
    long bits_per_sample;
    long extra_params_size;
    char * extra_params;
} WavHeaders;

void free_wav_headers(WavHeaders headers) {
    free(headers.chunk_id);
    free(headers.format);
    free(headers.sub_chunk_id);
    free(headers.extra_params);
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
        if (is_little_endian) {
            current_char = string[k];
        } else {
            current_char = string[length-k-1];
        }

        result += current_char << (8 * k);
    }
    return result;
}

double bits_to_float(
    const unsigned long bits[], const unsigned long size
) {
    unsigned long raw_bit_data;

    if (size == 2) {
        raw_bit_data = (bits[1] << 8) | bits[0];
    } else if (size == 4) {
        raw_bit_data = (
            (bits[3] << 24) | (bits[2] << 16) | (bits[1] << 8) | bits[0]
        );
    } else if (size == 8) {
        raw_bit_data = (
            (bits[7] << 56) | (bits[6] << 48) | (bits[5] << 40) |
            (bits[4] << 32) | (bits[3] << 24) | (bits[2] << 16) |
            (bits[1] << 8) | bits[0]
        );
    } else {
        printf("INVALID SIZE TO PARSE FLOAT BITS");
        exit(1);
    }

    double floating_point;
    memcpy(&floating_point, &raw_bit_data, sizeof(double));
    return floating_point;
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

    long str_size = end_index - start_index + 1;
    char * str_slice = (char *) malloc(str_size * sizeof(char));
    str_slice[end_index - start_index] = 0;

    for (long k=start_index; k<end_index; k++) {
        fseek(fp, k, SEEK_SET);
        long str_slice_index = k - start_index;
        int fget_result = fgetc(fp);
        if (fget_result == EOF) {
            printf("END_OF_FILE REACHED");
            exit(1);
        }
        str_slice[str_slice_index] = (char) fget_result;
    }

    fseek(fp, 0L, SEEK_SET);
    return str_slice;
}

unsigned long read_long_from_str_slice(
    FILE *fp, unsigned long start_index, unsigned long end_index,
    int is_little_endian
) {
    unsigned long length = end_index - start_index;
    char * raw_str_slice = read_str_slice(fp, start_index, end_index);
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

    int length = end - start + 1;
    char * dest_str = (char *) malloc(length * sizeof(char));
    dest_str[length - 1] = 0;

    // https://stackoverflow.com/questions/26620388/
    strncpy(dest_str, source_str + start, end - start);
    return dest_str;
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
    if (!is_str_equal(chunk_id, "RIFF")) {
        printf("INVALID_FILE_HEADER");
        exit(1);
    }

    int chunk_size = read_long_from_str_slice(fp, 4, 8, 1);
    char * format_str = read_str_slice(fp, 8, 12);
    if (!is_str_equal(format_str, "WAVE")) {
        printf("WAV FORMAT IS NOT WAVE: %s\n", format_str);
        exit(1);
    }

    char * sub_chunk_id = read_str_slice(fp, 12, 16);
    int sub_chunk_size = read_long_from_str_slice(fp, 16, 20, 1);
    int format = read_long_from_str_slice(fp, 20, 22, 1);
    int num_channels = read_long_from_str_slice(fp,22, 24, 1);
    int sample_rate = read_long_from_str_slice(fp, 24, 28, 1);
    int byte_rate = read_long_from_str_slice(fp, 28, 32, 1);
    int block_align = read_long_from_str_slice(fp, 32, 34, 1);
    int bits_per_sample = read_long_from_str_slice(fp, 34, 36, 1);

    int extra_params_size = 0;
    char * extra_params = 0;

    if (sub_chunk_size != 16) {
        char * raw_extra_size = read_str_slice(fp, 36, 38);
        extra_params_size = byte_str_to_long(raw_extra_size, 1, 2);
        extra_params = read_str_slice(fp, 38, 38 + extra_params_size);
        free(raw_extra_size);
    }

    WavHeaders headers;
    headers.chunk_id = chunk_id;
    headers.chunk_size = chunk_size;
    headers.format = format_str;
    headers.filesize = filesize;

    headers.sub_chunk_id = sub_chunk_id;
    headers.sub_chunk_size = sub_chunk_size;
    headers.audio_format = format;
    headers.num_channels = num_channels;
    headers.sample_rate = sample_rate;
    headers.byte_rate = byte_rate;
    headers.block_align = block_align;
    headers.bits_per_sample = bits_per_sample;
    headers.extra_params_size = extra_params_size;
    headers.extra_params = extra_params;
    return headers;
}

void print_wav_headers(WavHeaders headers) {
    printf("---- WAV FILE HEADERS ----\n");
    printf("CHUNK_ID: %s\n", headers.chunk_id);
    printf("CHUNK_SIZE: %ld\n", headers.chunk_size);
    printf("FORMAT: %s\n", headers.format);
    printf("SUB_CHUNK_ID: %s\n", headers.sub_chunk_id);
    printf("SUB_CHUNK_SIZE: %ld\n", headers.sub_chunk_size);
    printf("AUDIO_FORMAT: %ld\n", headers.audio_format);
    printf("NUM_CHANNELS: %ld\n", headers.num_channels);
    printf("SAMPLE_RATE: %ld\n", headers.sample_rate);
    printf("BYTE_RATE: %ld\n", headers.byte_rate);
    printf("BLOCK_ALIGN: %ld\n", headers.block_align);
    printf("BITS_PER_SAMPLE: %ld\n", headers.bits_per_sample);
    printf("EXTRA_PARAMS_SIZE: %ld\n", headers.extra_params_size);
    printf("EXTRA_PARAMS: %s\n", headers.extra_params);
}

/*
void read_frames(FILE * fp) {
    WavHeaders headers = read_wav_headers(fp);

    int start_index = 36 + headers.extra_params_size;
    char * raw_audio_data = read_str_slice(fp, start_index + 8, headers.filesize);
    int sample_size = headers.bits_per_sample / 8;
    long raw_data_length = strlen(raw_audio_data);
    int num_samples = raw_data_length / sample_size;

    for (int k=0; k<num_samples; k++) {
        char * raw_frame = slice_str(
            raw_audio_data, k * sample_size, (k+1) * sample_size
        );
    }

    free(raw_audio_data);
    free_wav_headers(headers);
}
*/