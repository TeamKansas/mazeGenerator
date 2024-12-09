#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETCOLOR(IMG,R,G,B) { IMG->color[0] = R; IMG->color[1] = G; IMG->color[2] = B; }

#define BYTE     1
#define ASCII    2
#define SHORT    3
#define LONG     4
#define RATIONAL 5

#define GRAYSCALE 1
#define FULLCOLOR 3

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t color[3];
    uint8_t *data;
    uint32_t datlen;
    uint8_t bytesPerPixel;
    char type;
} image;

void setColor(image *img, uint8_t r, uint8_t g, uint8_t b) {
    SETCOLOR(img,r,g,b);
}

/* fills image with current color */
void fillColor(image *img) {
    for(uint32_t i = 0; i < img->datlen; i += img->bytesPerPixel) {
        memcpy(img->data + i, img->color, img->bytesPerPixel);
    }
}

/* set given pixel to current color. */
void setPixel(image *img, int x, int y) {
    if(x >= 0 && x < img->width && y >= 0 && y < img->height) {
        uint32_t index = (y * img->width + x) * img->bytesPerPixel;
        memcpy(img->data + index, img->color, img->bytesPerPixel);
    }
}

/* same as setPixel, but it does not protect against bad (x, y) pairs */
void unsafeSetPixel(image *img, int x, int y) {
    uint32_t index = (y * img->width + x) * img->bytesPerPixel;
    memcpy(img->data + index, img->color, img->bytesPerPixel);
}

/* draws orthogonal line (vertical or horizontal), of length (int) len, starting at (x, y), moving in direction dir */
/* dir = 0: right, dir = 1: numerically up (visually down), dir = 3: left, dir = 4: numerically down (visually up)  */
void orthoLine(image *img, int x, int y, char dir, int len) {
    int direction[2] = {((dir + 1)%2), (dir%2)};
    dir = -(2 * ((dir/2)%2) - 1);
    direction[0] *= dir;
    direction[1] *= dir;
    if(direction[0] == 0 && (x < 0 || x >= img->width)) /* vertical line */
        return;
    if(direction[1] == 0 && (y < 0 || y >= img->width)) /* horizontal line */
        return;
    int start = direction[0] ? x : y;
    int limit = direction[0] ? img->width : img->height;
    uint32_t index;
    for(; len >= 0; --len) {
        if(start + len * dir < limit && start + len * dir >= 0) {
            index = ((y + len * direction[1]) * img->width + x + len * direction[0]) * img->bytesPerPixel;
            memcpy(img->data + index, img->color, img->bytesPerPixel);

        }
    }
}

/* trace rectangle whose opposite vertices are specified by (x1, y1) and (x2, y2) */
void traceRect(image *img, int x1, int y1, int x2, int y2) {
    int tmp = x1;
    if(tmp > x2) {
        x1 = x2;
        x2 = tmp;
    }
    if((tmp = y1) > y2) {
        y1 = y2;
        y2 = tmp;
    }
    int pos;
    for(int i = y2 - y1; i >= 0; --i) {
        if(i + y1 >= 0 && i + y1 < img->height) {
            if(x1 >= 0 && x1 < img->width)
                unsafeSetPixel(img,x1,y1 + i);
            if(x2 >= 0 && x2 < img->width)
                unsafeSetPixel(img,x2,y1 + i);
        }
    }
    for(int i = x2 - x1; i >= 0; --i) {
        if(i + x1 >= 0 && i + x1 < img->width) {
            if(y1 >= 0 && y1 < img->height) {
                if(i + y1 >= 0 && i + y1 < img->datlen)
                    unsafeSetPixel(img,x1 + i,y1);
            }
            if(y2 >= 0 && y2 < img->height) {
                if(i + y2 >= 0 && i + y2 < img->datlen)
                    unsafeSetPixel(img,x1 + i,y2);
            }
        }
    }

}

/* trace rectangle whose opposite vertices are specified by (x1, y1) and (x2, y2) */
void fillRect(image *img, int x1, int y1, int x2, int y2) {
    int tmp = x1;
    if(tmp > x2) {
        x1 = x2;
        x2 = tmp;
    }
    if((tmp = y1) > y2) {
        y1 = y2;
        y2 = tmp;
    }
    if(x1 >= img->width || y1 >= img->height || x2 < 0 || y2 < 0)
        return;
    if(x1 < 0)
        x1 = 0;
    if(y1 < 0)
        y1 = 0;
    if(x2 >= img->width)
        x2 = img->width - 1;
    if(y2 >= img->height)
        y2 = img->height - 1;
    for(; x1 <= x2; ++x1)
        for(int y = y1; y <= y2; ++y)
            unsafeSetPixel(img, x1, y);
}

/* create image structure with specified parameters and return pointer to structure */
image *iopen(uint16_t width, uint16_t height, int type) {
    image *img = malloc(sizeof(image));
    switch(type) {
        case GRAYSCALE:
            img->bytesPerPixel = 1;
            break;
        case FULLCOLOR:
            img->bytesPerPixel = 3;
        default:
            img->bytesPerPixel = 3;
    }
    img->type = type;
    img->width = width;
    img->height = height;
    img->color[0] = img->color[1] = img->color[2] = 0;
    img->datlen = width * height * img->bytesPerPixel;
    if(!(img->data = malloc(img->datlen)))
        return NULL;
    memset(img->data,0,img->datlen);
    return img;
}

/* free allocated memory */
void iclose(image *img) {
    free(img->data);
    free(img);
}

/* write an entry to the image file directory */
void writeIFDEntry(FILE *fp, uint16_t tag, uint16_t type, uint32_t valCount, uint32_t offset) {
    fwrite(&tag, 2, 1, fp);
    fwrite(&type, 2, 1, fp);
    fwrite(&valCount, 4, 1, fp);
    fwrite(&offset, 4, 1, fp);
}

/* write the image header for the specified image */
void writeHeader(image *img, FILE *fp) {
    uint16_t b2;
    uint32_t b4;
    fwrite("II", 2, 1, fp); /* Little endian format        0-1 */ 
    b2 = 42;
    fwrite(&b2, 2, 1, fp); /* TIFF identification number   2-3 */
    b4 = 8 + img->datlen; /* header + data + XResolution + YResolution */
    fwrite(&b4, 4, 1, fp); /* offset of first IFD          4-7 */
}

/* write the IFD for the specified image */
void writeIFD(image *img, FILE *fp) {
    uint16_t b2;
    uint32_t b4;

    switch(img->type) {
        case GRAYSCALE:
            b2 = 11;
            break;
        case FULLCOLOR:
            b2 = 12;
            break;
        default:
            return;
    }
    fwrite(&b2, 2, 1, fp); /* number of IFD entries*/

    writeIFDEntry(fp, 256, SHORT, 1, img->width);             /* ImageWidth: number of columns per image */
    writeIFDEntry(fp, 257, SHORT, 1, img->height);            /* ImageLength: number of scanlines */
    if(img->type == FULLCOLOR)                                /* BitsPerSample: number of bits per pixel. 8-bits for range of 0-255 */
        writeIFDEntry(fp, 258, SHORT, 3, 8 + 2 + img->datlen + b2*12 + 4 + 16);
    else
        writeIFDEntry(fp, 258, SHORT, 1, 8);
    writeIFDEntry(fp, 259, SHORT, 1, 1);                      /* Compression: 1 = no compression, 2 = CCITT Group 3 1-Dimensional Modified Huffman run length encoding, 32773 = PackBits compression */
    writeIFDEntry(fp, 262, SHORT, 1, img->type == FULLCOLOR ? 2 : 1);                  /* PhotometricInterpretation: 1 = black is zero, 0 = white is zero 3 = palette color with colormap (black is zero in this case) */
    writeIFDEntry(fp, 273, LONG, 1, 8);                       /* StripOffsets: the offset in bytes from the beinning of the file to each strip. In this case, the image data comes right after the header, then the IFD */
    if(img->type == FULLCOLOR)
        writeIFDEntry(fp, 277, SHORT, 1, 3);                  /* SamplesPerPixel: 3 */
    writeIFDEntry(fp, 278, SHORT, 1, img->height);            /* RowsPerStrip: number of rows in each strip. (I am only doing one strip, so it will be all of them) */
    writeIFDEntry(fp, 279, LONG, 1, img->datlen);             /* StripByteCounts: number of bytes in each strip */
    writeIFDEntry(fp, 282, RATIONAL, 1, 12 + img->datlen + b2 * 12);  /* XResolution number of pixels per resolution unit in imageWidth. 1/1 */
    writeIFDEntry(fp, 283, RATIONAL, 1, 20 + img->datlen + b2 * 12);  /* XResolution nuYmber of pixels per resolution unit in imageLength. 1/1 */
    writeIFDEntry(fp, 296, SHORT, 1, 1);                      /* ResolutionUnit: 1 = no unit, 2 = inch, 3 = centimeter, default is inch */
    b4 = 0;
    fwrite(&b4,4,1,fp);

    /* define X and Y resolution (just one) */
    b4 = 1;
    for(int i = 0; i < 4; ++i)
        fwrite(&b4, 4, 1, fp);

    /* define bitsPerSample (3 samples, 8 bits each) */
    b2 = 8;
    for(int i = 0; i < 3 && img->type == FULLCOLOR; ++i)
        fwrite(&b2, 2, 1, fp);
}

/* write the complete tiff file with the specified name from the specified image structure. */
void writeFile(image *img, char *name) {
    uint16_t width = img->width;
    uint16_t length = img->height;
    
    uint8_t b1;
    uint16_t b2;
    uint32_t b4;

    /* uint8_t header = [(uint8_t)'I',(uint8_t)'I',] */

    /* header */
    FILE *fp = fopen(name,"wb");
    writeHeader(img,fp);

    fwrite(img->data, 1, img->datlen, fp);

    /* IFD */
    writeIFD(img,fp);
    fclose(fp);


}