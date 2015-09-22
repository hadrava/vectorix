#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include "v_image.h"
#include "pnm_handler.h"
#include "vectorizer.h"
#include "parameters.h"

// Simple vectorizer and output functions

using namespace pnm;

namespace vect {

void generic_vectorizer::vectorizer_error(const char *format, ...) { // Prints vectorizer error message
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

void generic_vectorizer::vectorizer_info(const char *format, ...) { // Prints vectorizer info message
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

#ifdef VECTORIZER_DEBUG
void generic_vectorizer::vectorizer_debug(const char *format, ...) { // Prints vectorizer debug message (iff VECTORIZER_DEBUG is defined)
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}
#else
void generic_vectorizer::vectorizer_debug(const char *format, ...) {}; // Does nothing
#endif

v_image stupid::vectorize(const pnm_image &image, const params &parameters) { // This is *not* a vectorizer. It just produce fixed vector image with same size as input
	auto out = v_image(image.width, image.height);
	auto line = v_line(0, 0, image.width/2, image.height/2, image.width, image.height/2, image.width, image.height);
	line.add_point(v_pt(image.width, image.height/2*3), v_pt(-image.width/2, -image.height/2), v_pt(0,0), v_co(255, 0, 0), 20);
	out.add_line(line);
	line.convert_to_outline(parameters.output.max_contour_error); // Convert from stroke to fill
	out.add_line(line); // Add same line in the other type as well

	line = v_line();
	line.add_point(v_pt(40, 40), v_co(255, 0, 0), 8);
	line.add_point(v_pt(50, 60), v_co(255, 0, 0), 8);
	line.add_point(v_pt(60, 40), v_co(255, 0, 0), 8);
	line.add_point(v_pt(70, 60), v_co(255, 0, 0), 8);
	line.add_point(v_pt(80, 40), v_co(255, 0, 0), 2);
	line.add_point(v_pt(110, 70), v_co(255, 0, 0), 30); // This line has variable width
	out.add_line(line);
	line.convert_to_outline(parameters.output.max_contour_error); // Convert from stroke to fill
	out.add_line(line); // Add same line in the other type as well
	return out;
}

}; // namespace
