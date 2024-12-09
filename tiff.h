/* This library defines functions used to create and draw tiff images. This library supports 8-bit full color RGB, and 8 bit grayscale. */

#define SETCOLOR(IMG,R,G,B) { IMG->color[0] = R; IMG->color[1] = G; IMG->color[2] = B; }

#define GRAYSCALE 1
#define FULLCOLOR 3

typedef struct {
    uint16_t width; /* horizontal width of image */
    uint16_t height; /* vertical height of image */
    uint8_t color[3]; /* current color {red, green, blue}*/
    uint8_t *data; /* image pixel data expressed as a series of {r,g,b} triplets */
    uint32_t datlen; /* length of data in bytes */
    uint8_t bytesPerPixel;
    char type;
} image;

void setColor(image *img, uint8_t r, uint8_t g, uint8_t b); /* set current color to the RGB color r,g,b */

void fillColor(image *img); /* set entire image to current color */

void setPixel(image *img, int x, int y); /* set pixel (x,y) to current color */

void unsafeSetPixel(image *img, int x, int y); /* same as setPixel, but it does not protect against bad (x, y) pairs */

void orthoLine(image *img, int x, int y, char dir, int len); /* draws a vertical or horizontal line starting ad (x, y) in direction dir (0 = right, going counterclockwise) of length len */

void traceRect(image *img, int x1, int y1, int x2, int y2); /* traces the outline of the rectangle whose opposite corners are (x1, y1) and (x2, y2).*/

void fillRect(image *img, int x1, int y1, int x2, int y2);

image *iopen(uint16_t width, uint16_t height, int type); /* creates an image structure with the specified size */

void iclose(image *img); /* frees the allocated memory */

void writeIFDEntry(FILE *fp, uint16_t tag, uint16_t type, uint32_t valCount, uint32_t offset); /* writes an Image File Directory Entry. */

void writeHeader(image *img, FILE *fp);

void writeIFD(image *img, FILE *fp);

void writeFile(image *img, char *name); /* write the data stored in the given image structure to a .tiff file with the given name */