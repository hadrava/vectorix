#include <cstdio>
#include "parameters.h"
#include <cstdlib>
#include <cstring>

namespace vect {

params global_params;

int load_var(char *name, char *value, const char *my_name, int &data) {
	if (!strcmp(name, my_name)) {
		sscanf(value, "%d", &data);
		return 1;
	}
	return 0;
}

int load_var(char *name, char *value, const char *my_name, p &data) {
	if (!strcmp(name, my_name)) {
		sscanf(value, p_scanf, &data);
		return 1;
	}
	return 0;
}

int load_var(char *name, char *value, const char *my_name, std::string &data) {
	if (!strcmp(name, my_name)) {
		data = value;
		return 1;
	}
	return 0;
}

void save_var(FILE *fd, const char *name, int data) {
	fprintf(fd, "%s %d\n", name, data);
}

void save_var(FILE *fd, const char *name, p data) {
	fprintf(fd, "%s ", name);
	fprintf(fd, p_printf, data);
	fprintf(fd, "\n");
}

void save_var(FILE *fd, const char *name, const std::string &data) {
	fprintf(fd, "%s %s\n", name, data.c_str());
}

params default_params() {
	params par;
	par.step1.invert_input = 1;
	par.step1.threshold_type = 0;
	par.step1.threshold = 127;
	par.step1.adaptive_threshold_size = 7;
	par.step2.type = 0;
	par.step2.show_window = 1;
	par.step2.save_peeled_name = "out/skeletonization_%03d.png";
	par.step2.save_skeleton_name = "out/skeleton.png";
	par.step2.save_distance_name = "out/distance.png";
	par.step2.save_skeleton_normalized_name = "out/skeleton_norm.png";
	par.step2.save_distance_normalized_name = "out/distance_norm.png";
	par.step3.depth_auto_choose = 1; // error allowed (optimization)
	par.step3.max_dfs_depth = 1; // take first, no dfs allowed
	par.opencv_render.render_max_distance = 1;
	par.output.svg_output_name = "";
	par.output.pnm_output_name = "";
	par.interactive = 2;
	par.save_parameters_append = 0;
	par.save_parameters_name = ".vector.params";
	return par;
}

int load_params(FILE *fd) {
	char *line;
	int count = 0;
	int linenumber = 0;

	while (fscanf(fd, "%m[^\n]\n", &line) >= 0) {
		linenumber++;
		if (!line)
			continue;
		if (line[0] == '#')
			continue;

		char *name;
		char *value;
		sscanf(line, "%m[^ ] %m[^\n]", &name, &value);
		if ((!name) && (!value)) {
			free(line);
			continue;
		}
		else if ((!name) && (value)) {
			free(value);
			free(line);
			continue;
		}
		else if ((name) && (!value)) {
			free(name);
			free(line);
			continue;
		}

		int loaded = 0;
		loaded += load_var(name, value, "file_pnm_input", global_params.input.pnm_input_name);
		loaded += load_var(name, value, "vectorization_method", global_params.vectorization_method);
		loaded += load_var(name, value, "invert_colors", global_params.step1.invert_input);
		loaded += load_var(name, value, "threshold_type", global_params.step1.threshold_type);
		loaded += load_var(name, value, "threshold", global_params.step1.threshold);
		loaded += load_var(name, value, "adaptive_threshold_size", global_params.step1.adaptive_threshold_size);
		loaded += load_var(name, value, "file_threshold_output", global_params.step1.save_threshold_name);
		loaded += load_var(name, value, "type", global_params.step2.type);
		loaded += load_var(name, value, "show_window_steps", global_params.step2.show_window);
		loaded += load_var(name, value, "files_steps_output", global_params.step2.save_peeled_name);
		loaded += load_var(name, value, "file_skeleton", global_params.step2.save_skeleton_name);
		loaded += load_var(name, value, "file_distance", global_params.step2.save_distance_name);
		loaded += load_var(name, value, "file_skeleton_norm", global_params.step2.save_skeleton_normalized_name);
		loaded += load_var(name, value, "file_distance_norm", global_params.step2.save_distance_normalized_name);
		loaded += load_var(name, value, "depth_auto_choose", global_params.step3.depth_auto_choose);
		loaded += load_var(name, value, "max_dfs_depth", global_params.step3.max_dfs_depth);
		loaded += load_var(name, value, "render_max_distance", global_params.opencv_render.render_max_distance);
		loaded += load_var(name, value, "file_svg_output", global_params.output.svg_output_name);
		loaded += load_var(name, value, "file_pnm_output", global_params.output.pnm_output_name);
		loaded += load_var(name, value, "file_opencv_output", global_params.output.pnm_output_name);
		loaded += load_var(name, value, "interactive", global_params.interactive);
		loaded += load_var(name, value, "parameters_append", global_params.save_parameters_append);
		loaded += load_var(name, value, "file_parameters", global_params.save_parameters_name);
		if (loaded != 1) {
			fprintf(stderr, "Error parsing config file on line %d: \"%s\"\n", linenumber, line);
		}

		free(name);
		free(value);
		free(line);
	}
	return count;
}

int save_params(FILE *fd){
	fprintf(fd, "###########################################################\n");
	fprintf(fd, "# General input pnm file\n");
	save_var(fd, "file_pnm_input", global_params.input.pnm_input_name);
	fprintf(fd, "# Vectorization method: 0: Custom, 1: Potrace, 2: Stupid\n");
	save_var(fd, "vectorization_method", global_params.vectorization_method);
	fprintf(fd, "# Phase 1: Thresholding\n");
	fprintf(fd, "# Invert colors: 0: white lines, 1: black lines\n");
	save_var(fd, "invert_colors", global_params.step1.invert_input);
	fprintf(fd, "# Threshold type: 0: Otsu's algorithm, 1: Fixed value\n");
	save_var(fd, "threshold_type", global_params.step1.threshold_type);
	fprintf(fd, "# Threshold vale: 0-255\n");
	save_var(fd, "threshold", global_params.step1.threshold);
	fprintf(fd, "# Adaptive threshold size: 3, 5, 7, ...\n");
	save_var(fd, "adaptive_threshold_size", global_params.step1.adaptive_threshold_size);
	fprintf(fd, "# Save thresholded image to file: empty: no output\n");
	save_var(fd, "file_threshold_output", global_params.step1.save_threshold_name);
	fprintf(fd, "# Phase 2: Skeletonization");
	fprintf(fd, "# Skeletonization type: 0: diamond-square, 1: square, 2: diamond, 3: circle (slow)\n");
	save_var(fd, "type", global_params.step2.type);
	fprintf(fd, "# Show steps in separate window\n");
	save_var(fd, "show_window_steps", global_params.step2.show_window);
	fprintf(fd, "# Save steps to files, %%03d will be replaced with iteration number\n");
	save_var(fd, "files_steps_output", global_params.step2.save_peeled_name);
	fprintf(fd, "# Save skeleton/distance with/without normalization\n");
	save_var(fd, "file_skeleton", global_params.step2.save_skeleton_name);
	save_var(fd, "file_distance", global_params.step2.save_distance_name);
	save_var(fd, "file_skeleton_norm", global_params.step2.save_skeleton_normalized_name);
	save_var(fd, "file_distance_norm", global_params.step2.save_distance_normalized_name);
	fprintf(fd, "# Phase 3: Tracing\n");
	fprintf(fd, "# Auto accept, higher values: slower tracing\n");
	save_var(fd, "depth_auto_choose", global_params.step3.depth_auto_choose);
	fprintf(fd, "# Maximal prediction depth\n");
	save_var(fd, "max_dfs_depth", global_params.step3.max_dfs_depth);
	fprintf(fd, "# Phase 5: Export\n");
	fprintf(fd, "# Maximal allowed error in OpenCV rendering in pixels\n");
	save_var(fd, "render_max_distance", global_params.opencv_render.render_max_distance);
	fprintf(fd, "# Save output to files\n");
	save_var(fd, "file_svg_output", global_params.output.svg_output_name);
	save_var(fd, "file_pnm_output", global_params.output.pnm_output_name);
	save_var(fd, "file_opencv_output", global_params.output.pnm_output_name);
	fprintf(fd, "# Interactive mode: 0: disable, 1: show windows, 2: show trackbars\n");
	save_var(fd, "interactive", global_params.interactive);
	fprintf(fd, "# Save parameters to file on exit: 0: overwrite, 1: append\n");
	save_var(fd, "parameters_append", global_params.save_parameters_append);
	save_var(fd, "file_parameters", global_params.save_parameters_name);
	fprintf(fd, "###########################################################\n");
	return 0;
}

int load_params(const std::string filename) {
	FILE *fd;
	if ((fd = fopen(filename.c_str(), "r")) == NULL) {
		fprintf(stderr, "Couldn't open config file for reading.\n");
		return 0;
	}
	int loaded = load_params(fd);
	fclose(fd);
	return loaded;
}

int save_params(const std::string filename) {
	FILE *fd;
	if ((fd = fopen(filename.c_str(), "w")) == NULL) {
		fprintf(stderr, "Couldn't open config file for writing.\n");
		return 1;
	}
	int saved = save_params(fd);
	fclose(fd);
	return saved;
}

}; // namespace
