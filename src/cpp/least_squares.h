#ifndef _LEAST_SQUARE_H
#define _LEAST_SQUARE_H

#include <vector>
#include "config.h"

namespace vect {

class least_squares {
	public:
		least_squares(unsigned int variable_count): count(variable_count) {};
		void add_equation(p *arr);
		void evaluate();
		p calc_error() const;
		p operator[](unsigned int i) const;
	private:
		unsigned int count;
		std::vector<std::vector<p>> A;
		std::vector<p> x_vector;
		std::vector<p> y_vector;
		static std::vector<std::vector<p>> transpose(const std::vector<std::vector<p>> &mat);
		static std::vector<std::vector<p>> multiply(const std::vector<std::vector<p>> &a, const std::vector<std::vector<p>> &b);
		static std::vector<std::vector<p>> inverse(std::vector<std::vector<p>> mat);
};

}; // namespace
#endif
