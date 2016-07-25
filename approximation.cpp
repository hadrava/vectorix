#include <vector>
#include "v_image.h"
#include "config.h"
#include "parameters.h"
#include "geom.h"
#include "least_squares_simple.h"
#include "least_squares_opencv.h"
#include "approximation.h"

namespace vectorix {

void approximation::run(v_image &image) {
	for (v_line &li: image.line) {
		log.log<log_level::debug>("Approximating line of %d points\n", li.segment.size());
		auto two = li.segment.begin(); // Right point of current segment
		if (two == li.segment.end())
			continue; // Empty line
		auto one = two; // Left point of current segment
		++two; // Change to the second point
		if (two == li.segment.end())
			continue; // Only one point
		++two; // Change to the second point
		if (two == li.segment.end())
			continue; // Only one segment

		// now we have two segments
		do {
			one++;
			two++;
		} while (two != li.segment.end());
		two = one;
		one = li.segment.begin();

		approximate_with_one_segment(li.segment.begin(), li.segment.end(), *one, *two);
		one++;
		li.segment.erase(one, two);
		log.log<log_level::debug>("Approximated with %d points\n", li.segment.size());
	}
}

bool approximation::approximate_with_one_segment(const std::list<v_point>::iterator begin, const std::list<v_point>::iterator end, v_point &a, v_point b) {
	auto two = begin; // Right point of current segment
	auto one = two; // Left point of current segment
	++two; // Change to the second point

	std::vector<v_pt> points;
	do {
		for (int i = 1; i < 8; i++) {
			v_point middle;
			geom::bezier_chop_in_t(*one, *two, middle, i/8.);
			points.push_back(middle.main);
		}
		points.push_back(two->main);
		one++;
		two++;
	} while (two != end);
	points.pop_back();

	std::vector<p> times;
	p total_length = geom::distance(begin->main, points[0]);
	times.push_back(total_length);
	for (int i = 1; i < points.size(); i++) {
		total_length += geom::distance(points[i-1], points[i]);
		times.push_back(total_length);
	}
	total_length += geom::distance(points.back(), one->main);

	for (int i = 0; i < times.size(); i++) {
		times[i] /= total_length;
	}

	a.main = begin->main;
	a.control_next = begin->control_next;
	b.control_prev = one->control_prev;
	b.main = one->main;

	return optimize_control_point_lengths(points, times, a.main, a.control_next, b.control_prev, b.main);
}

bool approximation::optimize_control_point_lengths(const std::vector<v_pt> &points, std::vector<p> &times, const v_pt &a_main, v_pt &a_next, v_pt &b_prev, const v_pt &b_main) {
	v_pt a_n = a_next - a_main;
	v_pt b_p = b_prev - b_main;

	int iteration = 0;

	// 1.5, preacalculate length for step 3
	p total_length = geom::distance(a_main, points[0]);
	for (int i = 1; i < points.size(); i++) {
		total_length += geom::distance(points[i-1], points[i]);
	}
	total_length += geom::distance(points.back(), b_main);

	while (true) {
		// 2, Compute coefficients by least squares
		std::unique_ptr<least_squares> mat;
		if (*param_lsq_method == 0)
			mat = std::unique_ptr<least_squares>(new least_squares_opencv(2, *par));
		else
			mat = std::unique_ptr<least_squares>(new least_squares_simple(2, *par));
		for (int i = 0; i < times.size(); i++) {
			p t = times[i];
			p s = 1 - t;

			// Equation (before subtraction):
			//s*s*s*a_main + 3*s*s*t*a_n + 3*s*t*t*b_p + t*t*t*b_main == points[i];
			//s*s*s*a_main + 3*s*s*t*(a_main + a_n) + 3*s*t*t*(b_main + b_p) + t*t*t*b_main == points[i];
			//s*s*s*a_main + 3*s*s*t*a_main + 3*s*s*t*a_n + 3*s*t*t*b_main + 3*s*t*t*b_p + t*t*t*b_main == points[i];

			v_pt c0 = a_n*3*s*s*t;
			v_pt c1 = b_p*3*s*t*t;
			v_pt c2 = points[i] - a_main*s*s*s - a_main*3*s*s*t - b_main*3*s*t*t - b_main*t*t*t;

			p eqx[] = {c0.x, c1.x, c2.x};
			mat->add_equation(eqx);
			p eqy[] = {c0.y, c1.y, c2.y};
			mat->add_equation(eqy);
		}
		mat->evaluate();
		p error = mat->calc_error();
		log.log<log_level::debug>("Approximation error %f\n", error);

		// 3, Find improved parametrization
		v_point a, b;
		a.main = a_main;
		a.control_next = a_n * (*mat)[0] + a_main;

		b.main = b_main;
		b.control_prev = b_p * (*mat)[1] + b_main;
		for (int i = 0; i < times.size(); i++) {
			v_point middle;
			geom::bezier_chop_in_t(a, b, middle, times[i], true);

			v_pt error_vector = points[i] - middle.main;
			middle.control_next -= middle.main;
			middle.control_next /= middle.control_next.len();

			log.log<log_level::debug>("Approximation time %f", times[i]);
			times[i] += geom::dot_product(middle.control_next, error_vector) / total_length;
			if (times[i] < 0.)
				times[i] = 0.;
			else if (times[i] > 1.)
				times[i] = 1.;
			log.log<log_level::debug>(" -> %f\n", times[i]);
		}

		// 4
		iteration++;

		// 5
		if (error < *param_approximation_error) {
			a_next = a_n * (*mat)[0] + a_main;
			b_prev = b_p * (*mat)[1] + b_main;

#ifdef VECTORIX_OUTLINE_DEBUG
			v_point a, b;
			a.main = a_main;
			a.control_next = a_next;

			b.main = b_main;
			b.control_prev = b_prev;

			for (int i = 0; i < times.size(); i++) {
				v_point middle;
				geom::bezier_chop_in_t(a, b, middle, times[i], true);
				v_image::add_debug_line(middle.main, points[i]);
			}
#endif

			return true;
		}
		else if (iteration >= *param_approximation_iterations) {
			a_next = a_n * (*mat)[0] + a_main;
			b_prev = b_p * (*mat)[1] + b_main;

			return false; // Split segment and try again
		}
	}
}

}; // namespace
