#include "v_image.h"
#include "parameters.h"
#include <list>
#include <cmath>
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

p distance(const v_pt &a, const v_pt &b) { // Calculate distance between two points
	p x = (a.x - b.x);
	p y = (a.y - b.y);
	return std::sqrt(x*x + y*y);
}

p maximal_bezier_length(const v_point &a, const v_point &b) { // Calculate maximal length of given segment
	return distance(a.main, a.control_next) + distance(a.control_next, b.control_prev) + distance(b.control_prev, b.main);
}

void chop_in_half(v_point &one, v_point &two, v_point &newpoint) { // Add newpoint in the middle of bezier segment
	newpoint.control_prev.x = ((one.control_next.x + one.main.x)/2 + (one.control_next.x + two.control_prev.x)/2)/2;
	newpoint.control_prev.y = ((one.control_next.y + one.main.y)/2 + (one.control_next.y + two.control_prev.y)/2)/2;

	newpoint.control_next.x = ((two.control_prev.x + two.main.x)/2 + (one.control_next.x + two.control_prev.x)/2)/2;
	newpoint.control_next.y = ((two.control_prev.y + two.main.y)/2 + (one.control_next.y + two.control_prev.y)/2)/2;

	newpoint.main.x = (newpoint.control_prev.x  + newpoint.control_next.x)/2;
	newpoint.main.y = (newpoint.control_prev.y  + newpoint.control_next.y)/2;

	newpoint.opacity = (one.opacity + two.opacity)/2;
	newpoint.width = (one.width + two.width)/2;
	newpoint.color = one.color;
	newpoint.color += two.color;
	newpoint.color /= 2;
	one.control_next.x = (one.control_next.x + one.main.x)/2;
	one.control_next.y = (one.control_next.y + one.main.y)/2;
	two.control_prev.x = (two.control_prev.x + two.main.x)/2;
	two.control_prev.y = (two.control_prev.y + two.main.y)/2;
}

void chop_line(v_line &line, p max_distance) { // Chop whole line, so the maximal length of segment is max_distance
	auto two = line.segment.begin();
	auto one = two;
	if (two != line.segment.end())
		++two;
	while (two != line.segment.end()) {
		while (maximal_bezier_length(*one, *two) > max_distance) {
			v_point newpoint;
			chop_in_half(*one, *two, newpoint);
			line.segment.insert(two, newpoint);
			--two;
		}
		one=two;
		two++;
	}
}

void group_line(std::list<v_line> &list, const v_line &line) { // Convert one line to list of line. Each created line consists of one segment. Created lines are marked as group
	auto two = line.segment.begin();
	auto one = two;
	if ((two != line.segment.end()) && (line.get_type() == stroke))
		++two;
	else {
		list.push_back(line);
		return; // fill or empty lines cannot be converted
	}
	int segment_count = 0;
	while (two != line.segment.end()) {
		v_line new_line;
		new_line.segment.push_back(*one);
		new_line.segment.push_back(*two);
		new_line.set_type(stroke);
		new_line.set_group(group_continue);
		list.push_back(new_line); // Add segment to list

		one=two;
		two++;
		segment_count++;
	}
	if (segment_count >= 2) {
		list.front().set_group(group_first);
		list.back().set_group(group_last);
	}
	else
		list.front().set_group(group_normal);
}

void v_line::rot(v_pt &pt, int sign) { // rotate vector by +- 90 degrees (sign == 1: rotate right; sign == -1: rotate left)
	p x = pt.x;
	p y = pt.y;
	pt.x = y * sign;
	pt.y = -x * sign;
}

void v_line::shift(const std::list<v_point> &context, std::list<v_point>::iterator pts, std::list<v_point> &output, int sign) { // Calculate position of new points when shifting point to boundary. Input: context + iterator to current point on line, sign (shift up +1 / down -1); Output: list of new point(s) (one or two)
	v_point pt = *pts;
	if (distance(pt.control_prev, pt.main) < epsilon) { // Previous control point not set
		if (pts != context.begin()) {
			auto tmp = pts;
			--tmp;
			pt.control_prev = tmp->main; // Replace with direction to previous main point
		}
	}
	if (distance(pt.control_next, pt.main) < epsilon) { // Next control point not set
		auto tmp = pts;
		++tmp;
		if (tmp != context.end()) {
			pt.control_next = tmp->main; // Replace with direction to previous main point
		}
	}
	if (distance(pt.control_prev, pt.main) < epsilon) { // Previous direction is still undefined
		pt.control_prev.x = 2*pt.main.x - pt.control_next.x; // Assume, that point is autosmooth: reverse direction to next control point
		pt.control_prev.y = 2*pt.main.y - pt.control_next.y;
	}
	if (distance(pt.control_next, pt.main) < epsilon) { // Next direction is still undefined
		pt.control_next.x = 2*pt.main.x - pt.control_prev.x; // Assume, that point is autosmooth: reverse direction to previous control point
		pt.control_next.y = 2*pt.main.y - pt.control_prev.y;
	}
	if (distance(pt.control_prev, pt.main) < epsilon) { // There is no control point at all (probably only one point on line -> should be replaced with circle)
		pt.control_prev = pt.main;
		--pt.control_prev.x; // Previous control point will be on left
		pt.control_next = pt.main;
		++pt.control_next.x; // Next control point will be on right
	}
	p dprev = distance(pt.control_prev, pt.main);
	p dnext = distance(pt.control_next, pt.main);
	v_pt prev, next; // Vectors from main point to previous / next point on line
	prev = (pt.control_prev - pt.main) / dprev; // Make it unit
	next = (pt.control_next - pt.main) / dnext;

	v_pt prot = prev;
	v_pt nrot = next;
	rot(prot, -sign); // perpendicular to line before current main point (*pts)
	rot(nrot, sign); // perpendicular to line after current main point (*pts)
	// For straight line, theese vectors have the save direction
	// If sign is positive, theese line are directing down (prev is to left, next is to right)

	v_pt shift;
	shift = prot + nrot;

	if ((prot.x * next.x + prot.y * next.y) >= -epsilon) {
		// angle is straight, obtuse , right or acute (<= 180 deg)
		// Will create just one point...
		p d = shift.x * prot.x + shift.y * prot.y;
		shift.x *= pts->width / 2 / d;
		shift.y *= pts->width / 2 / d;

		v_point out = *pts;
		out.main.x += shift.x; // move main to shifted position
		out.main.y += shift.y;

		if (pts != context.begin()) { // We are not the first point
			out.control_prev += shift; // Move control point as well
		}
		else {
			// Begining of line
			out.control_prev.x = out.main.x + prev.x*out.width*2/3; // Set control point to make round end of line (4/3 of half-width)
			out.control_prev.y = out.main.y + prev.y*out.width*2/3;
		}
		auto tmp = pts;
		++tmp;
		if (tmp != context.end())
			out.control_next += shift; // Shift also next control point
		else
			out.control_next = out.main + next*out.width*2/3; // Make round end of line
		out.width = 1;

		output.push_back(out);
	}
	else {
		// Reflex angle (> 180 deg)
		// Convert to two points
		shift /= shift.len(); // Make shift vector unit
		// Next two are for correct calculating of control points before two new points
		p want = 1 - (prot.x*shift.x + prot.y*shift.y); // Wanted sagitta (height of circular segment) (for width 1px)
		p curr = -prev.x*shift.x - prev.y*shift.y; // Height gained by placint control point in 1px distance from main point

		v_point out1 = *pts;
		out1.main += prot * out1.width/2; // Just move main point perpendicular to direction
		out1.control_prev += prot * out1.width/2; // Just move previous control point perpendicular to direction
		out1.control_next = out1.main - prev*out1.width*2/3*want/curr; // Scale direction to prev correctly to create round continuation
		out1.width = 1;

		v_point out2 = *pts;
		out2.main += nrot * out2.width/2; // Just move main point perpendicular to direction
		out2.control_prev = out2.main - next*out2.width*2/3*want/curr;// Scale direction to prev correctly to create round continuation
		out2.control_next += nrot * out2.width/2; // Just move next control point perpendicular to direction
		out2.width = 1;

		output.push_back(out1);
		output.push_back(out2);
	}
}

p v_line::calculate_error(const v_point &uc, const v_point &cc, const v_point &lc) { // Measure error between upper bound (uc), lower bound (lc) and line defined by point cc and width
	fprintf(stderr, "calculate_error: lc: %f, %f; cc: %f, %f; uc: %f, %f\n", lc.main.x, lc.main.y, cc.main.x, cc.main.y, uc.main.x, uc.main.y);
	//u.x = lc.control_next.x - uc.control_prev.x
	//u.y = lc.control_next.y - uc.control_prev.y
	v_pt c = cc.control_next - cc.control_prev; // Tangent to line
	c /= c.len(); // Make unit length
	rot(c, 1); // Rotate right 90 deg (perpendicular to line)
	//l.x = lc.control_next.x - lc.control_prev.x
	//l.y = lc.control_next.y - lc.control_prev.y
	//up = c
	//low = -c
	v_pt u = uc.main - cc.main; // Up vector
	v_pt l = lc.main - cc.main; // Down vector
	p error_u = c.x*u.x + c.y*u.y - cc.width/2; // Calculate distance of point uc to centerline
	p error_l = c.x*l.x + c.y*l.y + cc.width/2;
	error_u *= error_u; // Square error
	error_l *= error_l;
	fprintf(stderr, "calculate_error: u: %f, %f; cc: %f, %f; l: %f, %f\n", u.x, u.y, cc.main.x, cc.main.y, l.x, l.y);
	fprintf(stderr, "calculate_error: distance %f, %f\n", error_u, error_l);
	return error_u + error_l;
}

void v_line::convert_to_outline(p max_error) { // Calculate outline of each line
	if (get_type() == fill) // It is already outline
		return;
	v_line upper;
	v_line lower;
	auto two = segment.begin(); // Right point of current segment
	auto one = two; // Left point of current segment
	if (two != segment.end())
		++two; // Change to second point
	else {
		set_type(fill); // Line is empty, just change its type
		return;
	}
	// Naming assumes that the line stored in segment is continuing to the right
	std::list<v_point> up; // Upper boundary of line
	shift(segment, one, up, 1); // Shift start of line up
	upper.segment.splice(upper.segment.end(), up);
	std::list<v_point> down; // Lower boundary of line
	shift(segment, one, down, -1); // Shift start of line down
	lower.segment.splice(lower.segment.end(), down);

	while (two != segment.end()) { // Until we run out of segments
		p error;
		do {
			auto u = upper.segment.end();
			auto l = lower.segment.end();
			v_point u1 = *(--u); // Last new (upper) point -- left end of current segment
			v_point l1 = *(--l); // Last new (lower) point -- left end of current segment

			std::list<v_point> up;
			shift(segment, two, up, 1); // Shift second end of current segment up
			upper.segment.splice(upper.segment.end(), up);
			std::list<v_point> down;
			shift(segment, two, down, -1); // Shift second end of current segment up
			lower.segment.splice(lower.segment.end(), down);

			v_point u2 = *(++u); // Right end of current segment (upper boundary)
			v_point l2 = *(++l); // Right end of current segment (lower boundary)

			v_point c1 = *one; // Left end of center line
			v_point c2 = *two; // Right end of center line
			v_point uc;
			v_point cc;
			v_point lc;
			chop_in_half(u1, u2, uc); // Chop upper segment in half
			chop_in_half(c1, c2, cc); // Chop center segment in half
			chop_in_half(l1, l2, lc); // Chop lower segment in half

			error = calculate_error(uc, cc, lc); // Calculate error in the middle of current segment
			if (error > max_error) {
				// Error is too high, chop current segment in half and try it again
				*one = c1; // Shorter control_next
				*two = c2; // Shorter control_prev
				segment.insert(two, cc); // Add point in the middle of center line
				--two;
				upper.segment.erase(u,upper.segment.end()); // Drop point(s) added in this iteration of while cycle to upper boundary
				lower.segment.erase(l,lower.segment.end()); // Drop point(s) added in this iteration of while cycle to lower boundary

				std::list<v_point> up;
				shift(segment, one, up, 1); // Recalculate control_next for begining of current segment (upper line)
				upper.segment.back() = up.back(); // Correct last upper point acording to newly chopped center segment
				std::list<v_point> down;
				shift(segment, one, down, -1); // Recalculate control_next for begining of current segment (lower line)
				lower.segment.back() = down.back(); // Correct last lower point acording to newly chopped center segment
			}
		} while (error > max_error); // If we choped our line, we need to add first segment again
		one=two;
		two++;
	}
	upper.reverse(); // Reverse upper line
	lower.segment.splice(lower.segment.end(), upper.segment); // Connect everything together -- points on outline are in a counterclockwise order
	lower.segment.push_back(lower.segment.front());
	std::swap(lower.segment, segment); // Replace centerline with outline
	set_type(fill);
}

void v_image::convert_to_variable_width(int type, output_params &par) { // Convert lines before exporting to support variable-width lines
	for (auto c = line.begin(); c != line.end(); c++) {
		std::list<v_line> new_list;
		int new_type = type;
		if (type == 3) { // Automatic convert - chnage only lines with variable width
			p mean = 0;
			p count = 0;
			for (auto a: c->segment) { // Calculate mean width
				mean += a.width;
				count++;
			}
			if (count == 0)
				new_type = 0;
			else {
				mean /= count;
				p variance = 0;
				for (auto a: c->segment) { // Calculate variance of width
					variance += (a.width - mean) * (a.width - mean);
				}
				variance /= count;
				if (variance > par.auto_contour_variance)
					new_type = 2; // Width is changing too much, calculate outline and fill it
				else
					new_type = 0; // Line has (almost) constant width, do nothing
			}
		}
		switch (new_type) {
			case 0: // Do not convert anything
				break;
			case 1: // Chop each line to separate segments
				group_line(new_list, *c);
				line.splice(c, new_list);
				line.erase(c);
				c--;
				break;
			case 2: // Convert line to its outline and fill it
				c->convert_to_outline(par.max_contour_error);
				break;
		}
	}
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

void v_line::auto_smooth() { // Forget all control points (except unused - first and last) and place them so the line is smooth
	for (auto pt = segment.begin(); pt != segment.end(); pt++) {
		auto prev = pt;
		if (prev == segment.begin()) // Skip first point
			continue;
		prev--;
		auto next = pt;
		next++;
		if (next == segment.end()) // Skip last point
			continue;
		v_pt vecp = prev->main - pt->main; // Direction to previous
		v_pt vecn = next->main - pt->main; // Direction to next
		p pl = vecp.len();
		p nl = vecn.len();
		if ((pl <= epsilon) || (nl <= epsilon)) // Current point is corner -> do not make it smooth
			continue;
		vecp /= pl; // Make direction unit vector
		vecn /= nl;
		v_pt control = vecn - vecp; // Direction for next control point
		control /= control.len(); // Normalize
		pt->control_prev = pt->main - (control * pl/3); // Place control point to 1/3 distance of next main point
		pt->control_next = pt->main + (control * nl/3);
	}
}

v_pt intersect(v_pt a, v_pt b, v_pt c, v_pt d) { // Calculate intersection of line AB with line CD. A and C are absolute coordinates. B is relative to A, D is relative to C
	// Geometry background is described in a documentation
	/*
	// Alternative version with unpleasant singularity
	c -= a;
	p dt = d.y/d.x;
	p t = c.y - dt*c.x;
	t /= b.y - dt*b.x;
	b *= t;
	a += b;
	return a;
	*/
	c -= a;
	b /= b.len(); // Change length to 1 unit
	v_pt c_proj = b*(c.x*b.x + c.y*b.y); // Project C to direction b
	v_pt d_proj = b*(d.x*b.x + d.y*b.y); // Project D to direction b
	p cl = distance(c_proj, c); // Distance of c from line AB
	p dl = distance(d_proj, d); // Distance of d from line AB
	d *= cl/dl; // Scale d to make it in same distance as c (from line AB)
	v_pt intersection = d + c; // Calculate position of intersection
	v_pt proj = b*(intersection.x*b.x + intersection.y*b.y);
	if (distance(proj, intersection) > epsilon)
		// Vector d is leading away from line AB. Intersection is placed at c - d
		d *= -1;
	return d+c+a; // Absolute position of intersection
}

}; // namespace
