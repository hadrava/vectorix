#ifndef _VECTORIZER_H
#define _VECTORIZER_H

#include "pnm_handler.h"
#include "lines.h"

class v_image vectorize(const class pnm_image &image);
class v_image vectorize_bare(const class pnm_image &image);
class pnm_image render(const class v_image &vector);
void bezier_render(class pnm_image &bitmap, const class v_line &line);

#endif
