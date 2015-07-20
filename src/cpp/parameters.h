#ifndef _PARAMETERS_H
#define _PARAMETERS_H

#include <string>
#include <cstdio>
#include "config.h"

namespace vect {

typedef struct {
	std::string pnm_input_name = "";
} input_params;

typedef struct {
	int threshold = 127;
	int threshold_type = 0; // 0=otsu, 1 fixed
	std::string save_threshold_name = "";
} step1_params;

typedef struct {
	int type = 0; // 0=fast diamod-square, 1=square, 2=diamond, 3=circle
	int show_window = 0; // 0 - no, 1 - yes
	std::string save_peeled_name = ""; // "out/boundary_%03d.png"
	std::string save_skeleton_name = "";
	std::string save_distance_name = "";
	std::string save_skeleton_normalized_name = "";
	std::string save_distance_normalized_name = "";
} step2_params;

typedef struct {
	float depth_auto_choose = 0;
	int max_dfs_depth = 1;
} step3_params;

typedef struct {
	p render_max_distance = 1;
} opencv_render_params;

typedef struct {
	std::string svg_output_name = "";
	std::string pnm_output_name = "";
	std::string save_opencv_rendered_name = "";
	int show_opencv_rendered_window = 0;
} output_params;

typedef struct {
	input_params input;
	int vectorization_method = 0; // 0-custom, 1-potrace, 2-stupid
	int interactive = 1; // 0-no, 1-window, 2-infinity
	step1_params step1;
	step2_params step2;
	step3_params step3;
	opencv_render_params opencv_render;
	output_params output;
	int last_changed_param_step = 1;
} params;

extern params global_params;

int load_params(FILE *fd);
int save_params(FILE *fd);

}; // namespace

#endif
