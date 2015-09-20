#include <cstdio>
#include "parameters.h"
#include <cstdlib>
#include <cstring>

// Program parameters

namespace vect {

params global_params; //TODO delete

int load_var(char *name, char *value, const char *my_name, int &data) { // Load integer
	if (!strcmp(name, my_name)) {
		sscanf(value, "%d", &data);
		return 1;
	}
	return 0;
}

int load_var(char *name, char *value, const char *my_name, p &data) { // Load p type (float)
	if (!strcmp(name, my_name)) {
		sscanf(value, p_scanf, &data);
		return 1;
	}
	return 0;
}

int load_var(char *name, char *value, const char *my_name, std::string &data) { // Load string
	if (!strcmp(name, my_name)) {
		data = value;
		return 1;
	}
	return 0;
}

void save_var(FILE *fd, const char *name, int data) { // Save integer
	fprintf(fd, "%s %d\n", name, data);
}

void save_var(FILE *fd, const char *name, p data) { // Save p type (float)
	fprintf(fd, "%s ", name);
	fprintf(fd, p_printf, data);
	fprintf(fd, "\n");
}

void save_var(FILE *fd, const char *name, const std::string &data) { // Save string
	fprintf(fd, "%s %s\n", name, data.c_str());
}

params default_params() { // Default parameters
	params par;
	par.input.pnm_input_name = "";
	par.input.custom_input_name = "";
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
	par.step3.nearby_limit = 10;
	par.step3.nearby_limit_gauss = 2;
	par.step3.distance_coef = 2;
	par.step3.gauss_precision = 0.001;
	par.step3.angle_steps = 20;
	par.step3.angular_precision = 0.001;
	//par.step3. = ;
	par.opencv_render.render_max_distance = 1;
	par.output.export_type = 3;
	par.output.output_engine = 0;
	par.output.max_contour_error = 0.5;
	par.output.auto_contour_variance = 5;
	par.output.vector_output_name = "";
	par.output.pnm_output_name = "";
	par.output.save_opencv_rendered_name = "";
	par.output.svg_underlay_image = "";
	par.output.svg_force_opacity = 0;
	par.output.svg_force_width = 0;
	par.output.false_colors = 0;
	//par.output. = "";
	par.interactive = 2;
	par.zoom_level = 0;
	par.save_parameters_append = 0;
	par.save_parameters_name = ".vector.params";
	return par;
}

int load_params(FILE *fd, params &parameters) { // Load parameters from file
	char *line;
	int count = 0;
	int linenumber = 0;

	while (fscanf(fd, "%m[^\n]\n", &line) >= 0) {
		linenumber++;
		if (!line) // skip empty line
			continue;
		if (line[0] == '#') // skip commented line
			continue; //TODO free line

		char *name;
		char *value;
		sscanf(line, "%m[^ ] %m[^\n]", &name, &value); // Split by first space
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
		// Everything is ok

		int loaded = 0;
		// Try to load every known parameter
		loaded += load_var(name, value, "file_pnm_input", parameters.input.pnm_input_name);
		loaded += load_var(name, value, "file_input", parameters.input.custom_input_name);
		loaded += load_var(name, value, "vectorization_method", parameters.vectorization_method);
		loaded += load_var(name, value, "invert_colors", parameters.step1.invert_input);
		loaded += load_var(name, value, "threshold_type", parameters.step1.threshold_type);
		loaded += load_var(name, value, "threshold", parameters.step1.threshold);
		loaded += load_var(name, value, "adaptive_threshold_size", parameters.step1.adaptive_threshold_size);
		loaded += load_var(name, value, "file_threshold_output", parameters.step1.save_threshold_name);
		loaded += load_var(name, value, "type", parameters.step2.type);
		loaded += load_var(name, value, "show_window_steps", parameters.step2.show_window);
		loaded += load_var(name, value, "files_steps_output", parameters.step2.save_peeled_name);
		loaded += load_var(name, value, "file_skeleton", parameters.step2.save_skeleton_name);
		loaded += load_var(name, value, "file_distance", parameters.step2.save_distance_name);
		loaded += load_var(name, value, "file_skeleton_norm", parameters.step2.save_skeleton_normalized_name);
		loaded += load_var(name, value, "file_distance_norm", parameters.step2.save_distance_normalized_name);
		loaded += load_var(name, value, "depth_auto_choose", parameters.step3.depth_auto_choose);
		loaded += load_var(name, value, "max_dfs_depth", parameters.step3.max_dfs_depth);
		loaded += load_var(name, value, "nearby_limit", parameters.step3.nearby_limit);
		loaded += load_var(name, value, "nearby_limit_gauss", parameters.step3.nearby_limit_gauss);
		loaded += load_var(name, value, "distance_coef", parameters.step3.distance_coef);
		loaded += load_var(name, value, "gauss_precision", parameters.step3.gauss_precision);
		loaded += load_var(name, value, "angle_steps", parameters.step3.angle_steps);
		loaded += load_var(name, value, "angular_precision", parameters.step3.angular_precision);
		//loaded += load_var(name, value, "", parameters.step3.);
		loaded += load_var(name, value, "render_max_distance", parameters.opencv_render.render_max_distance);
		loaded += load_var(name, value, "export_type", parameters.output.export_type);
		loaded += load_var(name, value, "max_contour_error", parameters.output.max_contour_error);
		loaded += load_var(name, value, "auto_contour_variance", parameters.output.auto_contour_variance);
		loaded += load_var(name, value, "output_engine", parameters.output.output_engine);
		loaded += load_var(name, value, "file_vector_output", parameters.output.vector_output_name);
		loaded += load_var(name, value, "file_pnm_output", parameters.output.pnm_output_name);
		loaded += load_var(name, value, "file_opencv_output", parameters.output.save_opencv_rendered_name);
		loaded += load_var(name, value, "svg_underlay_image", parameters.output.svg_underlay_image);
		loaded += load_var(name, value, "svg_force_opacity", parameters.output.svg_force_opacity);
		loaded += load_var(name, value, "svg_force_width", parameters.output.svg_force_width);
		loaded += load_var(name, value, "false_colors", parameters.output.false_colors);
		//loaded += load_var(name, value, "", parameters.output.);
		loaded += load_var(name, value, "interactive", parameters.interactive);
		loaded += load_var(name, value, "zoom_level", parameters.zoom_level);
		loaded += load_var(name, value, "parameters_append", parameters.save_parameters_append);
		loaded += load_var(name, value, "file_parameters", parameters.save_parameters_name);
		if (loaded != 1) { // Detecting was unsuccesfull, warn user
			fprintf(stderr, "Error parsing config file on line %d: \"%s\"\n", linenumber, line);
		}

		free(name);
		free(value);
		free(line);
	}
	return count;
}

int save_params(FILE *fd, const params &parameters){ // Write parameters (with simple help) to opened filedescriptor
	fprintf(fd, "###########################################################\n");
	fprintf(fd, "# General input pnm file\n");
	save_var(fd, "file_pnm_input", parameters.input.pnm_input_name);
	fprintf(fd, "# Input from any file format supported by OpenCV (only with Custom vectorization method, this option overrides pnm_input):\n");
	save_var(fd, "file_input", parameters.input.custom_input_name);
	fprintf(fd, "# Vectorization method: 0: Custom, 1: Potrace, 2: Stupid\n");
	save_var(fd, "vectorization_method", parameters.vectorization_method);
	fprintf(fd, "# Phase 1: Thresholding\n");
	fprintf(fd, "# Invert colors: 0: white lines, 1: black lines\n");
	save_var(fd, "invert_colors", parameters.step1.invert_input);
	fprintf(fd, "# Threshold type: 0: Otsu's algorithm, 1: Fixed value\n");
	save_var(fd, "threshold_type", parameters.step1.threshold_type);
	fprintf(fd, "# Threshold vale: 0-255\n");
	save_var(fd, "threshold", parameters.step1.threshold);
	fprintf(fd, "# Adaptive threshold size: 3, 5, 7, ...\n");
	save_var(fd, "adaptive_threshold_size", parameters.step1.adaptive_threshold_size);
	fprintf(fd, "# Save thresholded image to file: empty: no output\n");
	save_var(fd, "file_threshold_output", parameters.step1.save_threshold_name);
	fprintf(fd, "# Phase 2: Skeletonization");
	fprintf(fd, "# Skeletonization type: 0: diamond-square, 1: square, 2: diamond, 3: circle (slow)\n");
	save_var(fd, "type", parameters.step2.type);
	fprintf(fd, "# Show steps in separate window\n");
	save_var(fd, "show_window_steps", parameters.step2.show_window);
	fprintf(fd, "# Save steps to files, %%03d will be replaced with iteration number\n");
	save_var(fd, "files_steps_output", parameters.step2.save_peeled_name);
	fprintf(fd, "# Save skeleton/distance with/without normalization\n");
	save_var(fd, "file_skeleton", parameters.step2.save_skeleton_name);
	save_var(fd, "file_distance", parameters.step2.save_distance_name);
	save_var(fd, "file_skeleton_norm", parameters.step2.save_skeleton_normalized_name);
	save_var(fd, "file_distance_norm", parameters.step2.save_distance_normalized_name);
	fprintf(fd, "# Phase 3: Tracing\n");
	fprintf(fd, "# Auto accept, higher values: slower tracing\n");
	save_var(fd, "depth_auto_choose", parameters.step3.depth_auto_choose);
	fprintf(fd, "# Maximal prediction depth\n");
	save_var(fd, "max_dfs_depth", parameters.step3.max_dfs_depth);
	fprintf(fd, "# Maximal neighbourhood in pixel\n");
	save_var(fd, "nearby_limit", parameters.step3.nearby_limit);
	fprintf(fd, "# Maximal neighbourhood for calculating gaussian error in pixel\n");
	save_var(fd, "nearby_limit_gauss", parameters.step3.nearby_limit_gauss);
	fprintf(fd, "# Coeficient for gaussian error\n");
	save_var(fd, "distance_coef", parameters.step3.distance_coef);
	save_var(fd, "gauss_precision", parameters.step3.gauss_precision);
	save_var(fd, "angle_steps", parameters.step3.angle_steps);
	save_var(fd, "angular_precision", parameters.step3.angular_precision);
	//save_var(fd, "", parameters.step3.);
	fprintf(fd, "# Phase 5: Export\n");
	fprintf(fd, "# Maximal allowed error in OpenCV rendering in pixels\n");
	save_var(fd, "render_max_distance", parameters.opencv_render.render_max_distance);
	fprintf(fd, "# Save output to files\n");
	fprintf(fd, "# Variable-width export: 0: mean, 1: grouped, 2: contour, 3: auto-detect\n");
	save_var(fd, "export_type", parameters.output.export_type);
	fprintf(fd, "# Precision of contour output (maximal allowed error in pixels):\n");
	save_var(fd, "max_contour_error", parameters.output.max_contour_error);
	fprintf(fd, "# How often draw as contour: higher values: less often, lower: more often, negative: always use contours\n");
	save_var(fd, "auto_contour_variance", parameters.output.auto_contour_variance);
	fprintf(fd, "# Output file format: 0: svg, 1: ps\n");
	save_var(fd, "output_engine", parameters.output.output_engine);
	save_var(fd, "file_vector_output", parameters.output.vector_output_name);
	save_var(fd, "file_pnm_output", parameters.output.pnm_output_name);
	save_var(fd, "file_opencv_output", parameters.output.save_opencv_rendered_name);
	save_var(fd, "svg_underlay_image", parameters.output.svg_underlay_image);
	save_var(fd, "svg_force_opacity", parameters.output.svg_force_opacity);
	save_var(fd, "svg_force_width", parameters.output.svg_force_width);
	save_var(fd, "false_colors", parameters.output.false_colors);
	//save_var(fd, "", parameters.output.);
	fprintf(fd, "# Interactive mode: 0: disable, 1: show windows, 2: show trackbars\n");
	save_var(fd, "interactive", parameters.interactive);
	fprintf(fd, "# Scale images before viewing in window: 0: No scaling, 100: Small pictures\n");
	save_var(fd, "zoom_level", parameters.zoom_level);
	fprintf(fd, "# Save parameters to file on exit: 0: overwrite, 1: append\n");
	save_var(fd, "parameters_append", parameters.save_parameters_append);
	save_var(fd, "file_parameters", parameters.save_parameters_name);
	fprintf(fd, "###########################################################\n");
	return 0;
}

int load_params(const std::string filename, params &parameters) { // Load parameters from file given by name //TODO do headeru
	FILE *fd;
	if ((fd = fopen(filename.c_str(), "r")) == NULL) {
		fprintf(stderr, "Couldn't open config file for reading.\n");
		return 0;
	}
	int loaded = load_params(fd, parameters);
	fclose(fd);
	return loaded;
}

int save_params(const std::string filename, params &parameters) { // Save parameters to file given by name
	FILE *fd;
	if ((fd = fopen(filename.c_str(), "w")) == NULL) {
		fprintf(stderr, "Couldn't open config file for writing.\n");
		return 1;
	}
	int saved = save_params(fd, parameters);
	fclose(fd);
	return saved;
}

}; // namespace
