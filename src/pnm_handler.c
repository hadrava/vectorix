#include "pnm_handler.h"
#include <stdio.h>
#include <stdlib.h>

struct pnm_image {
    int type;
    int width;
    int height;
    int maxvalue;
    void *data;
};

void pnm_error(char *message) {
    fprintf(stderr, message);
    return;
}

struct pnm_image *pnm_read_header(FILE *fd) {
    char magic, type;
    if (fscanf(fd, "%c%c", &magic, &type) != 2 || magic != 'P' || type < '1' || type > '6') {
        pnm_error("Error reading image header!\n");
	return NULL;
    }
    int width, height, maxvalue = 1;
    if (fscanf(fd, "%i%i", &width, &height) != 2) {
        pnm_error("Error reading image dimensions!\n");
	return NULL;
    }
    if (type != 1 && type != 4) {
        if (fscanf(fd, "%i", &maxvalue) != 1) {
            pnm_error("Error reading image maxvalue!\n");
	    return NULL;
        }
    }
    struct pnm_image *image = malloc(sizeof(struct pnm_image));
    image->type = type - '0';
    image->width = width;
    image->height = height;
    image->maxvalue = maxvalue;
    image->data = NULL;
}

struct pnm_image *pnm_read(FILE *fd) {
    struct pnm_image *image = pnm_read_header(fd);
    if (!image)
        return NULL;
    switch (image->type) {
      case 1:
        pnm_error("Error: reading ASCII PBM (black/white) unimplemented!\n");
	break;
      case 2:
        pnm_error("Error: reading ASCII PGM (gray scale) unimplemented!\n");
	break;
      case 3:
        pnm_error("Error: reading ASCII PPM (RGB) unimplemented!\n");
	break;
      case 4:
        pnm_error("Error: reading binary PBM (black/white) unimplemented!\n");
	break;
      case 5:
        pnm_error("Error: reading binary PGM (gray scale) unimplemented!\n");
	break;
      case 6:
        pnm_error("Error: reading binary PPM (RGB) unimplemented!\n");
	break;
     }
     return image;
}
