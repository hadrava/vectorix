#ifndef _PNM_HANDLER_H
#define _PNM_HANDLER_H

#include <stdio.h>

#define PNM_ASCII_PBM	1
#define PNM_ASCII_PGM	2
#define PNM_ASCII_PPM	3
#define PNM_BINARY_PBM	4
#define PNM_BINARY_PGM	5
#define PNM_BINARY_PPM	6

struct pnm_image {
	int type;
	int width;
	int height;
	int maxvalue;
	int *data;
};

struct pnm_image *pnm_read(FILE *fd);
int pnm_write(FILE *fd, const struct pnm_image *image);
struct pnm_image *pnm_convert(const struct pnm_image *image, int new_type);
void pnm_free(struct pnm_image *image);

#endif
