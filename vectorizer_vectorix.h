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

enum class variant_type { // State of traced point
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
	match_variant(): depth(0), type(variant_type::unset) { pt = v_point(); };
	match_variant(v_point p): depth(1), type(variant_type::unset), pt(p) {};
};

class vectorizer_vectorix: public vectorizer {
public:
	virtual v_image vectorize(const pnm_image &image);
	vectorizer_vectorix(parameters &params): vectorizer(params) {
		par->bind_param(param_custom_input_name, "file_input", (std::string) "");


		par->add_comment("Interactive mode: 0: disable, 1: show windows and trackbars");
		par->bind_param(param_interactive, "interactive", 2);
		par->add_comment("Scale images before viewing in window: 0: No scaling, 100: Small pictures");
		par->bind_param(param_zoom_level, "zoom_level", 0);




		par->add_comment("Phase 3: Tracing");
		par->add_comment("Auto accept, higher values: slower tracing");
		par->bind_param(param_depth_auto_choose, "depth_auto_choose", (float) 1);
		par->add_comment("Maximal prediction depth");
		par->bind_param(param_max_dfs_depth, "max_dfs_depth", 1);
		par->add_comment("Maximal neighbourhood in pixel");
		par->bind_param(param_nearby_limit, "nearby_limit", (p) 10);
		par->add_comment("Maximal neighbourhood for calculating gaussian error in pixel");
		par->bind_param(param_nearby_limit_gauss, "nearby_limit_gauss", 2);
		par->add_comment("Coeficient for gaussian error");
		par->bind_param(param_distance_coef, "distance_coef", (float) 2);
		par->bind_param(param_gauss_precision, "gauss_precision", (float) 0.0001);
		par->bind_param(param_angle_steps, "angle_steps", 20);
		par->bind_param(param_angular_precision, "angular_precision", (float) 0.001);

		par->bind_param(param_size_nearby_smooth, "size_nearby_smooth", (p) 3);
		par->bind_param(param_max_angle_search_smooth, "max_angle_search_smooth", (p) 0.8);
		par->bind_param(param_nearby_control_smooth, "nearby_control_smooth", (p) 5);
		par->bind_param(param_smoothness, "smoothness", (p) 0.5);
		par->bind_param(param_min_nearby_straight, "param_min_nearby_straight", (p) 0);
	};
private:
	std::string *param_custom_input_name;

	int *param_zoom_level;
	int *param_interactive;

	float *param_depth_auto_choose;
	int *param_max_dfs_depth;
	p *param_nearby_limit;
	int *param_nearby_limit_gauss;
	float *param_distance_coef;
	float *param_gauss_precision;
	int *param_angle_steps;
	float *param_angular_precision;

	p *param_size_nearby_smooth;
	p *param_max_angle_search_smooth;
	p *param_nearby_control_smooth;
	p *param_smoothness;
	p *param_min_nearby_straight;


	void vectorize_imshow(const std::string& winname, const cv::Mat mat); // Display image in window iff graphics (highgui) is enabled
	int vectorize_waitKey(int delay = 0); // wait for key iff graphics (highgui) is enabled
	void vectorize_destroyWindow(const std::string& winname); // Destroy named window

	void step2_skeletonization(const cv::Mat &binary_input, cv::Mat &skeleton, cv::Mat &distance, int &iteration); // Find skeleton
	void step3_tracing(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, v_image &vectorization_output); // Trace skeleton
	void trace_part(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, cv::Point startpoint, v_line &line); // Trace one line

	// Trackbar callback functions (passed as parameter to non-member function)
	static void step1_changed(int, void *ptr);
	static void step2_changed(int, void *ptr);

	int interactive(int state, int key); // Process key press and decide what to do

	/*
	 * Functions for tracing
	 */
	float do_prediction(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, const match_variant &last_placed, int allowed_depth, v_line &line, match_variant &new_point);
	void find_best_variant(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, const match_variant &last, const v_line &line, std::vector<match_variant> &match);
	void find_best_variant_first_point(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match);
	void find_best_variant_smooth(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match);
	void find_best_variant_straight(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match);
	void filter_best_variant_end(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match);

	// Optimization of placed points
	float find_best_line(const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt center, float angle, float size, float min_dist = 0); // Find best line continuation in given angle
	float calculate_line_fitness(const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt center, v_pt end, float min_dist, float max_dist); // Calculate how 'good' is given line
	v_pt find_best_gaussian(const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt center, float size = 1); // Find best value in given area
	float calculate_gaussian(const cv::Mat &skeleton, const cv::Mat &distance, const cv::Mat &used_pixels, v_pt center); // Get average value from neighborhood with gaussian distribution

	// Placing points
	int place_next_point_at(const cv::Mat &skeleton, v_point &new_point, int current_depth, v_line &line, cv::Mat &used_pixels); // Add point to line and mark them as used
	v_pt try_line_point(v_pt center, float angle); // Return point in distance par.nearby_limit from center in given angle

	// Find starting points for lines
	void prepare_starting_points(const cv::Mat &skeleton, std::vector<start_point> &starting_points); // Find all possible startingpoints
	void find_max_starting_point(std::vector<start_point> &starting_points, const cv::Mat &used_pixels, int &max, cv::Point &max_pos); // Find first unused starting point from queue (std::vector)

	/*
	 * Functions for accesing image data
	 */
	uchar nullpixel; // allways null pixel, cleared and returned by safeat when accessing pixels outside of an image

	uchar &safeat(const cv::Mat &image, int i, int j); // Safe access image data for reading or writing
	v_co apxat_co(const cv::Mat &image, v_pt pt); // Get rgb at non-integer position (aproximate from neighbors)
	v_co safeat_co(const cv::Mat &image, int i, int j); // Safely access rgb image data
	float apxat(const cv::Mat &image, v_pt pt); // Get value at non-integer position (aproximate from neighbors)

	int inc_pix_to_near(const cv::Mat &mask, int value, const v_pt &point, cv::Mat &used_pixels, int near = 1); // Increase all pixels in neighborhood
	int pix(const cv::Mat &img, const v_pt &point); // Get pix value
	void spix(const cv::Mat &img, const v_pt &point, int value); // Set pix value
	void spix(const cv::Mat &img, const cv::Point &point, int value); // Set pix value
	int inc_pix_to(const cv::Mat &mask, int value, const v_pt &point, cv::Mat &used_pixels); // Increase pix value

	/*
	 * Speedup tracing using regions of interest
	 */

	changed_pix_roi step3_changed; // Roi with pixels (value < 254)
	changed_pix_roi step3_changed_start; // Roi with pixels (value == 254)

	void step3_roi_clear(changed_pix_roi &roi, int x, int y); // remove all pixels from roi
	cv::Rect step3_roi_get(const changed_pix_roi &roi); // Return OpenCV roi
	void step3_roi_update(changed_pix_roi &roi, int x, int y); // Add pixel (x,y) to roi
};

}; // namespace

#endif
