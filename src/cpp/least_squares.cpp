#include "least_squares.h"
#include <cmath>

namespace vect {

void least_squares::add_equation(p *arr) {
	std::vector<p> eq;
	std::copy(arr, arr+count, std::back_inserter(eq));
	A.push_back(eq);
	y_vector.push_back(arr[count]);
}

std::vector<std::vector<p>> least_squares::transpose(const std::vector<std::vector<p>> mat) {
	std::vector<std::vector<p>> ans;
	unsigned int cols = mat.size();

	if (cols) {
		unsigned int rows = mat[0].size();
		for (int i = 0; i < rows; i++) {
			std::vector<p> row;
			for (int j = 0; j < cols; j++) {
				row.push_back(mat[j][i]);
			}
			ans.push_back(row);
		}
	}

	return ans;
};

std::vector<std::vector<p>> least_squares::multiply(const std::vector<std::vector<p>> a, const std::vector<std::vector<p>> b) {
	// TODO
	return a;
}

std::vector<std::vector<p>> least_squares::inverse(const std::vector<std::vector<p>> mat) {
	// TODO
	return mat;
}

void least_squares::evaluate() {
	auto At = transpose(A);

	auto m = multiply(At, A);
	m = inverse(m);

	std::vector<std::vector<p>> y_mat;
	y_mat.push_back(y_vector);
	y_mat = transpose(y_mat);

	y_mat = multiply(At, y_mat);

	y_mat = multiply(m, y_mat);
	y_mat = transpose(y_mat);
	x_vector = y_mat[0];
}

p least_squares::calc_error() const {
	std::vector<std::vector<p>> x_mat;
	x_mat.push_back(x_vector);
	x_mat = transpose(x_mat);

	x_mat = multiply(A, x_mat);

	unsigned int rows = x_mat.size();
	p error = 0;
	for (int j = 0; j < rows; j++) {
		p err = x_mat[j][0] - y_vector[j];
		error += err * err;
	}

	return error;
}

p least_squares::operator[](unsigned int i) const{
	return x_vector[i];
}

}; // namespace
