#include "v_image.h"
#include "parameters.h"
#include <list>
#include <cmath>
#include <cstdio>

namespace vect {

void v_image::clean() {
	for (v_line _line: line) {
		_line.segment.erase(_line.segment.begin(), _line.segment.end());
	}
	line.erase(line.begin(), line.end());
}

v_line::v_line(p x0, p y0, p x1, p y1, p x2, p y2, p x3, p y3) {
	segment.emplace_back(v_point(v_pt(x0,y0), v_pt(x0,y0), v_pt(x1,y1)));
	segment.emplace_back(v_point(v_pt(x2,y2), v_pt(x3,y3), v_pt(x3,y3)));
	type_ = stroke;
}

void v_line::add_point(v_pt &&_p_control_next, v_pt &&_control_prev, v_pt &&_main) {
	segment.back().control_next = _p_control_next;
	segment.emplace_back(v_point(_control_prev, _main, _main));
}

void v_line::add_point(v_pt &&_p_control_next, v_pt &&_control_prev, v_pt &&_main, v_co _color, p _width) {
	segment.back().control_next = _p_control_next;
	segment.emplace_back(v_point(_control_prev, _main, _main, _color, _width));
}

void v_line::add_point(v_pt &&_main) {
	segment.emplace_back(v_point(_main, _main, _main));
}

void v_line::add_point(v_pt &&_main, v_co _color, p _width) {
	segment.emplace_back(v_point(_main, _main, _main, _color, _width));
}

void v_line::reverse() {
	segment.reverse();
	for (auto control = segment.begin(); control != segment.end(); control++) {
		std::swap(control->control_next, control->control_prev);
	}
};

void v_image::add_line(v_line _line) {
	line.push_back(_line);
}

p distance(const v_pt &a, const v_pt &b) {
	p x = (a.x - b.x);
	p y = (a.y - b.y);
	return std::sqrt(x*x + y*y);
}

p distance(const v_point &a, const v_point &b) {
	return distance(a.main, a.control_next) + distance(a.control_next, b.control_prev) + distance(b.control_prev, b.main);
}

void chop_in_half(v_point &one, v_point &two, v_point &newpoint) {
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

void chop_line(v_line &line, p max_distance) {
	auto two = line.segment.begin();
	auto one = two;
	if (two != line.segment.end())
		++two;
	while (two != line.segment.end()) {
		while (distance(*one, *two) > max_distance) {
			v_point newpoint;
			chop_in_half(*one, *two, newpoint);
			line.segment.insert(two, newpoint);
			--two;
		}
		one=two;
		two++;
	}
}

void group_line(std::list<v_line> &list, const v_line &line) {
	auto two = line.segment.begin();
	auto one = two;
	if ((two != line.segment.end()) && (line.get_type() == stroke))
		++two;
	else {
		list.push_back(line);
		return;
	}
	int segment_count = 0;
	while (two != line.segment.end()) {
		v_line new_line;
		new_line.segment.push_back(*one);
		new_line.segment.push_back(*two);
		new_line.set_type(stroke);
		new_line.set_group(group_continue);
		list.push_back(new_line);

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

void rot(v_pt &pt, int sign) {
	p x = pt.x;
	p y = pt.y;
	pt.x = y * sign;
	pt.y = -x * sign;
}

void shift(const std::list<v_point> &context, std::list<v_point>::iterator pts, std::list<v_point> &output, int sign) {
	v_point pt = *pts;
	if (distance(pt.control_prev, pt.main) < epsilon) {
		if (pts != context.begin()) {
			auto tmp = pts;
			--tmp;
			pt.control_prev = tmp->main;
		}
	}
	if (distance(pt.control_next, pt.main) < epsilon) {
		auto tmp = pts;
		++tmp;
		if (tmp != context.end()) {
			pt.control_next = tmp->main;
		}
	}
	if (distance(pt.control_prev, pt.main) < epsilon) {
		pt.control_prev.x = 2*pt.main.x - pt.control_next.x;
		pt.control_prev.y = 2*pt.main.y - pt.control_next.y;
	}
	if (distance(pt.control_next, pt.main) < epsilon) {
		pt.control_next.x = 2*pt.main.x - pt.control_prev.x;
		pt.control_next.y = 2*pt.main.y - pt.control_prev.y;
	}
	if (distance(pt.control_prev, pt.main) < epsilon) {
		pt.control_prev = pt.main;
		--pt.control_prev.x;
		pt.control_next = pt.main;
		++pt.control_next.x;
	}
	p dprev = distance(pt.control_prev, pt.main);
	p dnext = distance(pt.control_next, pt.main);
	v_pt prev, next;
	prev.x = (pt.control_prev.x - pt.main.x) / dprev;
	prev.y = (pt.control_prev.y - pt.main.y) / dprev;
	next.x = (pt.control_next.x - pt.main.x) / dnext;
	next.y = (pt.control_next.y - pt.main.y) / dnext;

	v_pt prot = prev;
	v_pt nrot = next;
	rot(prot, -sign);
	rot(nrot, sign);

	v_pt shift;
	shift.x = prot.x + nrot.x;
	shift.y = prot.y + nrot.y;

	if ((prot.x * next.x + prot.y * next.y) >= -epsilon) {
		p d = shift.x * prot.x + shift.y * prot.y;
		shift.x *= pts->width / 2 / d;
		shift.y *= pts->width / 2 / d;

		v_point out = *pts;
		out.main.x += shift.x;
		out.main.y += shift.y;

		if (pts != context.begin()) {
			out.control_prev.x += shift.x;
			out.control_prev.y += shift.y;
		}
		else {
			out.control_prev.x = out.main.x + prev.x*out.width*2/3;
			out.control_prev.y = out.main.y + prev.y*out.width*2/3;
		}
		auto tmp = pts;
		++tmp;
		if (tmp != context.end()) {
			out.control_next.x += shift.x;
			out.control_next.y += shift.y;
		}
		else {
			out.control_next.x = out.main.x + next.x*out.width*2/3;
			out.control_next.y = out.main.y + next.y*out.width*2/3;
		}
		out.width = 1;

		output.push_back(out);
	}
	else {
		p lshift = std::sqrt(shift.x*shift.x + shift.y*shift.y);
		shift.x /= lshift;
		shift.y /= lshift;
		p want = 1 - (prot.x*shift.x + prot.y*shift.y);
		p curr = -prev.x*shift.x - prev.y*shift.y;

		v_point out1 = *pts;
		out1.main.x += prot.x * out1.width/2;
		out1.main.y += prot.y * out1.width/2;
		out1.control_prev.x += prot.x * out1.width/2;
		out1.control_prev.y += prot.y * out1.width/2;
		out1.control_next.x = out1.main.x - prev.x*out1.width*2/3*want/curr;//TODO
		out1.control_next.y = out1.main.y - prev.y*out1.width*2/3*want/curr;//TODO
		out1.width = 1;

		v_point out2 = *pts;
		out2.main.x += nrot.x * out2.width/2;
		out2.main.y += nrot.y * out2.width/2;
		out2.control_prev.x = out2.main.x - next.x*out2.width*2/3*want/curr;//TODO
		out2.control_prev.y = out2.main.y - next.y*out2.width*2/3*want/curr;//TODO
		out2.control_next.x += nrot.x * out2.width/2;
		out2.control_next.y += nrot.y * out2.width/2;
		out2.width = 1;

		output.push_back(out1);
		output.push_back(out2);
	}
}

p calculate_error(const v_point &uc, const v_point &cc, const v_point &lc) {
	fprintf(stderr, "calculate_error: lc: %f, %f; cc: %f, %f; uc: %f, %f\n", lc.main.x, lc.main.y, cc.main.x, cc.main.y, uc.main.x, uc.main.y);
	//u.x = lc.control_next.x - uc.control_prev.x
	//u.y = lc.control_next.y - uc.control_prev.y
	v_pt c = cc.control_next - cc.control_prev;
	p len = std::sqrt(c.x*c.x + c.y*c.y);
	c /= len;
	rot(c, 1);
	//l.x = lc.control_next.x - lc.control_prev.x
	//l.y = lc.control_next.y - lc.control_prev.y
	//up = c
	//low = -c
	v_pt u = uc.main - cc.main;
	v_pt l = lc.main - cc.main;
	p error_u = c.x*u.x + c.y*u.y - cc.width/2;
	p error_l = c.x*l.x + c.y*l.y + cc.width/2;
	error_u *= error_u;
	error_l *= error_l;
	fprintf(stderr, "calculate_error: u: %f, %f; cc: %f, %f; l: %f, %f\n", u.x, u.y, cc.main.x, cc.main.y, l.x, l.y);
	fprintf(stderr, "calculate_error: distance %f, %f\n", error_u, error_l);
	return error_u + error_l;
}

void v_line::convert_to_outline(p max_error) {
	if (get_type() == fill)
		return;
	v_line upper;
	v_line lower;
	auto two = segment.begin();
	auto one = two;
	if (two != segment.end())
		++two;
	else {
		set_type(fill);
		return;
	}
	std::list<v_point> up;
	shift(segment, one, up, 1);
	upper.segment.splice(upper.segment.end(), up);
	std::list<v_point> down;
	shift(segment, one, down, -1);
	lower.segment.splice(lower.segment.end(), down);

	while (two != segment.end()) {
		p error;
		do {
			auto u = upper.segment.end();
			auto l = lower.segment.end();
			v_point u1 = *(--u);
			v_point l1 = *(--l);

			std::list<v_point> up;
			shift(segment, two, up, 1);
			upper.segment.splice(upper.segment.end(), up);
			std::list<v_point> down;
			shift(segment, two, down, -1);
			lower.segment.splice(lower.segment.end(), down);

			v_point u2 = *(++u);
			v_point l2 = *(++l);

			v_point c1 = *one;
			v_point c2 = *two;
			v_point uc;
			v_point cc;
			v_point lc;
			chop_in_half(u1, u2, uc);
			chop_in_half(c1, c2, cc);
			chop_in_half(l1, l2, lc);

			error = calculate_error(uc, cc, lc);
			if (error > max_error) {
				*one = c1;
				*two = c2;
				segment.insert(two, cc);
				--two;
				upper.segment.erase(u,upper.segment.end());
				lower.segment.erase(l,lower.segment.end());

				std::list<v_point> up;
				shift(segment, one, up, 1);
				upper.segment.back() = up.back();
				std::list<v_point> down;
				shift(segment, one, down, -1);
				lower.segment.back() = down.back();
			}
		} while (error > max_error);
		one=two;
		two++;
	}
	upper.reverse();
	lower.segment.splice(lower.segment.end(), upper.segment);
	lower.segment.push_back(lower.segment.front());
	std::swap(lower.segment, segment);
	set_type(fill);
}

void v_image::convert_to_variable_width(int type, output_params &par) {
	for (auto c = line.begin(); c != line.end(); c++) {
		std::list<v_line> new_list;
		int new_type = type;
		if (type == 3) {
			p mean = 0;
			p count = 0;
			for (auto a: c->segment) {
				mean += a.width;
				count++;
			}
			if (count == 0)
				new_type = 0;
			else {
				mean /= count;
				p variance = 0;
				for (auto a: c->segment) {
					variance += (a.width - mean) * (a.width - mean);
				}
				variance /= count;
				if (variance > par.auto_contour_variance)
					new_type = 2;
				else
					new_type = 0;
			}
		}
		switch (new_type) {
			case 0:
				break;
			case 1:
				group_line(new_list, *c);
				line.splice(c, new_list);
				line.erase(c);
				c--;
				break;
			case 2:
				c->convert_to_outline(par.max_contour_error);
				break;
		}
	}
}

void v_line::set_color(v_co color) {
	for (auto pt = segment.begin(); pt != segment.end(); pt++) {
		pt->color = color;
	}
}

void v_image::false_colors(p hue_step) {
	p hue = 0;
	for (auto l = line.begin(); l != line.end(); l++) {
		v_co col = v_co::from_color(hue);
		l->set_color(col);
		hue += hue_step;
		hue = fmodf (hue, 360);
	}
}

void v_line::auto_smooth() {
	for (auto pt = segment.begin(); pt != segment.end(); pt++) {
		auto prev = pt;
		if (prev == segment.begin())
			continue;
		prev--;
		auto next = pt;
		next++;
		if (next == segment.end())
			continue;
		v_pt vecp = prev->main - pt->main;
		v_pt vecn = next->main - pt->main;
		p pl = std::sqrt(vecp.x*vecp.x + vecp.y*vecp.y);
		p nl = std::sqrt(vecn.x*vecn.x + vecn.y*vecn.y);
		if ((pl <= epsilon) || (nl <= epsilon))
			continue;
		vecp /= pl;
		vecn /= nl;
		v_pt control = vecn - vecp;
		p cl = std::sqrt(control.x*control.x + control.y*control.y);
		pt->control_prev = pt->main - ((control/cl) * pl/3);
		pt->control_next = pt->main + ((control/cl) * nl/3);
	}
}

}; // namespace
