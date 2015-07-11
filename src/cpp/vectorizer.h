#ifndef _VECTORIZER_H
#define _VECTORIZER_H

#include "pnm_handler.h"
#include "lines.h"

class v_image vectorize(const class pnm_image &image);
class v_image vectorize_bare(const class pnm_image &image);

#endif
