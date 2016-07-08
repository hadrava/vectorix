#include <cstring>
#include <cerrno>
#include <cstdio>
#include <new>
#include "potrace/include/potracelib.h"
#include "v_image.h"
#include "pnm_handler.h"
#include "vectorizer_potrace.h"
#include "logger.h"

// Vectorize using potracelib

namespace vectorix {

v_image vectorizer_potrace::vectorize(const pnm_image &original, params &parameters) { // Vectorize using potrace library
	pnm_image image = original;
	image.convert(pnm_variant_type::binary_pbm); // Potrace uses binary images, packed in word -- almost similar to binary PBM

	potrace_bitmap_t pot_bitmap; // Prepare Potrace data structure with image
	pot_bitmap.w = image.width;
	pot_bitmap.h = image.height;
	pot_bitmap.dy = ((image.width - 1) / (8*sizeof(potrace_word))) + 1; // Potrace uses this for accessing lines
	pot_bitmap.map = new potrace_word[pot_bitmap.h * pot_bitmap.dy]; // Buffer for real image data data

	for (int i = 0; i < image.height; i++) {
		for (int j = 0; j < pot_bitmap.dy; j++) {
			pot_bitmap.map[i*pot_bitmap.dy + j] = 0;
			for (int x = 0; (x < sizeof(potrace_word)) && (x+j*sizeof(potrace_word) < ((image.width-1)/8+1)); x++) { // Fill word with bytes form binary PBM.
				pot_bitmap.map[i*pot_bitmap.dy + j] |= (static_cast<potrace_word> (image.data[i*((image.width-1)/8+1) + j*sizeof(potrace_word) + x])) << (8*(sizeof(potrace_word)-x-1));
			}
		}
	}

	potrace_param_t *pot_params = potrace_param_default(); // Use default Potrace parameters
	if (!pot_params) {
		log.log<log_level::error>("Error: " __FILE__ ":%i: In %s(): %s\n", __LINE__, __func__, strerror(errno));
		throw std::bad_alloc();
	}

	potrace_state_t *pot_state = potrace_trace(pot_params, &pot_bitmap); // Run tracing
	if (!pot_state || pot_state->status != POTRACE_STATUS_OK) {
		log.log<log_level::error>("Error: " __FILE__ ":%i: In %s(): %s\n", __LINE__, __func__, strerror(errno));
		throw std::bad_alloc();
	}

	delete [] pot_bitmap.map; // Binary image is no longer needed
	v_image vector(image.width, image.height); // Prepare empty v_image for output

	potrace_path_t *pot_path = pot_state->plist;
	while (pot_path != NULL) { // Convert each path from Potrace to our v_line format
		v_line line;
		v_co color(0, 0, 0); // Foreground color should be black since Potrace knows only BW images
		if (pot_path->sign == '-') {
			color = v_co(255, 255, 255); // Inner curve (subtract) --> white color
			if (vector.line.back().get_group() == v_line_group::group_normal) // last segment has + sign
				vector.line.back().set_group(v_line_group::group_first); // last segment should be first in a group
			if (vector.line.back().get_group() == v_line_group::group_last) // last segment has - sign
				vector.line.back().set_group(v_line_group::group_continue); // last segment should be continuation of a group
			line.set_group(v_line_group::group_last);
		}
		line.add_point(v_pt(pot_path->curve.c[pot_path->curve.n - 1][2].x, pot_path->curve.c[pot_path->curve.n - 1][2].y), color); // Add last point --> closed path
		for (int n = 0; n < pot_path->curve.n; n++) {
			if (pot_path->curve.tag[n] == POTRACE_CORNER) { // Two straight segments
				line.add_point(v_pt(pot_path->curve.c[n][1].x, pot_path->curve.c[n][1].y), color);
				line.add_point(v_pt(pot_path->curve.c[n][2].x, pot_path->curve.c[n][2].y), color);
			}
			else if (pot_path->curve.tag[n] == POTRACE_CURVETO) { // Bezier curve with two control points
				line.add_point( \
						v_pt(pot_path->curve.c[n][0].x, pot_path->curve.c[n][0].y), \
						v_pt(pot_path->curve.c[n][1].x, pot_path->curve.c[n][1].y), \
						v_pt(pot_path->curve.c[n][2].x, pot_path->curve.c[n][2].y), color);
			}
		}
		line.set_type(v_line_type::fill); // Change type to fill (from default stroke)
		vector.add_line(line); // Add line to output
		pot_path = pot_path->next; // Process next potrace path
	}

	potrace_state_free(pot_state);
	potrace_param_free(pot_params);

	return vector;
}

}; // namespace
