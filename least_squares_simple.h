#ifndef VECTORIX__LEAST_SQUARES_SIMPLE_H
#define VECTORIX__LEAST_SQUARES_SIMPLE_H

#include <vector>
#include "config.h"
#include "logger.h"
#include "parameters.h"
#include "least_squares.h"

namespace vectorix {

class least_squares_simple: public least_squares {
public:
	least_squares_simple(unsigned int variable_count, parameters &params): least_squares(variable_count, params) {};
	virtual void add_equation(p *arr);
	virtual void evaluate();
	virtual p calc_error() const;
	virtual p operator[](unsigned int i) const;
private:
	typedef std::vector<p> vector_p;
	class matrix_p {
	public:
		void add_row(vector_p v);
		unsigned int rows() const {
			return data.size();
		};
		const vector_p operator[] (unsigned int i) const {
			return data[i];
		}
		vector_p &operator[] (unsigned int i) {
			return data[i];
		}
		void transpose();
		void inverse();
		matrix_p operator*(const matrix_p &a) const;
	private:
		std::vector<vector_p> data;
	};

	matrix_p A;
	vector_p x_vector;
	vector_p y_vector;
};

}; // namespace
#endif
