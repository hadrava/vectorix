#include <opencv2/opencv.hpp>
#include "parameters.h"
#include "logger.h"
#include "thresholder.h"

using namespace cv;

namespace vectorix {

void thresholder::run(const Mat &original, Mat &bin) {
	max_image_size = (original.cols + original.rows) * 2;
	grayscale = Mat(original.rows, original.cols, CV_8UC(1)); // Grayscale original
	cvtColor(original, grayscale, CV_RGB2GRAY);

	// Invert black/white
	if (*param_invert_input)
		subtract(Scalar(255,255,255), grayscale, grayscale);


	//TODO is this needed?// bin = Mat(original.rows, original.cols, CV_8UC(1)); // Thresholded image
	// Do Thresholding
	if ((*param_threshold_type >= 2) && (*param_threshold_type <= 3)) { // Adaptive threshold
		log.log<log_level::info>("threshold: Using adaptive thresholdb with bias %i.\n", *param_threshold - 128);

		// Make threshold size odd and >= 3.
		*param_adaptive_threshold_size |= 1;
		if (*param_adaptive_threshold_size < 3)
			*param_adaptive_threshold_size = 3;

		int type = ADAPTIVE_THRESH_GAUSSIAN_C; // *param_threshold_type == 2 --> gaussian
		if (*param_threshold_type == 3)
			type = ADAPTIVE_THRESH_MEAN_C; // *param_threshold_type == 3 --> mean

		adaptiveThreshold(grayscale, bin, 255, type, THRESH_BINARY, *param_adaptive_threshold_size, *param_threshold - 128);
	}
	else if (*param_threshold_type == 1) { // Binary threshold, user-defined value
		log.log<log_level::info>("threshold: Using binary threshold %i.\n", *param_threshold);
		threshold(grayscale, bin, *param_threshold, 255, THRESH_BINARY);
	}
	else {
		if (*param_threshold_type != 0)
			log.log<log_level::warning>("threshold: Unknown threshold type specified (%i)!\n", *param_threshold_type);
		// Binary threshold, calculated value
		log.log<log_level::info>("threshold: Using Otsu's algorithm\n");
		threshold(grayscale, bin, *param_threshold, 255, THRESH_BINARY | THRESH_OTSU);
	}

	// Close objects (remove small holes in thicker lines)
	// TODO implement
	binary = bin;


	// Save image after thresholding
	if (!param_save_threshold_name->empty()) {
		imwrite(*param_save_threshold_name, binary);
	}
}

void thresholder::interactive(TrackbarCallback onChange, void *userdata) {
	/*// TODO vectorize_*/imshow("Grayscale", grayscale); // Show grayscale input image
	/*// TODO vectorize_*/imshow("Threshold", binary); // Show after thresholding

	createTrackbar("Invert input", "Grayscale", param_invert_input, 1, onChange, userdata);
	createTrackbar("Threshold type", "Threshold", param_threshold_type, 3, onChange, userdata);
	createTrackbar("Threshold", "Threshold", param_threshold, 255, onChange, userdata);
	createTrackbar("Adaptive threshold", "Threshold", param_adaptive_threshold_size, max_image_size, onChange, userdata);
	waitKey(1);
};

}; // namespace
