#include "least_squares.h"
#include <cmath>
#include <iostream>

namespace vectorix {

void least_squares::add_equation(p *arr) {
	vector_p eq;
	std::copy(arr, arr+count, std::back_inserter(eq));
	A.add_row(eq);
	y_vector.push_back(arr[count]);
}

void least_squares::matrix_p::transpose() {
	matrix_p ans;
	unsigned int ans_cols = rows();

	if (ans_cols) {
		unsigned int ans_rows = (*this)[0].size();
		for (int i = 0; i < ans_rows; i++) {
			vector_p row;
			for (int j = 0; j < ans_cols; j++) {
				row.push_back((*this)[j][i]);
			}
			ans.add_row(row);
		}
	}

	std::swap(ans, *this);
};

least_squares::matrix_p least_squares::matrix_p::operator*(const matrix_p &b) const {
	matrix_p ans;
	//std::cout << "mat a: " << a.size() << "x" << a[0].size() << std::endl;
	//std::cout << "mat b: " << b.size() << "x" << b[0].size() << std::endl;
	unsigned int ans_rows = rows();
	unsigned int ans_cols = b[0].size();
	unsigned int q = b.rows();

	for (int i = 0; i < ans_rows; i++) {
		vector_p row;
		for (int j = 0; j < ans_cols; j++) {
			p val = 0;
			for (int k = 0; k < q; k++) {
				val += (*this)[i][k] * b[k][j];
			}
			row.push_back(val);
		}
		ans.add_row(row);
	}
	return ans;
}

void least_squares::matrix_p::inverse() {
	unsigned int dim = rows();

	matrix_p ans;
	for (int i = 0; i < dim; i++) {
		vector_p row;
		for (int j = 0; j < dim; j++)
			row.push_back(i == j ? 1.0 : 0.0);
		ans.add_row(row);
	}

	for (int i = 0; i < dim; i++) {
		int j = i;
		p maxval = (*this)[j][i];
		int k = j;
		for (; j < dim; j++) {
			if (maxval < (*this)[j][i]) {
				maxval = (*this)[j][i];
				k = j;
			}
		}
		if (k != i) {
			std::swap((*this)[i], (*this)[k]);
			std::swap(ans[i], ans[k]);
		}
		for (int l = 0; l < dim; l++) {
			(*this)[i][l] /= maxval;
			ans[i][l] /= maxval;
		}

		for (j = i + 1; j < dim; j++) {
			p coef = (*this)[j][i];
			for (int l = 0; l < dim; l++) {
				(*this)[j][l] -= coef * (*this)[i][l];
				ans[j][l] -= coef * ans[i][l];
			}
		}
	}

	for (int i = dim - 1; i >= 0; i--) {
		for (int j = i - 1; j >= 0; j--) {
			p coef = (*this)[j][i];
			for (int l = 0; l < dim; l++) {
				(*this)[j][l] -= coef * (*this)[i][l];
				ans[j][l] -= coef * ans[i][l];
			}
		}
	}
	//std::cout << "mat: " << size() << "x" << (*this)[0].size() << std::endl;
	std::swap(ans, *this);
}

void least_squares::evaluate() {
	auto At = A;
	At.transpose();

	auto m = At * A;
	// test:
	//std::cout << "matm0: " << m[0][0] << "\t" << m[0][1] << std::endl;
	//std::cout << "matm1: " << m[1][0] << "\t" << m[1][1] << std::endl;


	m.inverse();

	matrix_p y_mat;
	y_mat.add_row(y_vector);
	y_mat.transpose();

	y_mat = At * y_mat;

	y_mat = m * y_mat;
	y_mat.transpose();
	x_vector = y_mat[0];
	log.log<log_level::debug>("least_squares evaluated\n");
}

p least_squares::calc_error() const {
	matrix_p x_mat;
	x_mat.add_row(x_vector);
	x_mat.transpose();

	x_mat = A * x_mat;

	unsigned int rows = x_mat.rows();
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
