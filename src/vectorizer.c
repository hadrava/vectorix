#include "svg_handler.h"
#include "pnm_handler.h"
#include "vectorizer.h"
#include <stdlib.h>

struct svg_image *vectorize(const struct pnm_image * image) {
	struct svg_image * vect = malloc(sizeof(const struct svg_image));
	vect->width = image->width;
	vect->height = image->height;
	//vect->data = NULL;
	vect->data = malloc(sizeof(const struct svg_line));
	vect->data->width = 1.0;
	vect->data->opacity = 1.0;
	vect->data->x0 = 0;
	vect->data->y0 = 0;
	vect->data->x1 = 10;
	vect->data->y1 = 0;
	vect->data->x2 = 100;
	vect->data->y2 = 50;
	vect->data->x3 = 100;
	vect->data->y3 = 100;
	vect->data->next = NULL;

	return vect;
}
