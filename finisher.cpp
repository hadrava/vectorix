#include "v_image.h"
#include "geom.h"
#include "finisher.h"
#include <cstdio>

namespace vectorix {

void finisher::apply_settings(v_image &vector) {
#ifndef NDEBUG
	for (v_line &line: vector.line) {
		for (v_point &segment: line.segment) {
			if (!(
			(segment.main.x == segment.main.x) &&
			(segment.main.y == segment.main.y) &&
			(segment.control_next.x == segment.control_next.x) &&
			(segment.control_next.y == segment.control_next.y) &&
			(segment.control_prev.x == segment.control_prev.x) &&
			(segment.control_prev.y == segment.control_prev.y)
				)) {
				fprintf(stderr, "Bug: Tracer output contains NaN!\n");
				throw;
			}
		}
	}
#endif

	if (*param_false_colors)
		vector.false_colors(*param_false_colors);

	geom::convert_to_variable_width(vector, *param_export_type, *par); // Convert image before writing

	// TODO show debug lines
	vector.show_debug_lines();

	for (v_line &line: vector.line) {
		for (v_point &segment: line.segment) {
			if (*param_force_width) {
				segment.width = *param_force_width; // set width of every line to the same value
			}
			if (*param_force_opacity)
				segment.opacity = *param_force_opacity; // set opacity of every line to the same value
		}
	}

	// TODO grouped export:
	//for (v_line &line: vector.line) {
	//std::list<v_line> to_render;
	//geom::group_line(to_render, line); // Convert every line to list of grouped one-segment lines
	// insert back


	if (!param_underlay_path->empty())
		vector.underlay_path = *param_underlay_path;
}

}; // namespace
