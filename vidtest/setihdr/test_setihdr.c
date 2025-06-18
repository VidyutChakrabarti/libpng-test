#include <stdio.h>

#include <setjmp.h>

#include <stdlib.h>

#include "png.h"

static jmp_buf error_handler_jmp_buf;

// Error handler for libpng

static void PNGCBAPI error_handler(png_structp png_ptr, png_const_charp error_msg)
{

    fprintf(stderr, "libpng error: %s\n", error_msg);

    longjmp(error_handler_jmp_buf, 1);
}

typedef struct
{

    png_uint_32 width, height;

    int bit_depth, color_type, compression, filter, interlace;

    const char *desc;

} IHDRTest;

int main(void)
{

    IHDRTest testcases[] = {

        {1, 1, 8, PNG_COLOR_TYPE_RGB, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE, PNG_INTERLACE_NONE, "Valid RGB 8-bit"},

        {0, 0, 8, PNG_COLOR_TYPE_RGB, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE, PNG_INTERLACE_NONE, "Zero width/height (invalid)"},

        {100, 100, 16, PNG_COLOR_TYPE_RGBA, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE, PNG_INTERLACE_ADAM7, "Valid RGBA 16-bit interlaced"},

        {10, 10, 1, PNG_COLOR_TYPE_GRAY, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE, PNG_INTERLACE_NONE, "Valid 1-bit grayscale"},

        {10, 10, 2, PNG_COLOR_TYPE_PALETTE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE, PNG_INTERLACE_NONE, "Invalid bit depth for palette"},

        {10, 10, 4, PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE, PNG_INTERLACE_NONE, "Invalid bit depth for gray+alpha"},

        {10, 10, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE, PNG_INTERLACE_NONE, "Valid RGBA 8-bit"},

    };

    size_t num_tests = sizeof(testcases) / sizeof(testcases[0]);

    for (size_t i = 0; i < num_tests; ++i)
    {

        printf("Running test %zu: %s\n", i + 1, testcases[i].desc);

        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, error_handler, NULL);

        if (!png_ptr)
        {

            fprintf(stderr, "Failed to create png_struct\n");

            continue;
        }

        png_infop info_ptr = png_create_info_struct(png_ptr);

        if (!info_ptr)
        {

            fprintf(stderr, "Failed to create info_ptr\n");

            png_destroy_write_struct(&png_ptr, NULL);

            continue;
        }

        if (setjmp(error_handler_jmp_buf))
        {

            fprintf(stderr, "Caught error in png_set_IHDR\n\n");

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

        printf("Test %zu completed successfully.\n\n", i + 1);
    }

    // Edge case: png_ptr == NULL

    {

        fprintf(stderr, "Testing with png_ptr == NULL\n");

        png_structp png_ptr = NULL;

        png_infop info_ptr = NULL;

        // This should do nothing and not crash

        png_set_IHDR(

            png_ptr,

            info_ptr,

            1, 1, 8,

            PNG_COLOR_TYPE_RGB,

            PNG_INTERLACE_NONE,

            PNG_COMPRESSION_TYPE_BASE,

            PNG_FILTER_TYPE_BASE

        );

        fprintf(stderr, "Test with png_ptr == NULL completed.\n\n");
    }

    // Edge case: info_ptr == NULL

    {

        fprintf(stderr, "Testing with info_ptr == NULL\n");

        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, error_handler, NULL);

        png_infop info_ptr = NULL;

        // safe: png_set_IHDR internally checks for NULL

        png_set_IHDR(

            png_ptr,

            info_ptr,

            1, 1, 8,

            PNG_COLOR_TYPE_RGB,

            PNG_INTERLACE_NONE,

            PNG_COMPRESSION_TYPE_BASE,

            PNG_FILTER_TYPE_BASE

        );

        png_destroy_write_struct(&png_ptr, NULL);

        fprintf(stderr, "Test with info_ptr == NULL completed.\n\n");
    }

    printf("All tests executed safely.\n");

    return 0;
}