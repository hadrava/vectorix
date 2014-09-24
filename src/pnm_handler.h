#ifndef _PNM_HANDLER_H
#define _PNM_HANDLER_H

#include <stdio.h>

struct pnm_image {
	int type;
	int width;
	int height;
	int maxvalue;
	int *data;
};

struct pnm_image *pnm_read(FILE *fd);
int pnm_write(FILE *fd, const struct pnm_image *image);

#endif
