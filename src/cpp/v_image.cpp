#include "v_image.h"
#include "parameters.h"
#include <list>
#include <cmath>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>

namespace vect {

void v_image::clean() { // Remove all lines
	for (v_line _line: line) {
		_line.segment.erase(_line.segment.begin(), _line.segment.end());
	}
	line.erase(line.begin(), line.end());
}

v_line::v_line(p x0, p y0, p x1, p y1, p x2, p y2, p x3, p y3) { // New line with control points: main(0), control(1) -- control(2), main(3)
	segment.emplace_back(v_point(v_pt(x0,y0), v_pt(x0,y0), v_pt(x1,y1)));
	segment.emplace_back(v_point(v_pt(x2,y2), v_pt(x3,y3), v_pt(x3,y3)));
	type_ = stroke;
}

void v_line::add_point(v_pt &&_p_control_next, v_pt &&_control_prev, v_pt &&_main) { // Add segment to end of a line (control_prev is added to previous point)
	segment.back().control_next = _p_control_next;
	segment.emplace_back(v_point(_control_prev, _main, _main));
}

void v_line::add_point(v_pt &&_p_control_next, v_pt &&_control_prev, v_pt &&_main, v_co _color, p _width) { // Add segment with specified color and width
	segment.back().control_next = _p_control_next;
	segment.emplace_back(v_point(_control_prev, _main, _main, _color, _width));
}

void v_line::add_point(v_pt &&_main) { // Add point (straight continuation)
	segment.emplace_back(v_point(_main, _main, _main));
}

void v_line::add_point(v_pt &&_main, v_co _color, p _width) { // Add point with color and width
	segment.emplace_back(v_point(_main, _main, _main, _color, _width));
}

void v_line::reverse() { // Reverse line
	segment.reverse();
	for (auto control = segment.begin(); control != segment.end(); control++) {
		std::swap(control->control_next, control->control_prev); // Swap control points
	}
};

void v_image::add_line(v_line _line) { // Add bezier curve to image
	line.push_back(_line);
}

void v_line::set_color(v_co color) { // Set color to each point of line
	for (auto pt = segment.begin(); pt != segment.end(); pt++) {
		pt->color = color;
	}
}

void v_image::false_colors(p hue_step) { // Colorize each line with saturated color
	p hue = 0;
	for (auto l = line.begin(); l != line.end(); l++) {
		v_co col = v_co::from_color(hue); // Calculate color from hue
		l->set_color(col); // Set color for current line
		hue += hue_step; // Increase hue angle
		hue = fmodf (hue, 360);
	}
}

}; // namespace
