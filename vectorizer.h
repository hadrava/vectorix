#ifndef VECTORIX__VECTORIZER_H
#define VECTORIX__VECTORIZER_H

// Interface of all vectorizers

#include "pnm_handler.h"
#include "v_image.h"
#include "logger.h"
#include <opencv2/opencv.hpp>
#include "parameters.h"

namespace vectorix {

class vectorizer {
public:
	virtual v_image vectorize(const pnm_image &image) = 0;
protected:
	vectorizer(): log(log_level::debug), par(NULL) {};
	logger log;
	parameters *par;
};

class vectorizer_example: public vectorizer {
public:
	virtual v_image vectorize(const pnm_image &image);
	vectorizer_example(parameters &params) {
		par = &params;
	};
};

}; // namespace

#endif
