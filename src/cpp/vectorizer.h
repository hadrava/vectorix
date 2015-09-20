#ifndef _VECTORIZER_H
#define _VECTORIZER_H

// Interface of all vectorizers

#include "pnm_handler.h"
#include "v_image.h"
#include <opencv2/opencv.hpp>

namespace vect {

class generic_vectorizer {
//protected: TODO
public:
	static void vectorizer_error(const char *format, ...); // Simple printf-like functions
	static void vectorizer_debug(const char *format, ...);
};

class stupid : generic_vectorizer {
public:
	static vect::v_image vectorize(const pnm::pnm_image &image);
};

template <class Vectorizer = stupid>
class vectorizer {
public:
	static vect::v_image run(const pnm::pnm_image &image) {
		return Vectorizer::vectorize(image);
	};
};


}; // namespace

#endif
