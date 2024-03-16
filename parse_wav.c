#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

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

int read_wav(
    const char *filename, int channels, int sample_rate
) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) { return -1; }

    fseek(fp, 0L, SEEK_END);
    long filesize = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
}
