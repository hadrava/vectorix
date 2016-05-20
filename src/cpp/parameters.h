#ifndef _PARAMETERS_H
#define _PARAMETERS_H

// Program parameters

#include <string>
#include <cstdio>
#include "config.h"

namespace vectorix {

typedef struct { // Input reading
	std::string pnm_input_name;
	std::string custom_input_name;
} input_params;

typedef struct { // Custom vectorizer thresholding params
	int invert_input;
	int threshold;
	int threshold_type; // 0=otsu, 1 fixed, 2 adaptive
	int adaptive_threshold_size;
	std::string save_threshold_name;
} step1_params;

typedef struct { // Custom vectorizer skeleronization params
	int type; // 0=fast diamod-square, 1=square, 2=diamond, 3=circle
	int show_window; // 0 - no, 1 - yes
	std::string save_peeled_name; // "out/boundary_%03d.png"
	std::string save_skeleton_name;
	std::string save_distance_name;
	std::string save_skeleton_normalized_name;
	std::string save_distance_normalized_name;
} step2_params;

typedef struct { // Custom vectorizer tracing params
	float depth_auto_choose;
	int max_dfs_depth;
	p nearby_limit;
	p min_nearby_straight;
	p size_nearby_smooth;
	p nearby_control_smooth;
	p max_angle_search_smooth;
	p smoothness;
	int nearby_limit_gauss;
	float distance_coef;
	float gauss_precision;
	int angle_steps;
	float angular_precision;
} step3_params;

typedef struct { // Renderer params
	p render_max_distance;
} opencv_render_params;

typedef struct { // Exporting params
	int export_type;
	int output_engine;
	p max_contour_error;
	p auto_contour_variance;
	std::string vector_output_name;
	std::string pnm_output_name;
	std::string save_opencv_rendered_name;
	std::string svg_underlay_image;
	p svg_force_opacity;
	p svg_force_width;
	p false_colors;
	int show_opencv_rendered_window;
} output_params;

class params { // All parameters
public:
	input_params input;
	int vectorization_method; // 0-custom, 1-potrace, 2-stupid
	int interactive; // 0-no, 1-window, 2-infinity
	step1_params step1;
	step2_params step2;
	step3_params step3;
	opencv_render_params opencv_render;
	output_params output;
	int save_parameters_append;
	std::string save_parameters_name;
	int zoom_level;

	params();
	int load_params(FILE *fd); // Read from filedescriptor
	int load_params(const std::string filename); // Load parameters from file given by name
	int save_params(FILE *fd) const; // Write to filedescriptor
	int save_params(const std::string filename) const; // Save parameters to file given by name
private:
	static int load_var(const char *name, const char *value, const char *my_name, int &data); // Load integer
	static int load_var(const char *name, const char *value, const char *my_name, p &data); // Load p type (float)
	static int load_var(const char *name, const char *value, const char *my_name, std::string &data); // Load string
	static void save_var(FILE *fd, const char *name, int data); // Save integer
	static void save_var(FILE *fd, const char *name, p data); // Save p type (float)
	static void save_var(FILE *fd, const char *name, const std::string &data); // Save string
};

}; // namespace

#endif
