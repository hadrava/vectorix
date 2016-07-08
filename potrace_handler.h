#ifndef VECTORIX__POTRACE_HANDLER_H
#define VECTORIX__POTRACE_HANDLER_H

// Vectorize using potracelib

#include "pnm_handler.h"
#include "v_image.h"
#include "vectorizer.h"

namespace vectorix {

class vectorizer_potrace: public vectorizer {
public:
	virtual v_image vectorize(const pnm_image &image, params &parameters);
};


}; // namespace

#endif
