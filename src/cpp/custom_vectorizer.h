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

typedef struct {
	volatile int *state;
	int *adaptive_threshold_size;
} trackbar_refs;

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
	static void vectorize_imshow(const std::string& winname, const cv::Mat mat, const params &parameters); // Display image in window iff graphics (highgui) is enabled
	static int vectorize_waitKey(int delay = 0); // wait for key iff graphics (highgui) is enabled
	static void vectorize_destroyWindow(const std::string& winname); // Destroy named window
	static void add_to_skeleton(cv::Mat &out, cv::Mat &bw, int iteration); // Add pixels from `bw' to `out'. Something like image `or', but with more information
	static void normalize(cv::Mat &out, int max); // Normalize grayscale image for displaying (0-255)

	static void step1_threshold(cv::Mat &to_threshold, step1_params &par); // Threshold images
	static void step2_skeletonization(const cv::Mat &binary_input, cv::Mat &skeleton, cv::Mat &distance, int &iteration, params &par); // Find skeleton
	static void step3_tracing(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, v_image &vectorization_output, step3_params &par); // Trace skeleton
	static void trace_part(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, cv::Point startpoint, v_line &line, step3_params &par); // Trace one line

	// Trackbar callback functions
	static void step1_changed(int, void *ptr);
	static void step2_changed(int, void *ptr);

	static int interactive(int state, int key); // Process key press and decide what to do

	/*
	 * Functions for tracing
	 */
	static float do_prediction(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, const match_variant &last_placed, int allowed_depth, v_line &line, match_variant &new_point, step3_params &par);
	static void find_best_variant(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, const match_variant &last, const v_line &line, std::vector<match_variant> &match, step3_params &par);
	static void find_best_variant_first_point(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match, step3_params &par);
	static void find_best_variant_smooth(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match, step3_params &par);
	static void find_best_variant_straight(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match, step3_params &par);
	static void filter_best_variant_end(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match, step3_params &par);

	// Optimization of placed points
	static float find_best_line(const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt center, float angle, step3_params &par, float size, float min_dist = 0); // Find best line continuation in given angle
	static float calculate_line_fitness(const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt center, v_pt end, float min_dist, float max_dist, step3_params &par); // Calculate how 'good' is given line
	static v_pt find_best_gaussian(const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt center, step3_params &par, float size = 1); // Find best value in given area
	static float calculate_gaussian(const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt center, step3_params &par); // Get average value from neighborhood with gaussian distribution

	// Placing points
	static int place_next_point_at(const cv::Mat &skeleton, v_point &new_point, int current_depth, v_line &line, cv::Mat &used_pixels); // Add point to line and mark them as used
	static v_pt try_line_point(v_pt center, float angle, step3_params &par); // Return point in distance par.nearby_limit from center in given angle

	// Find starting points for lines
	static void prepare_starting_points(const cv::Mat &skeleton, std::vector<start_point> &starting_points); // Find all possible startingpoints
	static void find_max_starting_point(std::vector<start_point> &starting_points, const cv::Mat &used_pixels, int &max, cv::Point &max_pos); // Find first unused starting point from queue (std::vector)

	/*
	 * Functions for accesing image data
	 */
	static uchar nullpixel; // allways null pixel, returned by safeat when accessing pixels outside of an image
	static uchar &safeat(const cv::Mat &image, int i, int j); // Safe access image data for reading or writing
	static v_co apxat_co(const cv::Mat &image, v_pt pt); // Get rgb at non-integer position (aproximate from neighbors)
	static v_co safeat_co(const cv::Mat &image, int i, int j); // Safely access rgb image data
	static float apxat(const cv::Mat &image, v_pt pt); // Get value at non-integer position (aproximate from neighbors)

	static int inc_pix_to_near(const cv::Mat &mask, int value, const v_pt &point, cv::Mat &used_pixels, int near = 1); // Increase all pixels in neighborhood
	static int pix(const cv::Mat &img, const v_pt &point); // Get pix value
	static void spix(const cv::Mat &img, const v_pt &point, int value); // Set pix value
	static void spix(const cv::Mat &img, const cv::Point &point, int value); // Set pix value
	static int inc_pix_to(const cv::Mat &mask, int value, const v_pt &point, cv::Mat &used_pixels); // Increase pix value

	/*
	 * Speedup tracing using regions of interest
	 */
	static changed_pix_roi step3_changed; // Roi with pixels (value < 254)
	static changed_pix_roi step3_changed_start; // Roi with pixels (value == 254)
	static void step3_roi_clear(changed_pix_roi &roi, int x, int y); // remove all pixels from roi
	static cv::Rect step3_roi_get(const changed_pix_roi &roi); // Return OpenCV roi
	static void step3_roi_update(changed_pix_roi &roi, int x, int y); // Add pixel (x,y) to roi
};

}; // namespace

#endif
