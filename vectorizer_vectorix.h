#ifndef VECTORIX__VECTORIZER_VECTORIX_H
#define VECTORIX__VECTORIZER_VECTORIX_H

// Vectorizer

#include <opencv2/opencv.hpp>
#include "pnm_handler.h"
#include "v_image.h"
#include "vectorizer.h"
#include "parameters.h"
#include <string>

namespace vectorix {


class vectorizer_vectorix: public vectorizer {
public:
	virtual v_image vectorize(const pnm_image &image);
	vectorizer_vectorix(parameters &params): vectorizer(params) {
		par->bind_param(param_custom_input_name, "file_input", (std::string) "");

		par->add_comment("Interactive mode: 0: disable, 1: show windows and trackbars");
		par->bind_param(param_interactive, "interactive", 1);
	};
private:
	std::string *param_custom_input_name;
	int *param_interactive;

	// Trackbar callback functions (passed as parameter to non-member function)
	static void step1_changed(int, void *ptr);
	static void step2_changed(int, void *ptr);

	int interactive(int state, int key); // Process key press and decide what to do

	cv::Mat orig;
	cv::Mat binary;
	cv::Mat skeleton;
	cv::Mat distance;
	void load_image(const pnm_image &original);
};

}; // namespace

#endif
