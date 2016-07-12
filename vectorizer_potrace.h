#ifndef VECTORIX__VECTORIZER_POTRACE_H
#define VECTORIX__VECTORIZER_POTRACE_H

// Vectorize using potracelib

#include "pnm_handler.h"
#include "v_image.h"
#include "vectorizer.h"

namespace vectorix {

class vectorizer_potrace: public vectorizer {
public:
	virtual v_image vectorize(const pnm_image &image);
	vectorizer_potrace(parameters &params): vectorizer(params) {};
};


}; // namespace

#endif
