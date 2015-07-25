#ifndef _VECTOR_LINES_H
#define _VECTOR_LINES_H

#include <list>
#include <cmath>
#include "config.h"
#include "parameters.h"

namespace vect {

class v_pt {
public:
	v_pt(p _x = 0, p _y = 0): x(_x), y(_y) {};
	p x;
	p y;
	v_pt &operator+=(const v_pt &other) { // Simple operators (in header for inlining).
		x += other.x;
		y += other.y;
	};
	friend v_pt operator+(v_pt first, const v_pt &other) {
		first += other;
		return first;
	};
	v_pt &operator-=(const v_pt &other) { // Simple operators (in header for inlining).
		x -= other.x;
		y -= other.y;
	};
	friend v_pt operator-(v_pt first, const v_pt &other) {
		first -= other;
		return first;
	};
	v_pt &operator*=(p mul) { // Simple operators (in header for inlining).
		x *= mul;
		y *= mul;
	};
	friend v_pt operator*(v_pt first, p mul) {
		first *= mul;
		return first;
	};
	v_pt &operator/=(p div) { // Simple operators (in header for inlining).
		x /= div;
		y /= div;
	};
	friend v_pt operator/(v_pt first, p div) {
		first /= div;
		return first;
	};
	bool operator==(const v_pt &other) {
		return ((x == other.x) && (y == other.y));
	};
	p len() const {
		return std::sqrt(x*x + y*y);
	}
};

class v_co {
public:
	v_co(int _r, int _g, int _b) { val[0] = _r; val[1] = _g; val[2] = _b; };
	v_co() { val[0] = 0; val[1] = 0; val[2] = 0; };
	v_co &operator+=(const v_co &other) { // Simple operators (in header for inlining).
		val[0] += other.val[0];
		val[1] += other.val[1];
		val[2] += other.val[2];
	};
	friend v_co operator+(v_co first, v_co second) {
		first += second;
		return first;
	};
	v_co &operator/=(int other) {
		val[0] /= other;
		val[1] /= other;
		val[2] /= other;
	};
	v_co &operator*=(p mul) {
		val[0] *= mul;
		val[1] *= mul;
		val[2] *= mul;
	};
	friend v_co operator*(v_co first, p mul) {
		first *= mul;
		return first;
	};
	int val[3];
};

class v_point {
public:
	v_point(v_pt _control_prev, v_pt _main, v_pt _control_next):
		main(_main), control_prev(_control_prev), control_next(_control_next),
		opacity(1), width(1) {};
	v_point(v_pt _control_prev, v_pt _main, v_pt _control_next, v_co _color, p _width = 1):
		main(_main), control_prev(_control_prev), control_next(_control_next),
		opacity(1), width(_width), color(_color) {};
	v_point(v_pt _pt, v_co _color, p _width):
		main(_pt), control_prev(_pt), control_next(_pt),
		opacity(1), width(_width), color(_color) {};
	v_point() {};
	v_pt main;
	v_pt control_prev;
	v_pt control_next;
	p opacity;
	p width;
	v_co color;
};

enum v_line_type {
	stroke,
	fill
};

enum v_line_group {
	group_normal,
	group_first,
	group_continue,
	group_last
};

class v_line {
public:
	v_line(p x0, p y0, p x1, p y1, p x2, p y2, p x3, p y3);
	v_line(): type_(stroke) {};
	void add_point(v_pt &&_p_control_next, v_pt &&_control_prev, v_pt &&_main);
	void add_point(v_pt &&_p_control_next, v_pt &&_control_prev, v_pt &&_main, v_co _color, p _width = 1);
	void add_point(v_pt &&_main);
	void add_point(v_pt &&_main, v_co _color, p _width = 1);
	void set_type(v_line_type type) { type_ = type; };
	v_line_type get_type() const { return type_; };
	void reverse();
	bool empty() const { return segment.empty(); };
	void set_group(v_line_group group) { group_ = group; };
	void convert_to_outline(p max_error = 1);
	void auto_smooth();
	v_line_group get_group() const { return group_; };
	std::list<v_point> segment;
private:
	v_line_type type_;
	v_line_group group_ = group_normal;
};

class v_image {
public:
	v_image(p _w, p _h): width(_w), height(_h) {};
	v_image(): width(0), height(0) {};
	void clean();
	void add_line(v_line _line);
	void convert_to_variable_width(int type, output_params &par);
	p width;
	p height;
	std::list<v_line> line;
};

p distance(const v_pt &a, const v_pt &b);
p distance(const v_point &a, const v_point &b);
inline p v_pt_distance(const v_pt &a, const v_pt &b) {
	distance(a, b);
}
void chop_in_half(v_point &one, v_point &two, v_point &newpoint);
void chop_line(v_line &line, p max_distance = 1);
void group_line(std::list<v_line> &list, const v_line &line);



}; // namespace
#endif
