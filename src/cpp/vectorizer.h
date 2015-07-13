#ifndef _VECTORIZER_H
#define _VECTORIZER_H

#include "pnm_handler.h"
#include "v_image.h"

vect::v_image vectorize(const pnm::pnm_image &image);
vect::v_image vectorize_bare(const pnm::pnm_image &image);

#endif
