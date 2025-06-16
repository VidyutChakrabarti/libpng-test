#include <stdio.h>

#include <setjmp.h>

#include "png.h"

// Custom error handler: print error and longjmp

static void PNGCBAPI error_handler(png_structp png_ptr, png_const_charp error_msg)
{

    fprintf(stderr, "libpng error: %s\n", error_msg);

    longjmp(png_jmpbuf(png_ptr), 1);
}

int main(void)
{

    struct
    {

        png_uint_32 width, height;

        int bit_depth, color_type, compression, filter, interlace;

    } testcases[] = {

        {1, 1, 8, PNG_COLOR_TYPE_RGB, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE, PNG_INTERLACE_NONE},

        {0, 0, 8, PNG_COLOR_TYPE_RGB, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE, PNG_INTERLACE_NONE}, // Invalid

        {100, 100, 16, PNG_COLOR_TYPE_RGBA, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE, PNG_INTERLACE_ADAM7},

        {10, 10, 1, PNG_COLOR_TYPE_GRAY, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE, PNG_INTERLACE_NONE},

        {10, 10, 2, PNG_COLOR_TYPE_PALETTE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE, PNG_INTERLACE_NONE},

        {10, 10, 4, PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE, PNG_INTERLACE_NONE},

        {10, 10, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE, PNG_INTERLACE_NONE},

        // Add more edge and invalid cases as needed

    };

    for (size_t i = 0; i < sizeof(testcases) / sizeof(testcases[0]); ++i)
    {

        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, error_handler, NULL);

        png_infop info_ptr = png_create_info_struct(png_ptr);

        if (setjmp(png_jmpbuf(png_ptr)))
        {

            // Error occurred, print and continue

            png_destroy_write_struct(&png_ptr, &info_ptr);

            continue;
        }

        png_set_IHDR(

            png_ptr,

            info_ptr,

            testcases[i].width,

            testcases[i].height,

            testcases[i].bit_depth,

            testcases[i].color_type,

            testcases[i].interlace,

            testcases[i].compression,

            testcases[i].filter

        );

        png_destroy_write_struct(&png_ptr, &info_ptr);
    }

    return 0;
}