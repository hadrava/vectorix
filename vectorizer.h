/*
 * Vectorix -- line-based image vectorizer
 * (c) 2016 Jan Hadrava <had@atrey.karlin.mff.cuni.cz>
 */
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
	vectorizer(parameters &params): par(&params) {
		int *param_vectorizer_verbosity;
		par->bind_param(param_vectorizer_verbosity, "vectorizer_verbosity", (int) log_level::info);
		log.set_verbosity((log_level) *param_vectorizer_verbosity);
	};
	logger log;
	parameters *par;
};

class vectorizer_example: public vectorizer {
public:
	virtual v_image vectorize(const pnm_image &image);
	vectorizer_example(parameters &params): vectorizer(params) {};
};

}; // namespace

#endif
