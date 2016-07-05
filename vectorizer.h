#ifndef VECTORIX__VECTORIZER_H
#define VECTORIX__VECTORIZER_H

// Interface of all vectorizers

#include "pnm_handler.h"
#include "v_image.h"
#include <opencv2/opencv.hpp>

namespace vectorix {

class generic_vectorizer {
protected:
	static void vectorizer_error(const char *format, ...); // Simple printf-like functions
	static void vectorizer_info(const char *format, ...);
	static void vectorizer_debug(const char *format, ...);
};

class stupid : generic_vectorizer {
public:
	static v_image vectorize(const pnm_image &image, const params &parameters);
};

template <class Vectorizer = stupid>
class vectorizer {
public:
	static v_image run(const pnm_image &image, params &parameters) {
		return Vectorizer::vectorize(image, parameters);
	};
};


}; // namespace

#endif
