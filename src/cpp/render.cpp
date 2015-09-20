#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include "v_image.h"
#include "pnm_handler.h"
#include "render.h"

// Simple renderer from v_image to pnm bitmap
// Does not need OpenCV
// Warning: Ignores fill type -- renders everything as lines

using namespace pnm;

namespace vect {

void renderer::render_error(const char *format, ...) { // Errors are writen to stderr
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

void renderer::bezier_render(pnm_image &bitmap, const v_line &line) { // Render one path made of bezier curves (or straight line)
	if (bitmap.type != PNM_BINARY_PGM) //TODO{
		render_error("Error: Image type %i not supported.\n", bitmap.type);
		//TODO return;
	//TODO}
	auto two = line.segment.cbegin();
	auto one = two;
	if (two != line.segment.cend())
		two++;
	while (two != line.segment.cend()) { // Cycle throught every segment
		for (p u = 0; u <= 1; u += 0.0005) { // Rendering is done with fixed step
			// This is definitely not a good idea. It is
			// unnecessarily slow on short paths and may leave
			// white spots on large paths. Fortunately there is
			// completely different approach implemented in
			// opencv_render.
			p v = 1-u;
			p x = v*v*v*one->main.x + 3*v*v*u*one->control_next.x + 3*v*u*u*two->control_prev.x + u*u*u*two->main.x; // Bezier coefficients
			p y = v*v*v*one->main.y + 3*v*v*u*one->control_next.y + 3*v*u*u*two->control_prev.y + u*u*u*two->main.y;
			p w = v*one->width + u*two->width;
			for (int j = y - w/2; j<= y + w/2; j++) { // Line has some width -- thick lines are realy slow to render
				if (j<0 || j>=bitmap.height) // Do not draw outside of an image
					continue;
				for (int i = x - w/2; i<= x + w/2; i++) {
					if (i<0 || i>=bitmap.width)
						continue;
					if ((x-i)*(x-i) + (y-j)*(y-j) <= w*w/4) // Check if we are in distance <= radius
						bitmap.data[i+j*bitmap.width] = 0;
				}
			}
		}
		one = two; // Move to next segment
		two++;
	}
}

pnm_image renderer::render(const v_image &vector) {
	auto bitmap = pnm_image(vector.width, vector.height);
	bitmap.erase_image();
	for (v_line line: vector.line) { // Render each line in image
		bezier_render(bitmap, line);
	}
	return bitmap;
}

}; // namespace
