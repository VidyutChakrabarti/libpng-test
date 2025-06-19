#include <stdio.h>

#include <stdlib.h>

#include <png.h>

int main(int argc, char **argv)
{

    if (argc != 2)
    {

        fprintf(stderr, "Usage: %s <input_png>\n", argv[0]);

        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");

    if (!fp)
    {

        perror("fopen");

        return 1;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
    {

        fclose(fp);

        return 1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);

    if (!info_ptr)
    {

        png_destroy_read_struct(&png_ptr, NULL, NULL);

        fclose(fp);

        return 1;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {

        // libpng error

        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

        fclose(fp);

        return 1;
    }

    png_init_io(png_ptr, fp);

    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    fclose(fp);

    return 0;
}