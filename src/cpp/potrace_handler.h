#ifndef _POTRACE_HANDLER_H
#define _POTRACE_HANDLER_H

#include "pnm_handler.h"
#include "v_image.h"
#include "vectorizer.h"

namespace vect {

class potrace : generic_vectorizer {
public:
	static vect::v_image vectorize(const pnm::pnm_image &image);
};

}; // namespace

#endif
