#include "lines.h"

void v_image::clean() {
	for (v_line _line: line) {
		_line.segment.erase(_line.segment.begin(), _line.segment.end());
	}
	line.erase(line.begin(), line.end());
}

v_line::v_line(p x0, p y0, p x1, p y1, p x2, p y2, p x3, p y3) {
	segment.emplace_back(v_point(v_pt(x0,y0), v_pt(x0,y0), v_pt(x1,y1)));
	segment.emplace_back(v_point(v_pt(x2,y2), v_pt(x3,y3), v_pt(x3,y3)));
}

void v_line::add_point(v_pt &&_p_control_next, v_pt &&_control_prev, v_pt &&_main) {
	segment.back().control_next = _p_control_next;
	segment.emplace_back(v_point(_control_prev, _main, _main));
}

void v_line::add_point(v_pt &&_p_control_next, v_pt &&_control_prev, v_pt &&_main, v_co &&_color) {
	segment.back().control_next = _p_control_next;
	segment.emplace_back(v_point(_control_prev, _main, _main, _color));
}

void v_line::add_point(v_pt &&_main) {
	segment.emplace_back(v_point(_main, _main, _main));
}

void v_line::add_point(v_pt &&_main, v_co &&_color) {
	segment.emplace_back(v_point(_main, _main, _main, _color));
}

void v_image::add_line(v_line _line) {
	line.push_back(_line);
}
