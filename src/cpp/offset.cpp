#include "geom.h"
#include "offset.h"
#include "v_image.h"
#include "parameters.h"
#include <list>
#include <cmath>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>

namespace vect {

void offset::rot(v_pt &pt, int sign) { // rotate vector by +- 90 degrees (sign == 1: rotate right; sign == -1: rotate left)
	p x = pt.x;
	p y = pt.y;
	pt.x = y * sign;
	pt.y = -x * sign;
}

void offset::shift(const std::list<v_point> &context, std::list<v_point>::iterator pts, std::list<v_point> &output, int sign) { // Calculate position of new points when shifting point to boundary. Input: context + iterator to current point on line, sign (shift up +1 / down -1); Output: list of new point(s) (one or two)
	v_point pt = *pts;
	if (geom::distance(pt.control_prev, pt.main) < epsilon) { // Previous control point not set
		if (pts != context.begin()) {
			auto tmp = pts;
			--tmp;
			pt.control_prev = tmp->main; // Replace with direction to previous main point
		}
	}
	if (geom::distance(pt.control_next, pt.main) < epsilon) { // Next control point not set
		auto tmp = pts;
		++tmp;
		if (tmp != context.end()) {
			pt.control_next = tmp->main; // Replace with direction to previous main point
		}
	}
	if (geom::distance(pt.control_prev, pt.main) < epsilon) { // Previous direction is still undefined
		pt.control_prev.x = 2*pt.main.x - pt.control_next.x; // Assume, that point is autosmooth: reverse direction to next control point
		pt.control_prev.y = 2*pt.main.y - pt.control_next.y;
	}
	if (geom::distance(pt.control_next, pt.main) < epsilon) { // Next direction is still undefined
		pt.control_next.x = 2*pt.main.x - pt.control_prev.x; // Assume, that point is autosmooth: reverse direction to previous control point
		pt.control_next.y = 2*pt.main.y - pt.control_prev.y;
	}
	if (geom::distance(pt.control_prev, pt.main) < epsilon) { // There is no control point at all (probably only one point on line -> should be replaced with circle)
		pt.control_prev = pt.main;
		--pt.control_prev.x; // Previous control point will be on left
		pt.control_next = pt.main;
		++pt.control_next.x; // Next control point will be on right
	}
	p dprev = geom::distance(pt.control_prev, pt.main);
	p dnext = geom::distance(pt.control_next, pt.main);
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

#ifdef OUTLINE_DEBUG
void offset::offset_debug(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}
#else
void offset::offset_debug(const char *format, ...) {}; // Does nothing
#endif

p offset::calculate_error(const v_point &uc, const v_point &cc, const v_point &lc) { // Measure error between upper bound (uc), lower bound (lc) and line defined by point cc and width
	offset_debug("calculate_error: lc: %f, %f; cc: %f, %f; uc: %f, %f\n", lc.main.x, lc.main.y, cc.main.x, cc.main.y, uc.main.x, uc.main.y);
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
	offset_debug("calculate_error: u: %f, %f; cc: %f, %f; l: %f, %f\n", u.x, u.y, cc.main.x, cc.main.y, l.x, l.y);
	offset_debug("calculate_error: distance %f, %f\n", error_u, error_l);
	return error_u + error_l;
}

void offset::one_point_circle(v_line &line) {
	v_point point = line.segment.front();
	line.segment.erase(line.segment.begin(), line.segment.end());

	p w = point.width/2;
	p c = w * 4 / 3 * (sqrt(2) - 1); // = (w - (sqrt(2)/2*w))*4/3*sqrt(2)

	v_point curr = point;

	// Right
	curr.main = point.main + v_pt(w, 0);
	curr.control_prev = curr.main + v_pt(0, c);
	curr.control_next = curr.main + v_pt(0, -c);
	line.segment.push_back(curr);

	// Up
	curr.main = point.main + v_pt(0, -w);
	curr.control_prev = curr.main + v_pt(c, 0);
	curr.control_next = curr.main + v_pt(-c, 0);
	line.segment.push_back(curr);

	// Left
	curr.main = point.main + v_pt(-w, 0);
	curr.control_prev = curr.main + v_pt(0, -c);
	curr.control_next = curr.main + v_pt(0, c);
	line.segment.push_back(curr);

	// Down
	curr.main = point.main + v_pt(0, w);
	curr.control_prev = curr.main + v_pt(-c, 0);
	curr.control_next = curr.main + v_pt(c, 0);
	line.segment.push_back(curr);

	// Close with bezier curve
	line.segment.push_back(line.segment.front());

	line.set_type(fill);
}

v_pt find_tangent(v_pt main, v_pt next, p width, p width_next) {
	v_pt base = next - main;
	p dl = 3 * base.len();
	p dw = width - width_next;
	p l = width/dw * dl;

	p angle = std::acos(width / l);
	base /= base.len();
	base *= width;
	base = geom::rotate(base, angle);

	base += main;
	return base;
}


v_line offset::smooth_segment_outline(std::list<v_point>::iterator one, std::list<v_point>::iterator two) {
	--two;
	v_line line;

	if (geom::distance(one->control_next, one->main) < epsilon) {
		one->control_next = one->main * 2/3 + two->main / 3;
	}

	if (geom::distance(two->control_prev, two->main) < epsilon) {
		two->control_prev = two->main * 2/3 + one->main / 3;
	}


	line.add_point(one->main);

	v_pt base = find_tangent(one->main, one->control_next, one->width, two->width);
	line.add_point(base);

	base = find_tangent(two->main, two->control_prev, two->width, one->width);
	line.add_point(base);

	line.add_point(one->control_next, two->control_prev, two->main);
	//line.set_type(fill);

	return line;
}

void offset::convert_to_outline(v_line &line, p max_error) { // Calculate outline of each line
	if (line.get_type() == fill) // It is already outline
		return;

	auto two = line.segment.begin(); // Right point of current segment
	auto one = two; // Left point of current segment
	if (two == line.segment.end()) { // Only one point
		one_point_circle(line);
		return;
	}

	++two; // Change to second point

	std::list<v_line> smooth_segments;
	// Naming assumes that the line stored in segment continues to the left
	do {
		// TODO uncomment and implement longer segments: & make it working
		//while (two != line.segment.end() && two->get_smooth() != corner)
			//++two;
		//if (two != line.segment.end())
			++two;

		smooth_segments.emplace_back(smooth_segment_outline(one, two));
		one = two;
		--one;
	} while (two != line.segment.end());

	// TODO merge segments:
	std::swap(line, smooth_segments.back());
	//
	/*
	std::list<v_point> up; // Upper boundary of line
	shift(line.segment, one, up, 1); // Shift start of line up
	upper.segment.splice(upper.segment.end(), up);
	std::list<v_point> down; // Lower boundary of line
	shift(line.segment, one, down, -1); // Shift start of line down
	lower.segment.splice(lower.segment.end(), down);

	while (two != line.segment.end()) { // Until we run out of segments
		p error;
		do {
			auto u = upper.segment.end();
			auto l = lower.segment.end();
			v_point u1 = *(--u); // Last new (upper) point -- left end of current segment
			v_point l1 = *(--l); // Last new (lower) point -- left end of current segment

			std::list<v_point> up;
			shift(line.segment, two, up, 1); // Shift second end of current segment up
			upper.segment.splice(upper.segment.end(), up);
			std::list<v_point> down;
			shift(line.segment, two, down, -1); // Shift second end of current segment up
			lower.segment.splice(lower.segment.end(), down);

			v_point u2 = *(++u); // Right end of current segment (upper boundary)
			v_point l2 = *(++l); // Right end of current segment (lower boundary)

			v_point c1 = *one; // Left end of center line
			v_point c2 = *two; // Right end of center line
			v_point uc;
			v_point cc;
			v_point lc;
			geom::bezier_chop_in_half(u1, u2, uc); // Chop upper segment in half
			geom::bezier_chop_in_half(c1, c2, cc); // Chop center segment in half
			geom::bezier_chop_in_half(l1, l2, lc); // Chop lower segment in half

			error = calculate_error(uc, cc, lc); // Calculate error in the middle of current segment
			if (error > max_error) {
				// Error is too high, chop current segment in half and try it again
				*one = c1; // Shorter control_next
				*two = c2; // Shorter control_prev
				line.segment.insert(two, cc); // Add point in the middle of center line
				--two;
				upper.segment.erase(u,upper.segment.end()); // Drop point(s) added in this iteration of while cycle to upper boundary
				lower.segment.erase(l,lower.segment.end()); // Drop point(s) added in this iteration of while cycle to lower boundary

				std::list<v_point> up;
				shift(line.segment, one, up, 1); // Recalculate control_next for begining of current segment (upper line)
				upper.segment.back() = up.back(); // Correct last upper point acording to newly chopped center segment
				std::list<v_point> down;
				shift(line.segment, one, down, -1); // Recalculate control_next for begining of current segment (lower line)
				lower.segment.back() = down.back(); // Correct last lower point acording to newly chopped center segment
			}
		} while (error > max_error); // If we choped our line, we need to add first segment again
		one=two;
		two++;
	}
	upper.reverse(); // Reverse upper line
	lower.segment.splice(lower.segment.end(), upper.segment); // Connect everything together -- points on outline are in a counterclockwise order
	lower.segment.push_back(lower.segment.front());
	std::swap(lower.segment, line.segment); // Replace centerline with outline
	*/
}


}; // namespace
