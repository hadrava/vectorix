/*
 * Vectorix -- line-based image vectorizer
 * (c) 2016 Jan Hadrava <had@atrey.karlin.mff.cuni.cz>
 */
#ifndef VECTORIX__TRACER_H
#define VECTORIX__TRACER_H

#include <opencv2/opencv.hpp>
#include "parameters.h"
#include "v_image.h"
#include "logger.h"
#include "tracer_helper.h"
#include <vector>

namespace vectorix {

enum class variant_type { // State of traced point
	unset, // nothing special -- ordinary point
	start, // start of line -- all directions has same probability
	end // last point of a line -- we will refuse to continue
};

class match_variant { // Structure for storing one possible continuation
public:
	v_point pt; // Point in image
	variant_type type; // Tracing state
	p depth; // Prediction depth (for DFS-like searching)
	match_variant(): depth(0), type(variant_type::unset) { pt = v_point(); };
	match_variant(v_point point): depth(1), type(variant_type::unset), pt(point) {};
};

class tracer {
public:
	tracer(parameters &params): par(&params) {
		int *param_vectorizer_verbosity;
		par->bind_param(param_vectorizer_verbosity, "vectorizer_verbosity", (int) log_level::warning);
		log.set_verbosity((log_level) *param_vectorizer_verbosity);

		par->add_comment("Phase 3: Tracing");
		par->add_comment("Auto accept, higher values: slower tracing");
		par->bind_param(param_depth_auto_choose, "depth_auto_choose", (p) 1);
		par->add_comment("Maximal prediction depth");
		par->bind_param(param_max_dfs_depth, "max_dfs_depth", 1);
		par->add_comment("Maximal neighbourhood in pixel");
		par->bind_param(param_nearby_limit, "nearby_limit", (p) 10);
		par->add_comment("Maximal neighbourhood for calculating gaussian error in pixel");
		par->bind_param(param_nearby_limit_gauss, "nearby_limit_gauss", 2);
		par->add_comment("Coeficient for gaussian error");
		par->bind_param(param_distance_coef, "distance_coef", (p) 2);
		par->bind_param(param_gauss_precision, "gauss_precision", (p) 0.0001);
		par->bind_param(param_angle_steps, "angle_steps", 20);
		par->bind_param(param_angular_precision, "angular_precision", (p) 0.001);

		par->bind_param(param_size_nearby_smooth, "size_nearby_smooth", (p) 3);
		par->bind_param(param_max_angle_search_smooth, "max_angle_search_smooth", (p) 0.8);
		par->bind_param(param_nearby_control_smooth, "nearby_control_smooth", (p) 5);
		par->bind_param(param_smoothness, "smoothness", (p) 0.5);
		par->bind_param(param_min_nearby_straight, "param_min_nearby_straight", (p) 0);
	}
	void run(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, v_image &vectorization_output); // Trace skeleton
	//void interactive(cv::TrackbarCallback onChange = 0, void *userdata = 0);

private:
	p *param_depth_auto_choose;
	int *param_max_dfs_depth;
	p *param_nearby_limit;
	int *param_nearby_limit_gauss;
	p *param_distance_coef;
	p *param_gauss_precision;
	int *param_angle_steps;
	p *param_angular_precision;

	p *param_size_nearby_smooth;
	p *param_max_angle_search_smooth;
	p *param_nearby_control_smooth;
	p *param_smoothness;
	p *param_min_nearby_straight;

	void trace_part(cv::Point startpoint, v_line &line); // Trace one line

	/*
	 * Functions for tracing
	 */
	p do_prediction(const match_variant &last_placed, int allowed_depth, v_line &line, match_variant &new_point);
	void find_best_variant(const match_variant &last, const v_line &line, std::vector<match_variant> &match);
	void find_best_variant_first_point(v_pt last, const v_line &line, std::vector<match_variant> &match);
	void find_best_variant_smooth(v_pt last, const v_line &line, std::vector<match_variant> &match);
	void find_best_variant_straight(v_pt last, const v_line &line, std::vector<match_variant> &match);
	void filter_best_variant_end(v_pt last, const v_line &line, std::vector<match_variant> &match);

	// Optimization of placed points
	p find_best_line(v_pt center, p angle, p size, p min_dist = 0); // Find best line continuation in given angle
	p calculate_line_fitness(v_pt center, v_pt end, p min_dist, p max_dist); // Calculate how 'good' is given line
	v_pt find_best_gaussian(v_pt center, p size = 1); // Find best value in given area
	p calculate_gaussian(v_pt center); // Get average value from neighborhood with gaussian distribution

	// Placing points
	int place_next_point_at(v_point &new_point, int current_depth, v_line &line); // Add point to line and mark them as used
	v_pt try_line_point(v_pt center, p angle); // Return point in distance par.nearby_limit from center in given angle


	/*
	 * Functions for accesing image data
	 */
	int32_t nullpixel; // allways null pixel, cleared and returned by safeat when accessing pixels outside of an image

	v_co safeat_co(const cv::Mat &image, int i, int j); // Safely access rgb image data
	v_co apxat_co(const cv::Mat &image, v_pt pt); // Get rgb at non-integer position (aproximate from neighbors)
	const int32_t &safeat(const cv::Mat &image, int i, int j); // Safe access image data for reading or writing
	p apxat(const cv::Mat &image, v_pt pt); // Get value at non-integer position (aproximate from neighbors)

	logger log;
	parameters *par;

	// Pixels used by tracing are labeled
	labeled_Mat lab_skel;
	cv::Mat color;
	cv::Mat dist;
};

}; // namespace

#endif
