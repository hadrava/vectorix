#include <cstdio>
#include "parameters.h"
#include <cstdlib>
#include <cstring>

namespace vect {

params global_params;

void load_var(char *name, char *value, const char *my_name, int &data) {
	if (!strcmp(name, my_name)) {
		sscanf(value, "%d", &data);
	}
}

void load_var(char *name, char *value, const char *my_name, p &data) {
	if (!strcmp(name, my_name)) {
		sscanf(value, p_scanf, &data);
	}
}

void load_var(char *name, char *value, const char *my_name, std::string &data) {
	if (!strcmp(name, my_name)) {
		data = value;
	}
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
	return par;
}

int load_params(FILE *fd) {
	char *line;
	int count = 0;

	while (fscanf(fd, "%ms", &line) >= 0) {
		if (!line)
			continue;
		if (line[0] == '#')
			continue;

		char *name;
		char *value;
		sscanf(line, "%ms%ms", &name, &value);
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

		load_var(name, value, "pnm_input", global_params.input.pnm_input_name);
		load_var(name, value, "vectorization_method", global_params.vectorization_method);
		load_var(name, value, "threshold_type", global_params.step1.threshold_type);
		load_var(name, value, "threshold", global_params.step1.threshold);
		load_var(name, value, "file_threshold", global_params.step1.save_threshold_name);
		load_var(name, value, "type", global_params.step2.type);
		load_var(name, value, "show_window", global_params.step2.show_window);
		load_var(name, value, "file_peeled_template", global_params.step2.save_peeled_name);
		load_var(name, value, "file_skeleton", global_params.step2.save_skeleton_name);
		load_var(name, value, "file_distance", global_params.step2.save_distance_name);
		load_var(name, value, "file_skeleton_norm", global_params.step2.save_skeleton_normalized_name);
		load_var(name, value, "file_distance_norm", global_params.step2.save_distance_normalized_name);
		load_var(name, value, "depth_auto_choose", global_params.step3.depth_auto_choose);
		load_var(name, value, "max_dfs_depth", global_params.step3.max_dfs_depth);
		load_var(name, value, "render_max_distance", global_params.opencv_render.render_max_distance);
		load_var(name, value, "svg_output", global_params.output.svg_output_name);
		load_var(name, value, "pnm_output", global_params.output.pnm_output_name);
		load_var(name, value, "interactive", global_params.interactive);

		free(name);
		free(value);
		free(line);
	}
	return count;
}

int save_params(FILE *fd){
	fprintf(fd, "# General input pnm file\n");
	save_var(fd, "file_pnm_input", global_params.input.pnm_input_name);
	fprintf(fd, "# Vectorization method: 0: Custom, 1: Potrace, 2: Stupid\n");
	save_var(fd, "vectorization_method", global_params.vectorization_method);
	fprintf(fd, "# Phase 1: Thresholding\n");
	fprintf(fd, "# Threshold type: 0: Otsu's algorithm, 1: Fixed value\n");
	save_var(fd, "threshold_type", global_params.step1.threshold_type);
	fprintf(fd, "# Threshold vale: 0-255\n");
	save_var(fd, "threshold", global_params.step1.threshold);
	fprintf(fd, "# Save thresholded image to file: empty: no output\n");
	save_var(fd, "file_threshold_output", global_params.step1.save_threshold_name);
	fprintf(fd, "# Phase 2: Skeletonization");
	fprintf(fd, "# Skeletonization type: 0: diamond-square, 1: square, 2: diamond, 3: circle (slow)\n");
	save_var(fd, "type", global_params.step2.type);
	fprintf(fd, "# Show steps in separate window\n");
	save_var(fd, "show_window_steps", global_params.step2.show_window);
	fprintf(fd, "# Save steps to files, \%03d will be replaced with iteration number\n");
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
	fprintf(fd, "# Interactive mode: 0: disable, 1: show windows, 2: show trackbars");
	save_var(fd, "interactive", global_params.interactive);
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
