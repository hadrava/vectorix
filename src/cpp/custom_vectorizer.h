#ifndef _CUSTOM_VECTORIZER_H
#define _CUSTOM_VECTORIZER_H

// Vectorizer

#include <opencv2/opencv.hpp>
#include "pnm_handler.h"
#include "v_image.h"
#include "vectorizer.h"
#include "parameters.h"
#include <string>

namespace vect {

typedef struct { // Rectangle in which are all changed pixels
	int min_x;
	int min_y;
	int max_x;
	int max_y;
} changed_pix_roi;

typedef struct { // Structure for handle all unused starting points
	cv::Point pt; // Position in input image
	int val; // Distance to edge of object
} start_point;

enum variant_type { // State of traced point
	unset, // nothing special -- ordinary point
	start, // start of line -- all directions has same probability
	end // last point of a line -- we will refuse to continue
};

class match_variant { // Structure for storing one possible continuation
public:
	v_point pt; // Point in image
	variant_type type; // Tracing state
	float depth; // Prediction depth (for DFS-like searching)
	match_variant(): depth(0), type(unset) { pt = v_point(); };
	match_variant(v_point p): depth(1), type(unset), pt(p) {};
};

class custom : generic_vectorizer {
public:
	static vect::v_image vectorize(const pnm::pnm_image &image, params &parameters); // Run vectorization
private:
	static uchar nullpixel; // allways null pixel, returned by safeat when accessing pixels outside of an image
public:
	static uchar &safeat(const cv::Mat &image, int i, int j); // Safe access image data for reading or writing
private:
	static void vectorize_imshow(const std::string& winname, cv::InputArray mat, const params &parameters); // Display image in window iff graphics (highgui) is enabled
	static int vectorize_waitKey(int delay = 0); // wait for key iff graphics (highgui) is enabled
	static void vectorize_destroyWindow(const std::string& winname); // Destroy named window
	static void add_to_skeleton(cv::Mat &out, cv::Mat &bw, int iteration); // Add pixels from `bw' to `out'. Something like image `or', but with more information
	static void normalize(cv::Mat &out, int max); // Normalize grayscale image for displaying (0-255)
private:
	static void step1_threshold(cv::Mat &to_threshold, step1_params &par); // Threshold images
	static void step2_skeletonization(const cv::Mat &binary_input, cv::Mat &skeleton, cv::Mat &distance, int &iteration, params &par); // Find skeleton
	static void step3_tracing(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, v_image &vectorization_output, step3_params &par); // Trace skeleton
	static void trace_part(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, cv::Point startpoint, v_line &line, step3_params &par); // Trace one line
};


int pix(const cv::Mat &img, const v_pt &point); // Get pix value
void spix(const cv::Mat &img, const v_pt &point, int value); // Set pix value
void spix(const cv::Mat &img, const cv::Point &point, int value); // Set pix value
int inc_pix_to(const cv::Mat &mask, int value, const v_pt &point, cv::Mat &used_pixels); // Increase pix value

}; // namespace

#endif
