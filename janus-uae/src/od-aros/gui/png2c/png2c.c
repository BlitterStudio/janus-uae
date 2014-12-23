/*
 * png2c - converts a PNG to C source
 *
 * (c) 2014 Nick "Kalamatee" Andrews
 * (c) 2001-2002 by David Gerber <zapek@meanmachine.ch>
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <png.h>

#define ID1 "$I"
#define ID2 "d$"

#define RGB888toRGB565(r, g, b) ((r >> 3) << 11)| \
	((g >> 2) << 5)| ((b >> 3) << 0)

#define RGB8888toRGB5A1(r, g, b, a) ((r >> 3) << 11)| \
	((g >> 3) << 6)| ((b >> 3) << 1) | (a == 0x0)

int main(int argc, char **argv)
{
    int showusage = 0, generate5A1 = 0;

    if (argc >= 2)
    {
        if (argc == 2 && !strcasecmp("-v", argv[1]))
        {
            showusage = 1;
        }
        else
        {
            FILE *in, *out, *hout;

            if (in = fopen(argv[1], "rb"))
            {
                if (argc == 4)
                {
                    if (!(hout = fopen(argv[3], "w")))
                    {
                        printf("unable to open hfile\n");
                        return (0);
                    }
                }

                if (out = fopen(argv[2], "w"))
                {
                    char header[8];

                    fread(header, 1, 8, in);
                    if (!png_sig_cmp(header, 0, 8))
                    {
                        png_structp png_ptr;

                        if (png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))
                        {
                            png_infop info_ptr;
                            png_infop end_info = NULL;

                            if (info_ptr = png_create_info_struct(png_ptr))
                            {
                                int width, height;
                                png_byte colorType;
                                png_byte bitDepth;
                                int numPasses;

                                png_bytep *rows;

                                if (!setjmp(png_jmpbuf(png_ptr)))
                                {
                                    png_init_io(png_ptr, in);
                                    
                                    png_set_sig_bytes(png_ptr, 8);

                                    png_read_info(png_ptr, info_ptr);
                                    width = png_get_image_width(png_ptr, info_ptr);
                                    height = png_get_image_height(png_ptr, info_ptr);
                                    colorType = png_get_color_type(png_ptr, info_ptr);
                                    bitDepth = png_get_bit_depth(png_ptr, info_ptr);
                                    numPasses = png_set_interlace_handling(png_ptr);

                                    png_read_update_info(png_ptr, info_ptr);
                                    printf("Metadata: width: %i, height: %i, color type: %i, bit depth: %i, num passes: %i\n", width, height, colorType, bitDepth, numPasses);

                                    // read the file
                                    if (!setjmp(png_jmpbuf(png_ptr))) {
                                        switch (colorType)
                                        {
                                            case PNG_COLOR_TYPE_RGB:
                                                printf("RGB");
                                                break;

                                            case PNG_COLOR_TYPE_RGB_ALPHA:
                                                printf("RGBA");
                                                break;

                                            case PNG_COLOR_TYPE_GRAY:
                                                printf("gray");
                                                break;

                                            case PNG_COLOR_TYPE_GRAY_ALPHA:
                                                printf("gray with alpha");
                                                break;

                                            case PNG_COLOR_TYPE_PALETTE:
                                                printf("palette based");
                                                break;

                                            default:
                                                printf("unknown");
                                        }

                                        printf(" color format");

                                        if ((colorType == PNG_COLOR_TYPE_RGB) || (colorType == PNG_COLOR_TYPE_RGB_ALPHA))
                                        {
                                            int pix_r, pix_g, pix_b, pix_a;
                                            unsigned short convPixel;
                                            png_byte *row, *pixel;
                                            char *p;
                                            char imagename[108];
                                            int x, y, namelen;
                                            png_bytep *rows;

                                            printf("\n");

                                            rows = (png_bytep*) malloc(sizeof(png_bytep) * height);
                                            for (x = 0; x < height; x++)
                                                rows[x] = (png_byte*) malloc(png_get_rowbytes(png_ptr, info_ptr));
                                            png_read_image(png_ptr, rows);

                                            if (p = strrchr(argv[1], '/'))
                                            {
                                                p++;
                                            }
                                            else if (p = strchr(argv[1], ':'))
                                            {
                                                p++;
                                            }
                                            else
                                            {
                                                p = argv[1];
                                            }

                                            strncpy(imagename, p, 108);
                                            p = imagename;
                                            namelen = strlen(imagename) - 4;
                                            while (*p)
                                            {
                                                if (p == (imagename + namelen))
                                                {
                                                    *p = 0;
                                                    break;
                                                }
                                                *p = toupper(*p);
                                                p++;
                                            }

                                            if (argc == 4)
                                            {
                                                // output the header
                                                fprintf(hout,
                                                    "/*\n * Automatically generated by png2c\n *\n * "ID1 ID2"\n */\n\n"
                                                    "#ifndef _%s_H_\n"
                                                    "#define _%s_H_\n\n"
                                                    "extern const unsigned int %s_WIDTH;\n"
                                                    "extern const unsigned int %s_HEIGHT;\n"
                                                    "extern const int %s_DEPTH;\n"
                                                    "extern const unsigned short %s[];\n\n"
                                                    "#endif\n\n",
                                                    imagename, imagename, imagename, imagename, imagename, imagename);
                                            }

                                            // output the source
                                            fputs("/*\n * Automatically generated by png2c\n *\n * "ID1 ID2"\n */\n\n", out);
                                            if (argc == 4)
                                            {
                                                fprintf(out,
                                                    "#include \"%s\"\n\n", argv[3]);
                                            }
                                            fprintf(out,
                                                "const unsigned int %s_WIDTH = %u;\n"
                                                "const unsigned int %s_HEIGHT = %u;\n"
                                                "const int %s_DEPTH = %u;\n"
                                                "const unsigned short %s[] = {",
                                                imagename, width, imagename, height, imagename, bitDepth, imagename);

                                            for (y = 0; y < height; y++)
                                            {
                                                row = rows[y];
                                                for (x = 0; x < width; x++)
                                                {
                                                    pixel = &(row[x*4]);

                                                    if (x == 0)
                                                    {
                                                        fputs("\n\t", out);
                                                    }

                                                    if (colorType == PNG_COLOR_TYPE_RGB)
                                                    {
                                                        pix_r = pixel[0];
                                                        pix_g = pixel[1];
                                                        pix_b = pixel[2];
                                                        pix_a = 0xFF;
                                                    }
                                                    else
                                                    {
                                                        pix_r = pixel[0];
                                                        pix_g = pixel[1];
                                                        pix_b = pixel[2];
                                                        pix_a = pixel[3];
                                                    }
                                                    convPixel = generate5A1 ? RGB8888toRGB5A1(pix_r, pix_g, pix_b, pix_a) : RGB888toRGB565(pix_r, pix_g, pix_b);
                                                    fprintf(out, "0x%x, ", convPixel);
                                                }
                                            }
                                            fputs("\n};\n", out);
                                        }
                                        else
                                        {
                                            printf(" (unsupported!)\n");
                                        }
                                    }
                                    else
                                    {
                                        printf("fatal error reading PNG file\n");
                                    }
                                }
                                else
                                {
                                    printf("fatal error reading PNG file properties\n");
                                }
                            }
                            else
                            {
                                printf("couldn't read PNG info structure\n");
                            }

                            if (info_ptr)
                            {
                                png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
                            }
                            else
                            {
                                png_destroy_read_struct(&png_ptr, NULL, NULL);
                            }
                        }
                        else
                        {
                                printf("couldn't read PNG structure\n");
                        }
                    }
                    else
                    {
                        printf("input is not a valid PNG file\n");
                    }
                    fclose(out);
                }
                else
                {
                    printf("cannot open outfile\n");
                }
                if (argc == 4)
                {
                    fclose(hout);
                }
                fclose(in);
            }
        }
    }
    else
        showusage = 1;

    if (showusage)
    {
        printf("png2c 1.2\n(c) 2014 Nick 'Kalamatee' Andrews, 2001-2002 by David Gerber <zapek@meanmachine.ch>\nFreeware - All Rights Reserved.\n");
        printf("usage: %s <pngfile> <cfile> [hfile] [-a] [-v]\n       -a output in RGB5A1 instead of RGB565\n\n       -v for version info\n", argv[0]);
    }

    return (0);
}
