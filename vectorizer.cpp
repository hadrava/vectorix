#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include "offset.h"
#include "v_image.h"
#include "pnm_handler.h"
#include "vectorizer.h"
#include "parameters.h"

// Simple vectorizer and output functions

namespace vectorix {

v_image vectorizer_example::vectorize(const pnm_image &image, params &parameters) { // This is *not* a vectorizer. It just produce fixed vector image with same size as input
	auto out = v_image(image.width, image.height);
	auto line = v_line(0, 0, image.width/2, image.height/2, image.width, image.height/2, image.width, image.height);
	line.add_point(v_pt(image.width, image.height/2*3), v_pt(-image.width/2, -image.height/2), v_pt(0,0), v_co(255, 0, 0), 20);
	out.add_line(line);
	offset convertor(out);
	convertor.convert_to_outline(line, parameters.output.max_contour_error); // Convert from stroke to fill
	out.add_line(line); // Add same line in the other type as well

	line = v_line();
	line.add_point(v_pt(40, 40), v_co(255, 0, 0), 8);
	line.add_point(v_pt(50, 60), v_co(255, 0, 0), 8);
	line.add_point(v_pt(60, 40), v_co(255, 0, 0), 8);
	line.add_point(v_pt(70, 60), v_co(255, 0, 0), 8);
	line.add_point(v_pt(80, 40), v_co(255, 0, 0), 2);
	line.add_point(v_pt(110, 70), v_co(255, 0, 0), 30); // This line has variable width
	out.add_line(line);
	convertor.convert_to_outline(line, parameters.output.max_contour_error); // Convert from stroke to fill
	out.add_line(line); // Add same line in the other type as well

	line = v_line();
	line.add_point(v_pt(0, 100), v_co(255, 0, 0), 8);
	line.add_point(v_pt(150, 100), v_pt(200, 100), v_pt(300, 100), v_co(255, 0, 0), 300);
	convertor.convert_to_outline(line, parameters.output.max_contour_error); // Convert from stroke to fill
	out.add_line(line);

	line = v_line();
	line.add_point(v_pt(0, 400), v_co(255, 0, 0), 202);
	line.add_point(v_pt(1, 400), v_pt(2.5, 400), v_pt(3, 400), v_co(255, 0, 0), 200);
	convertor.convert_to_outline(line, parameters.output.max_contour_error); // Convert from stroke to fill
	out.add_line(line);

	line = v_line();
	line.add_point(v_pt(0, 900), v_co(255, 0, 0), 0.5);
	line.add_point(v_pt(1, 901), v_pt(2.5, 902), v_pt(3, 903), v_co(255, 0, 0), 0.5);
	out.add_line(line);

	return out;
}

}; // namespace
