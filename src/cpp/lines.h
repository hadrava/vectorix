#ifndef _VECTOR_LINES_H
#define _VECTOR_LINES_H

#include <list>
#include "config.h"

class v_pt {
public:
	v_pt(p _x = 0, p _y = 0): x(_x), y(_y) {};
	p x;
	p y;
};

class v_co {
public:
	v_co(int _r, int _g, int _b) { val[0] = _r; val[1] = _g; val[2] = _b; };
	v_co() { val[0] = 0; val[1] = 0; val[2] = 0; };
	class v_co &operator+=(const class v_co &other) {
		val[0] += other.val[0];
		val[1] += other.val[1];
		val[2] += other.val[2];
	};
	class v_co &operator/=(int other) {
		val[0] /= other;
		val[1] /= other;
		val[2] /= other;
	};
	int val[3];
};

class v_point {
public:
	v_point(v_pt _control_prev, v_pt _main, v_pt _control_next):
		main(_main), control_prev(_control_prev), control_next(_control_next),
		opacity(1), width(1) {};
	v_point(v_pt _control_prev, v_pt _main, v_pt _control_next, v_co _color):
		main(_main), control_prev(_control_prev), control_next(_control_next),
		opacity(1), width(1), color(_color) {};
	v_pt main;
	v_pt control_prev;
	v_pt control_next;
	p opacity;
	p width;
	v_co color;
};

class v_line {
public:
	v_line(p x0, p y0, p x1, p y1, p x2, p y2, p x3, p y3);
	v_line() {};
	void add_point(v_pt &&_p_control_next, v_pt &&_control_prev, v_pt &&_main);
	void add_point(v_pt &&_p_control_next, v_pt &&_control_prev, v_pt &&_main, v_co &&_color);
	void add_point(v_pt &&_main);
	void add_point(v_pt &&_main, v_co &&_color);
	std::list<v_point> segment;
};

class v_image {
public:
	v_image(p _w, p _h): width(_w), height(_h) {};
	void clean();
	void add_line(v_line _line);
	p width;
	p height;
	std::list<v_line> line;
};

#endif
