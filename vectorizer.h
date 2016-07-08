#ifndef VECTORIX__VECTORIZER_H
#define VECTORIX__VECTORIZER_H

// Interface of all vectorizers

#include "pnm_handler.h"
#include "v_image.h"
#include "logger.h"
#include <opencv2/opencv.hpp>

namespace vectorix {

class vectorizer {
public:
	virtual v_image vectorize(const pnm_image &image, params &parameters) = 0;
	vectorizer(): log(log_level::debug) {};
	//TODO delete;
protected:
	logger log;
};

class vectorizer_example: public vectorizer {
public:
	virtual v_image vectorize(const pnm_image &image, params &parameters);
};

}; // namespace

#endif
