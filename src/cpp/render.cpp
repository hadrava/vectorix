#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include "v_image.h"
#include "pnm_handler.h"
#include "render.h"

using namespace vect;
using namespace pnm;

void render_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

void bezier_render(pnm_image &bitmap, const v_line &line) {
	if (bitmap.type != PNM_BINARY_PGM)
		render_error("Error: Image type %i not supported.\n", bitmap.type);
	auto two = line.segment.cbegin();
	auto one = two;
	if (two != line.segment.cend())
		two++;
	while (two != line.segment.cend()) {
		for (p u=0; u<=1; u+=0.0005) {
			p v = 1-u;
			p x = v*v*v*one->main.x + 3*v*v*u*one->control_next.x + 3*v*u*u*two->control_prev.x + u*u*u*two->main.x;
			p y = v*v*v*one->main.y + 3*v*v*u*one->control_next.y + 3*v*u*u*two->control_prev.y + u*u*u*two->main.y;
			p w = v*one->width + u*two->width;
			for (int j = y - w/2; j<= y + w/2; j++) {
				if (j<0 || j>=bitmap.height)
					continue;
				for (int i = x - w/2; i<= x + w/2; i++) {
					if (i<0 || i>=bitmap.width)
						continue;
					if ((x-i)*(x-i) + (y-j)*(y-j) <= w*w/4)
						bitmap.data[i+j*bitmap.width] = 0;
				}
			}
		}
		one=two;
		two++;
	}
}

pnm_image render(const v_image &vector) {
	auto bitmap = pnm_image(vector.width, vector.height);
	bitmap.erase_image();
	for (v_line line: vector.line) {
		bezier_render(bitmap, line);
	}
	return bitmap;
}
