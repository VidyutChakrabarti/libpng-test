#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

/* Simple error handler: jumps back to setjmp point */
static void user_error_fn(png_structp png_ptr, png_const_charp msg)
{
    /* Print error and longjmp */
    fprintf(stderr, "libpng fatal error: %s\n", msg);
    longjmp(png_jmpbuf(png_ptr), 1);
}

static void user_warning_fn(png_structp png_ptr, png_const_charp msg)
{
    /* Warnings only */
    fprintf(stderr, "libpng warning: %s\n", msg);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <input.png> <left_output.png> <right_output.png>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *infile = argv[1];
    const char *leftfile = argv[2];
    const char *rightfile = argv[3];

    /*******************************************************/
    /* 1. Open input PNG in binary mode                    */
    /*******************************************************/
    FILE *fp_in = fopen(infile, "rb");
    if (!fp_in)
    {
        perror("Error opening input PNG");
        return EXIT_FAILURE;
    }

    /*******************************************************/
    /* 2. Read first 8 bytes (PNG signature)               */
    /*******************************************************/
    png_byte header[8];
    if (fread(header, 1, 8, fp_in) != 8)
    {
        fprintf(stderr, "Failed to read PNG signature (file too small).\n");
        fclose(fp_in);
        return EXIT_FAILURE;
    }

    /*******************************************************/
    /* 3. Verify signature with png_sig_cmp()              */
    /*******************************************************/
    if (png_sig_cmp(header, 0, 8) != 0)
    {
        fprintf(stderr, "File is not recognized as a valid PNG.\n");
        fclose(fp_in);
        return EXIT_FAILURE;
    }

    /*******************************************************/
    /* 4. Create read_struct and info_struct               */
    /*******************************************************/
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                 NULL,
                                                 user_error_fn,
                                                 user_warning_fn);
    if (!png_ptr)
    {
        fprintf(stderr, "Cannot create png read_struct.\n");
        fclose(fp_in);
        return EXIT_FAILURE;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fprintf(stderr, "Cannot create png info_struct.\n");
        fclose(fp_in);
        return EXIT_FAILURE;
    }

    /* 5. Set up error handling (longjmp) */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        /* libpng encountered a fatal error */
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fprintf(stderr, "Fatal libpng error during reading.\n");
        fclose(fp_in);
        return EXIT_FAILURE;
    }

    /*******************************************************/
    /* 6. Tell libpng to read from our FILE*               */
    /*******************************************************/
    png_init_io(png_ptr, fp_in);

    /*******************************************************/
    /* 7. Inform libpng we already read 8 signature bytes  */
    /*******************************************************/
    png_set_sig_bytes(png_ptr, 8);

    /*******************************************************/
    /* 8. Read PNG info (IHDR + other chunks until IDAT)   */
    /*******************************************************/
    png_read_info(png_ptr, info_ptr);

    /* 9. Extract basic image info from info_ptr            */
    png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
    png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
    int bit_depth = png_get_bit_depth(png_ptr, info_ptr);   /* e.g. 8 or 16 */
    int color_type = png_get_color_type(png_ptr, info_ptr); /* palette, RGB, RGBA, etc. */
    int interlace_method = png_get_interlace_type(png_ptr, info_ptr);
    int compression_method = png_get_compression_type(png_ptr, info_ptr);
    int filter_method = png_get_filter_type(png_ptr, info_ptr);

    /* 10. Set up our desired transformations:
     *   - Expand palettes and tRNS → full alpha
     *   - Strip 16 → 8 if necessary
     *   - Convert grayscale → RGB
     */
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr); /* Expand palette to RGB */
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr); /* Expand tRNS to alpha channel */
    if (bit_depth == 16)
        png_set_strip_16(png_ptr); /* Strip 16→8 */
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr); /* Gray → RGB */

    /* After transforms, we want either RGB (3 channels) or RGBA (4 channels) */
    /* Tell libpng to update info_ptr to reflect these transforms */
    png_read_update_info(png_ptr, info_ptr);

    /* 11. Obtain updated info */
    png_uint_32 new_width = png_get_image_width(png_ptr, info_ptr);   /* same as width */
    png_uint_32 new_height = png_get_image_height(png_ptr, info_ptr); /* same as height */
    int new_bit_depth = png_get_bit_depth(png_ptr, info_ptr);         /* should be 8 */
    int new_color_type = png_get_color_type(png_ptr, info_ptr);       /* PNG_COLOR_TYPE_RGB or RGBA */
    int channels = png_get_channels(png_ptr, info_ptr);               /* 3 or 4 */
    png_size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);        /* new_width × channels */

    if (new_bit_depth != 8 ||
        (new_color_type != PNG_COLOR_TYPE_RGB && new_color_type != PNG_COLOR_TYPE_RGBA))
    {
        fprintf(stderr,
                "Unsupported final format: bit_depth=%d, color_type=%d\n",
                new_bit_depth, new_color_type);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp_in);
        return EXIT_FAILURE;
    }

    /* 12. Allocate array of row pointers for original image */
    png_bytep *orig_rows = malloc(sizeof(png_bytep) * new_height);
    if (!orig_rows)
    {
        fprintf(stderr, "Out of memory allocating orig_rows.\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp_in);
        return EXIT_FAILURE;
    }
    for (png_uint_32 y = 0; y < new_height; y++)
    {
        orig_rows[y] = malloc(rowbytes);
        if (!orig_rows[y])
        {
            fprintf(stderr, "Out of memory for orig_rows[%u].\n", y);
            for (png_uint_32 k = 0; k < y; k++)
                free(orig_rows[k]);
            free(orig_rows);
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            fclose(fp_in);
            return EXIT_FAILURE;
        }
    }

    /* 13. Read the entire image into orig_rows[][] */
    png_read_image(png_ptr, orig_rows);

    /* 14. Finish reading (IEND etc.) */
    png_read_end(png_ptr, NULL);

    /* We have the full image now; close input file */
    fclose(fp_in);

    /*******************************************************/
    /* 15. Compute left/right split dimensions             */
    /*******************************************************/
    png_uint_32 left_width = new_width / 2;
    png_uint_32 right_width = new_width - left_width;
    png_size_t bpp = channels; /* bytes per pixel since bit_depth=8 */

    /* 16. Allocate row pointers for left and right halves */
    png_bytep *left_rows = malloc(sizeof(png_bytep) * new_height);
    png_bytep *right_rows = malloc(sizeof(png_bytep) * new_height);
    if (!left_rows || !right_rows)
    {
        fprintf(stderr, "Out of memory allocating split row pointers.\n");
        goto cleanup_orig;
    }

    for (png_uint_32 y = 0; y < new_height; y++)
    {
        left_rows[y] = malloc(left_width * bpp);
        right_rows[y] = malloc(right_width * bpp);
        if (!left_rows[y] || !right_rows[y])
        {
            fprintf(stderr, "Out of memory allocating split row %u.\n", y);
            for (png_uint_32 k = 0; k <= y; k++)
            {
                free(left_rows[k]);
                free(right_rows[k]);
            }
            free(left_rows);
            free(right_rows);
            goto cleanup_orig;
        }

        /* Copy left half of this row */
        memcpy(left_rows[y],
               orig_rows[y], /* byte 0 */
               left_width * bpp);

        /* Copy right half of this row */
        memcpy(right_rows[y],
               orig_rows[y] + (left_width * bpp),
               right_width * bpp);
    }

    /*******************************************************/
    /* 17. Write LEFT half to 'leftfile'                   */
    /*******************************************************/
    {
        FILE *fp_left = fopen(leftfile, "wb");
        if (!fp_left)
        {
            perror("Error opening left output PNG");
            goto cleanup_splits;
        }

        png_structp png_left = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                       NULL,
                                                       user_error_fn,
                                                       user_warning_fn);
        if (!png_left)
        {
            fprintf(stderr, "Cannot create write_struct for left half.\n");
            fclose(fp_left);
            goto cleanup_splits;
        }

        png_infop info_left = png_create_info_struct(png_left);
        if (!info_left)
        {
            fprintf(stderr, "Cannot create info_struct for left half.\n");
            png_destroy_write_struct(&png_left, NULL);
            fclose(fp_left);
            goto cleanup_splits;
        }

        if (setjmp(png_jmpbuf(png_left)))
        {
            fprintf(stderr, "libpng error while writing left half.\n");
            png_destroy_write_struct(&png_left, &info_left);
            fclose(fp_left);
            goto cleanup_splits;
        }

        png_init_io(png_left, fp_left);

        /* Set IHDR for left half: 8-bit, same color type, no interlace */
        png_set_IHDR(png_left, info_left,
                     left_width, new_height,
                     new_bit_depth,
                     new_color_type,
                     PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE,
                     PNG_FILTER_TYPE_BASE);

        png_set_rows(png_left, info_left, left_rows);
        png_write_info(png_left, info_left);
        png_write_image(png_left, left_rows);
        png_write_end(png_left, NULL);

        png_destroy_write_struct(&png_left, &info_left);
        fclose(fp_left);
    }

    /*******************************************************/
    /* 18. Write RIGHT half to 'rightfile'                 */
    /*******************************************************/
    {
        FILE *fp_right = fopen(rightfile, "wb");
        if (!fp_right)
        {
            perror("Error opening right output PNG");
            goto cleanup_splits;
        }

        png_structp png_right = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                        NULL,
                                                        user_error_fn,
                                                        user_warning_fn);
        if (!png_right)
        {
            fprintf(stderr, "Cannot create write_struct for right half.\n");
            fclose(fp_right);
            goto cleanup_splits;
        }

        png_infop info_right = png_create_info_struct(png_right);
        if (!info_right)
        {
            fprintf(stderr, "Cannot create info_struct for right half.\n");
            png_destroy_write_struct(&png_right, NULL);
            fclose(fp_right);
            goto cleanup_splits;
        }

        if (setjmp(png_jmpbuf(png_right)))
        {
            fprintf(stderr, "libpng error while writing right half.\n");
            png_destroy_write_struct(&png_right, &info_right);
            fclose(fp_right);
            goto cleanup_splits;
        }

        png_init_io(png_right, fp_right);

        png_set_IHDR(png_right, info_right,
                     right_width, new_height,
                     new_bit_depth,
                     new_color_type,
                     PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE,
                     PNG_FILTER_TYPE_BASE);

        png_set_rows(png_right, info_right, right_rows);
        png_write_info(png_right, info_right);
        png_write_image(png_right, right_rows);
        png_write_end(png_right, NULL);

        png_destroy_write_struct(&png_right, &info_right);
        fclose(fp_right);
    }

    /*******************************************************/
    /* 19. Cleanup split rows and exit                      */
    /*******************************************************/
cleanup_splits:
    for (png_uint_32 y = 0; y < new_height; y++)
    {
        free(left_rows[y]);
        free(right_rows[y]);
    }
    free(left_rows);
    free(right_rows);

    /* Fall through to clean up orig rows */

cleanup_orig:
    for (png_uint_32 y = 0; y < new_height; y++)
    {
        free(orig_rows[y]);
    }
    free(orig_rows);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return EXIT_SUCCESS;
}
