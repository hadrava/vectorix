/*
 * Vectorix -- line-based image vectorizer
 * (c) 2016 Jan Hadrava <had@atrey.karlin.mff.cuni.cz>
 */
#ifndef VECTORIX__APPROXIMATION_H
#define VECTORIX__APPROXIMATION_H

#include <vector>
#include "v_image.h"
#include "config.h"
#include "parameters.h"
#include "logger.h"

namespace vectorix {

class approximation {
public:
	approximation(parameters &params): par(&params) {
		int *param_approximation_verbosity;
		par->bind_param(param_approximation_verbosity, "approximation_verbosity", (int) log_level::warning);
		log.set_verbosity((log_level) *param_approximation_verbosity);

		par->add_comment("Variable-width export: 0: mean, 1: grouped, 2: contour, 3: auto-detect");
		par->bind_param(param_export_type, "export_type", 0);
		par->add_comment("Least squares method: 0 = OpenCV (default, suggested), 1 = own implementation");
		par->bind_param(param_lsq_method, "lsq_method", 0);
		par->add_comment("Approximation of Bezier splines");
		par->bind_param(param_approximation_error, "approximation_error", (p) 1.8);
		par->bind_param(param_approximation_iterations, "approximation_iterations", 5);
		par->bind_param(param_approximation_preserve_corners, "approximation_preserve_corners", 1);
	}
	bool optimize_control_point_lengths(const std::vector<v_pt> &points, std::vector<p> &times, const v_pt &a_main, v_pt &a_next, v_pt &b_prev, const v_pt &b_main);
	void bind_limits(p &error, int &iterations) {
		param_approximation_error = &error;
		param_approximation_iterations = &iterations;
	};
	void run(v_image &image);
private:
	int *param_lsq_method;
	int *param_export_type;
	p *param_approximation_error;
	int *param_approximation_iterations;
	int *param_approximation_preserve_corners;

	bool approximate_with_one_segment(const std::list<v_point>::iterator begin, const std::list<v_point>::iterator end, v_point &a, v_point &b);
	std::list<v_point>::iterator find_longest_aproximable(const std::list<v_point>::iterator begin, const std::list<v_point>::iterator end);


	logger log;
	parameters *par;
};


}; // namespace
#endif
