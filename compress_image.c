#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "compress.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main(int argc, char **argv){
    if (argc < 3){
        puts("Usage: compress_image <input_file> <output_file> [k]");
        return 0;
    }
    printf("Converting %s => %s\n", argv[1], argv[2]);
    char *input_filename = argv[1];
    char *output_filename = argv[2];
    int k = 256;
    if (argc >3){
        k = atoi(argv[3]);
        if (k < 1){
            puts("k must be at least 1");
        }
    }
    int x, y, n;
    unsigned char *data = stbi_load(input_filename, &x, &y, &n, 3);
    if (data == NULL){
        fputs("Something went wrong, your image might be corrupted or is not in the correct format\n", stdout);
        return 1;
    }
    compress_image(data, (size_t) 3*x*y, k);
    stbi_write_png(output_filename, x, y, n, data, x*n);
}
