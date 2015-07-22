#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include "v_image.h"
#include "pnm_handler.h"
#include "vectorizer.h"
#include "parameters.h"

using namespace pnm;

namespace vect {

void generic_vectorizer::vectorizer_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

#ifdef VECTORIZER_DEBUG
void generic_vectorizer::vectorizer_debug(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}
#else
void generic_vectorizer::vectorizer_debug(const char *format, ...) {};
#endif

v_image stupid::vectorize(const pnm_image &image) {
	auto out = v_image(image.width, image.height);
	auto line = v_line(0, 0, image.width/2, image.height/2, image.width, image.height/2, image.width, image.height);
	line.add_point(v_pt(image.width, image.height/2*3), v_pt(-image.width/2, -image.height/2), v_pt(0,0), v_co(255, 0, 0), 20);
	out.add_line(line);
	line.convert_to_outline(global_params.output.max_contour_error);
	out.add_line(line);

	line = v_line();
	line.add_point(v_pt(40, 40), v_co(255, 0, 0), 8);
	line.add_point(v_pt(50, 60), v_co(255, 0, 0), 8);
	line.add_point(v_pt(60, 40), v_co(255, 0, 0), 8);
	line.add_point(v_pt(70, 60), v_co(255, 0, 0), 8);
	line.add_point(v_pt(80, 40), v_co(255, 0, 0), 2);
	line.add_point(v_pt(110, 70), v_co(255, 0, 0), 30);
	out.add_line(line);
	line.convert_to_outline(global_params.output.max_contour_error);
	out.add_line(line);
	return out;
}

}; // namespace
