#ifndef VECTORIX__OFFSET_H
#define VECTORIX__OFFSET_H

// Basic vector data structures and transformations

#include <list>
#include <vector>
#include <cmath>
#include "v_image.h"
#include "config.h"
#include "parameters.h"
#include "approximation.h"

namespace vectorix {

class offset {
public:
	offset(v_image &img, parameters &params): apx(params), par(&params), image(&img) {
		int *param_offset_verbosity;
		par->bind_param(param_offset_verbosity, "offset_verbosity", (int) log_level::info);
		log.set_verbosity((log_level) *param_offset_verbosity);

		par->bind_param(param_offset_error, "offset_error", (p) 1);
		par->bind_param(param_offset_iterations, "offset_iterations", 5);
		apx.bind_limits(*param_offset_error, *param_offset_iterations);
	}
	void convert_to_outline(v_line &line); // Convert from stroke to fill (calculate line outline)
private:
	p *param_offset_error;
	int *param_offset_iterations;

	void one_point_circle(v_line &line); // Line containing one point will be converted to outline
	bool segment_outline(v_point &one, v_point &two, std::vector<v_point> &outline);

	v_pt find_cap_end(v_pt main, v_pt next, p width);

	void prepare_tangent_offset_points(std::vector<p> &times, std::vector<v_pt> &center_pt, std::vector<p> &width, std::vector<v_pt> &offset_pt, int check_point_count, const v_point &a, const v_point &b, const v_pt &c_main, const v_pt &c_next, const v_pt &d_prev, const v_pt &d_main, p c_width, p d_width);
	int remove_hidden_offset_points(std::vector<v_pt> &center_pt, std::vector<v_pt> &offset_pt, std::vector<p> &times, std::vector<p> &width);
	v_pt find_tangent(v_pt main, v_pt next, v_pt next_backup, p width, p width_next, p sign);

	v_point calculate_control_points_perpendicular(v_pt pt, v_pt inside);
	void set_circle_control_point_lengths(v_point &a, v_point &b, const v_pt &center, p width);

	bool optimize_offset_control_point_lengths(v_point &a, v_point &b, const v_pt &c_main, const v_pt &c_next, const v_pt &d_prev, const v_pt &d_main, p c_width, p d_width);
	bool optimize_control_point_lengths(const std::vector<v_pt> &points, std::vector<p> &times, const v_pt &a_main, v_pt &a_next, v_pt &b_prev, const v_pt &b_main);

	v_image *image;
	logger log;
	parameters *par;
	approximation apx;
};


}; // namespace
#endif
