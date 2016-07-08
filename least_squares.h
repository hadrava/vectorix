#ifndef VECTORIX__LEAST_SQUARE_H
#define VECTORIX__LEAST_SQUARE_H

#include <vector>
#include "config.h"
#include "logger.h"

namespace vectorix {

class least_squares {
public:
	least_squares(unsigned int variable_count): count(variable_count), log(log_level::debug) {};
	void add_equation(p *arr);
	void evaluate();
	p calc_error() const;
	p operator[](unsigned int i) const;
private:
	typedef std::vector<p> vector_p;
	class matrix_p {
	public:
		void add_row(vector_p v) {
			// TODO assert save vector size
			data.push_back(v);
		};
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

	unsigned int count;
	matrix_p A;
	vector_p x_vector;
	vector_p y_vector;
	logger log;
};

}; // namespace
#endif
