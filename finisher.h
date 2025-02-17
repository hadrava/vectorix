/*
 * Vectorix -- line-based image vectorizer
 * (c) 2016 Jan Hadrava <had@atrey.karlin.mff.cuni.cz>
 */
#ifndef VECTORIX__FINISHER_H
#define VECTORIX__FINISHER_H

#include "v_image.h"
#include "parameters.h"

namespace vectorix {

class finisher {
private:
	parameters *par;
	p *param_false_colors;
	int *param_force_black;
	p *param_force_width;
	p *param_force_opacity;
	std::string *param_underlay_path;
	int *param_debug_lines;
public:
	finisher(parameters &params) {
		par = &params;
		par->bind_param(param_false_colors, "false_colors", (p) 0);
		par->add_comment("Set color of every line to black:");
		par->bind_param(param_force_black, "force_black", 1);
		par->bind_param(param_force_width, "force_width", (p) 0);
		par->bind_param(param_force_opacity, "force_opacity", (p) 0);
		par->bind_param(param_underlay_path, "underlay_image", (std::string) "");
		par->add_comment("Show debug lines: 0 = off");
		par->bind_param(param_debug_lines, "debug_lines", 0);
	};
	void apply_settings(v_image &vector);
};

}; // namespace

#endif
