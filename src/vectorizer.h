#ifndef _VECTORIZER_H
#define _VECTORIZER_H

#include "pnm_handler.h"
#include "svg_handler.h"

struct svg_image *vectorize(const struct pnm_image * image);

#endif
