#include "least_squares.h"
#include <cmath>
#include <iostream>

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
	std::vector<std::vector<p>> ans;
	//std::cout << "mat a: " << a.size() << "x" << a[0].size() << std::endl;
	//std::cout << "mat b: " << b.size() << "x" << b[0].size() << std::endl;
	unsigned int rows = a.size();
	unsigned int cols = b[0].size();
	unsigned int q = b.size();

	for (int i = 0; i < rows; i++) {
		std::vector<p> row;
		for (int j = 0; j < cols; j++) {
			p val = 0;
			for (int k = 0; k < q; k++) {
				val += a[i][k] * b[k][j];
			}
			row.push_back(val);
		}
		ans.push_back(row);
	}

	return ans;
}

std::vector<std::vector<p>> least_squares::inverse(std::vector<std::vector<p>> mat) {
	unsigned int dim = mat.size();

	std::vector<std::vector<p>> ans;
	for (int i = 0; i < dim; i++) {
		std::vector<p> row;
		for (int j = 0; j < dim; j++)
			row.push_back(i == j ? 1.0 : 0.0);
		ans.push_back(row);
	}

	for (int i = 0; i < dim; i++) {
		int j = i;
		p maxval = mat[j][i];
		int k = j;
		for (; j < dim; j++) {
			if (maxval < mat[j][i]) {
				maxval = mat[j][i];
				k = j;
			}
		}
		if (k != i) {
			std::swap(mat[i], mat[k]);
			std::swap(ans[i], ans[k]);
		}
		for (int l = 0; l < dim; l++) {
			mat[i][l] /= maxval;
			ans[i][l] /= maxval;
		}

		for (j = i + 1; j < dim; j++) {
			p coef = mat[j][i];
			for (int l = 0; l < dim; l++) {
				mat[j][l] -= coef * mat[i][l];
				ans[j][l] -= coef * ans[i][l];
			}
		}
	}

	for (int i = dim - 1; i >= 0; i--) {
		for (int j = i - 1; j >= 0; j--) {
			p coef = mat[j][i];
			for (int l = 0; l < dim; l++) {
				mat[j][l] -= coef * mat[i][l];
				ans[j][l] -= coef * ans[i][l];
			}
		}
	}
	//std::cout << "mat: " << mat.size() << "x" << mat[0].size() << std::endl;
	return ans;
}

void least_squares::evaluate() {
	auto At = transpose(A);

	auto m = multiply(At, A);
	// test:
	//std::cout << "matm0: " << m[0][0] << "\t" << m[0][1] << std::endl;
	//std::cout << "matm1: " << m[1][0] << "\t" << m[1][1] << std::endl;


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
