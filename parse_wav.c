#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    char * chunk_id;
    unsigned long chunk_size;
    char * format;
} WavHeaders;

typedef struct {
    char * sub_chunk_id;
    int sub_chunk_size;
    int audio_format;
    int num_channels;
    int sample_rate;
    int byte_rate;
    int block_align;
    int bits_per_sample;
    int extra_params_size;
    char * extra_params;
} WavFormat;

int byte_str_to_int(char * string, int is_little_endian) {
    int result = 0;
    unsigned long length = strlen(string);

    for (int k=0; k<length; k++) {
        char current_char;
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
    if (strlen(string1) != strlen(string2)) {
        return 0;
    }

    unsigned long length = strlen(string1);
    for (unsigned long k=0; k<length; k++) {
        if (string1[k] != string2[k]) {
            return 0;
        }
    }

    return 1;
}

char * read_str_slice(
    FILE *fp, long start_index, long end_index
) {
    if (end_index < start_index) {
        printf("END_INDEX IS LESS THAN START_INDEX");
        exit(1);
    }

    long str_size = end_index - start_index + 1;
    char * str_slice = (char *) malloc(str_size * sizeof(char));
    str_slice[end_index+1] = 0;

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

WavHeaders read_wav_headers(FILE * fp) {
    if (fp == NULL) {
        printf("FILE_OPEN_FAILED");
        exit(1);
    }

    fseek(fp, 0L, SEEK_END);
    long filesize = ftell(fp);

    char * chunk_id = read_str_slice(fp, 0, 4);
    if (!is_str_equal(chunk_id, "RIFF")) {
        printf("INVALID_FILE_HEADER");
        exit(1);
    }

    char * chunk_size_str = read_str_slice(fp, 4, 8);
    unsigned long chunk_size;
    // copy raw chunk size data into chunk size variable
    memccpy(&chunk_size, chunk_size_str, '\0', 4);

    char * format_str = read_str_slice(fp, 8, 12);
    if (!is_str_equal(format_str, "WAVE")) {
        printf("WAV FORMAT IS NOT WAVE");
        exit(1);
    }

    WavHeaders headers = {};
    headers.chunk_id = chunk_id;
    headers.chunk_size = chunk_size;
    headers.format = format_str;
    return headers;
}

WavFormat read_wav_format(FILE * fp) {
    WavHeaders headers;
    char * sub_chunk_id = read_str_slice(fp, )
}

int read_wav(
    const char *filename, int channels, int sample_rate
) {
    FILE *fp = fopen(filename, "r");
    WavHeaders headers = read_wav_headers(fp);

}
