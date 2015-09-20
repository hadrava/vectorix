#ifndef _PARAMETERS_H
#define _PARAMETERS_H

// Program parameters

#include <string>
#include <cstdio>
#include "config.h"

namespace vect {

typedef struct { // Input reading
	std::string pnm_input_name = "";
	std::string custom_input_name = "";
} input_params;

typedef struct { // Custom vectorizer thresholding params
	int invert_input = 1;
	int threshold = 127;
	int threshold_type = 0; // 0=otsu, 1 fixed
	int adaptive_threshold_size = 7;
	std::string save_threshold_name = "";
} step1_params;

typedef struct { // Custom vectorizer skeleronization params
	int type = 0; // 0=fast diamod-square, 1=square, 2=diamond, 3=circle
	int show_window = 0; // 0 - no, 1 - yes
	std::string save_peeled_name = ""; // "out/boundary_%03d.png"
	std::string save_skeleton_name = "";
	std::string save_distance_name = "";
	std::string save_skeleton_normalized_name = "";
	std::string save_distance_normalized_name = "";
} step2_params;

typedef struct { // Custom vectorizer tracing params
	float depth_auto_choose = 0;
	int max_dfs_depth = 1;
	p nearby_limit = 10;
	p min_nearby_straight = 0; //TODO
	p size_nearby_smooth = 3; //TODO
	p nearby_control_smooth = 5; //TODO
	p max_angle_search_smooth = 0.8; //TODO
	p smoothness = 0.5; //TODO
	int nearby_limit_gauss = 2;
	float distance_coef = 2;
	float gauss_precision = 0.001;
	int angle_steps = 20;
	float angular_precision = 0.001;
} step3_params;

typedef struct { // Renderer params
	p render_max_distance = 1;
} opencv_render_params;

typedef struct { // Exporting params
	int export_type = 0;
	int output_engine = 0;
	p max_contour_error = 4;
	p auto_contour_variance = 1;
	std::string vector_output_name = "";
	std::string pnm_output_name = "";
	std::string save_opencv_rendered_name = "";
	std::string svg_underlay_image = "";
	p svg_force_opacity = 0;
	p svg_force_width = 0;
	p false_colors = 0;
	int show_opencv_rendered_window = 0;
} output_params;

typedef struct { // All parameters
	input_params input;
	int vectorization_method = 0; // 0-custom, 1-potrace, 2-stupid
	int interactive = 1; // 0-no, 1-window, 2-infinity
	step1_params step1;
	step2_params step2;
	step3_params step3;
	opencv_render_params opencv_render;
	output_params output;
	int save_parameters_append = 0;
	std::string save_parameters_name = "";
	int zoom_level = 0;
} params;

extern params global_params; //TODO should be deleted and moved to main only

params default_params(); // Default parameters
int load_params(FILE *fd, params &parameters); // Read from filedescriptor
int save_params(FILE *fd, const params &parameters); // Write to filedescriptor

}; // namespace

#endif
