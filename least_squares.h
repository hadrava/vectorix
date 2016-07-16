#ifndef VECTORIX__LEAST_SQUARES_H
#define VECTORIX__LEAST_SQUARES_H

#include <vector>
#include "config.h"
#include "logger.h"
#include "parameters.h"

namespace vectorix {

class least_squares {
public:
	virtual void add_equation(p *arr) = 0;
	virtual void evaluate() = 0;
	virtual p calc_error() const = 0;
	virtual p operator[](unsigned int i) const = 0;
protected:
	least_squares(unsigned int variable_count, parameters &params): count(variable_count), par(&params) {
		int *param_lsq_verbosity;
		par->bind_param(param_lsq_verbosity, "lsq_verbosity", (int) log_level::warning);
		log.set_verbosity((log_level) *param_lsq_verbosity);
	};
	unsigned int count;
	logger log;
	parameters *par;
};

}; // namespace
#endif
