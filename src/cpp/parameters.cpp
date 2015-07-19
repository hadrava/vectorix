#include <cstdio>
#include "parameters.h"

namespace vect {

params global_params;

void load_params(FILE *fd){
	global_params.input.pnm_input_name = "";
	global_params.vectorization_method = 2; // 0-custom, 1-potrace, 2-stupid
	global_params.step1.threshold_type = 0;
	global_params.step1.threshold = 127;
	global_params.step1.save_threshold_name = "out/threshold.png";

	global_params.step2.type = 0;
	global_params.step2.show_window = 1;
	global_params.step2.save_peeled_name = "out/skeletonization_%03d.png";
	global_params.step2.save_skeleton_name = "out/skeleton.png";
	global_params.step2.save_distance_name = "out/distance.png";
	global_params.step2.save_skeleton_normalized_name = "out/skeleton_norm.png";
	global_params.step2.save_distance_normalized_name = "out/distance_norm.png";
	global_params.step3.depth_auto_choose = 1; // error allowed (optimization)
	global_params.step3.max_dfs_depth = 1; // take first, no dfs allowed
	global_params.opencv_render.render_max_distance = 1;
	global_params.output.svg_output_name = "";
	global_params.output.pnm_output_name = "";
	global_params.interactive = 2;
}

void save_params(FILE *fd){
}


}; // namespace
