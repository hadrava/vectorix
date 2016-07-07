#ifndef VECTORIX__VECTOR_LINES_H
#define VECTORIX__VECTOR_LINES_H

// Basic vector data structures and transformations

#include <list>
#include <cmath>
#include "config.h"
#include "parameters.h"

namespace vectorix {

enum v_smooth_type {
	corner = 0,
	smooth = 1,
	symetric = 2
};

class v_pt { // Point/vector in 2D
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
	bool operator!=(const v_pt &other) {
		return !((x == other.x) && (y == other.y));
	};
	p len() const { // get length of vector = distance from pt (0, 0)
		return std::sqrt(x*x + y*y);
	}
	p angle() const { // calculate direction of vector
		float ret = std::atan(y/x);
		if (x<0)
			ret += M_PI;
		if (ret<0)
			ret += 2*M_PI;
		return ret;
	}
};

class v_co { // Color (3D vector)
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
public:
	void saturated_hue(p hue) { // Set color based on given hue
		auto from_hue = [](p hue) {
			if (hue < 60)
				return hue/60*255;
			else if (hue < 180)
				return (p) (255);
			else if (hue < 240)
				return (p) ((240 - hue)/60*255);
			else
				return (p) (0);
		};

		hue = fmodf(hue, 360);
		val[1] = from_hue(hue);
		hue += 120;
		hue = fmodf(hue, 360);
		val[0] = from_hue(hue);
		hue += 120;
		hue = fmodf(hue, 360);
		val[2] = from_hue(hue);
	}
	int val[3];
};

class v_point { // Control point of Bezier curve
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
	v_point(): opacity(1) {};
	v_pt main; // One main point
	v_pt control_prev; // Two control points
	v_pt control_next;
	p opacity; // Opacity of line in given point
	p width; // Line width in given point
	v_co color; // Line color in given point
	v_smooth_type get_smooth();
};

enum v_line_type { // Line with given width / outline of area
	stroke,
	fill
};

enum v_line_group { // How are lines grouped
	// with v_line_type stroke similar to svg group
	group_normal, // Just one line per object (no group)
	group_first, // First line in an object - it has positive sign (black color)
	group_continue, // Other lines in the same object - negative sign (white color - transparent)
	group_last // Last line - negative sign (white color - transparent)
};

class v_line { // One line or area
public:
	v_line(p x0, p y0, p x1, p y1, p x2, p y2, p x3, p y3);
	v_line(): type_(stroke) {};
	void add_point(v_pt _p_control_next, v_pt _control_prev, v_pt _main);
	void add_point(v_pt _p_control_next, v_pt _control_prev, v_pt _main, v_co _color, p _width = 1);
	void add_point(v_pt _main);
	void add_point(v_pt _main, v_co _color, p _width = 1);
	void set_type(v_line_type type) { type_ = type; }; // Change line type (stroke/fill)
	v_line_type get_type() const { return type_; };
	void reverse(); // Reverse line
	bool empty() const { return segment.empty(); };
	void set_group(v_line_group group) { group_ = group; };
	void set_color(v_co color);
	v_line_group get_group() const { return group_; };
	std::list<v_point> segment; // Line data
private:
	v_line_type type_; // Line type
	v_line_group group_ = group_normal;

};

class v_image { // Vector image
public:
	v_image(p _w, p _h): width(_w), height(_h) {};
	v_image(): width(0), height(0) {};
	void clean(); // Drop all lines
	void add_line(v_line _line); // Add given line to image
	// type == 0: do nothing
	// type == 1: chop segments to separate lines and group them
	// type == 2: convert to outline
	//            precision of conversion is given by parameter par.max_contour_error
	// type == 3: calculate variance of width and guess, whether line should be converted to outline
	//            max allowed variance is given by parameter par.auto_contour_variance
	void false_colors(p hue_step); // Color each line with different color
	p width; // Image dimensions
	p height;
	std::list<v_line> line; // Image data
	std::string underlay_path; // Underlay image for exporting

	static void add_debug_line(v_pt a, v_pt b);
	void show_debug_lines();
	static std::list<v_line> debug_line;
};

}; // namespace
#endif
