#ifndef _VECTORIZER_H
#define _VECTORIZER_H

#include "pnm_handler.h"
#include "svg_handler.h"

struct svg_image *vectorize(const struct pnm_image * image);
struct svg_image *vectorize_bare(const struct pnm_image * image);
void bezier_render(struct pnm_image * image, const struct svg_line *line);
void render(struct pnm_image *bitmap, const struct svg_image *vector);

#endif
