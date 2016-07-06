#include "v_image.h"
#include "geom.h"
#include "finisher.h"
#include <cstdio>

namespace vectorix {

void finisher::apply_settings(v_image &vector, const params &parameters) {
	geom::convert_to_variable_width(vector, parameters.output.export_type, parameters.output); // Convert image before writing

	// TODO show debug lines
	vector.show_debug_lines();

	for (v_line &line: vector.line) {
		for (v_point &segment: line.segment) {
			if (parameters.output.svg_force_width) {
				segment.width = parameters.output.svg_force_width; // set width of every line to the same value
			}
			if (parameters.output.svg_force_opacity)
				segment.opacity = parameters.output.svg_force_opacity; // set opacity of every line to the same value
		}
	}
}

}; // namespace
