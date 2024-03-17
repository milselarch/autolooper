#include <stdio.h>
#include "parse_wav.c"

int main(int argc, char ** argv) {
    for (int k=0; k<argc; k++) {
        printf("ARG[%d]: %s\n", k, argv[k]);
    }

    if (argc < 2) {
        printf("NOT ENOUGH ARGUMENTS");
        exit(1);
    }

    char * filepath = argv[1];
    FILE * fp = fopen(filepath, "r");
    WavHeaders headers = read_wav_headers(fp);
    print_wav_headers(headers);
    free_wav_headers(headers);
    return 0;
}