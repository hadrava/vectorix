#ifndef VECTORIX__APPROXIMATION_H
#define VECTORIX__APPROXIMATION_H

#include <vector>
#include "v_image.h"
#include "config.h"
#include "parameters.h"

namespace vectorix {

class approximation {
public:
	approximation(parameters &params) {
		par = &params;
		par->add_comment("Least squares method: 0 = OpenCV (default, suggested), 1 = own implementation");
		par->bind_param(param_lsq_method, "lsq_method", 0);
		par->add_comment("Approximation of Bezier splines");
		par->bind_param(param_approximation_error, "approximation_error", (p) 1);
		par->bind_param(param_approximation_iterations, "approximation_iterations", 5);
	}
	bool optimize_control_point_lengths(const std::vector<v_pt> &points, std::vector<p> &times, const v_pt &a_main, v_pt &a_next, v_pt &b_prev, const v_pt &b_main);
	void bind_limits(p &error, int &iterations) {
		param_approximation_error = &error;
		param_approximation_iterations = &iterations;
	};
private:
	int *param_lsq_method;
	p *param_approximation_error;
	int *param_approximation_iterations;
	parameters *par;
};


}; // namespace
#endif
