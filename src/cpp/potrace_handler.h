#ifndef _POTRACE_HANDLER_H
#define _POTRACE_HANDLER_H

#include "pnm_handler.h"
#include "v_image.h"

vect::v_image vectorize_potrace(const pnm::pnm_image &original);

#endif
