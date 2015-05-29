#ifndef _SVG_HANDLER_H
#define _SVG_HANDLER_H

#include <stdio.h>

struct svg_line {
	float opacity;
	float width;
	float x0,y0,x1,y1,x2,y2,x3,y3;
	struct svg_line *next_segment;
	struct svg_line *next;
};

struct svg_image {
	int width;
	int height;
	struct svg_line *data;
};

void svg_write(FILE *fd, const struct svg_image * image);
void svg_free(struct svg_image *image);

#endif
