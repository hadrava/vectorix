#include "lines.h"
#include "pnm_handler.h"
#include "vectorizer.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
//#include <cv.h>
//#include <highgui.h>
#include <opencv2/opencv.hpp>

using namespace cv;

void vectorizer_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	return;
}

class v_image vectorize_bare(const class pnm_image &image) {
	auto out = v_image(image.width, image.height);
	auto line = v_line(0, 0, image.width/2, image.height/2, image.width, image.height/2, image.width, image.height);
	line.add_point(v_pt(image.width, image.height/2*3), v_pt(-image.width/2, -image.height/2), v_pt(0,0));
	out.add_line(line);
	return out;
}

/*
void pridej(IplImage *out, IplImage *bw, int iterace) {
	for (int i = 0; i < out->height; i++) {
		for (int j = 0; j < out->width; j++) {
			if (!out->data[i*out->widthStep + j]) {
				out->data[i*out->widthStep + j] = (!!bw->data[i*bw->widthStep + j]) * iterace;
			}
		}
	}
}

void normalize(IplImage *out, int max) {
	for (int i = 0; i < out->height; i++) {
		for (int j = 0; j < out->width; j++) {
			out->data[i*out->widthStep + j] *= 255/max;
		}
	}
}
*/

void my_threshold(Mat &out) {
	for (int i = 0; i < out.rows; i++) {
		for (int j = 0; j < out.cols; j++) {
			out.data[i*out.step + j] = (!!out.data[i*out.step + j])*255;
		}
	}
}

const int step = 2;

Point find_adj(const Mat &out, Point pos) {
	int max = 0;
	int max_x = -1;
	int max_y = -1;
	for (int y = -step; y<=step; y++) {
		for (int x = -step; x<=step; x++) {
			int i = pos.y + y;
			int j = pos.x + x;
			if (max < (unsigned char)out.data[i*out.step + j]) {
				max = (unsigned char)out.data[i*out.step + j];
				max_x = j;
				max_y = i;
				printf("%i %i %i\n", i, j, out.data[i*out.step + j]);
			}
		}
	}
	Point ret;
	ret.x = max_x;
	ret.y = max_y;
	return ret;
}

v_line *trace(Mat &out, Mat &seg, Point pos, Scalar col) {
	v_line * line = new v_line;
	line->add_point(v_pt(pos.x, pos.y));
	int first = 1;
	while (pos.x >= 0 && out.data[pos.y*out.step + pos.x]) {
		out.data[pos.y*out.step + pos.x] = 0;
		seg.data[pos.y*seg.step + pos.x*3 + 0] = col.val[0];
		seg.data[pos.y*seg.step + pos.x*3 + 1] = col.val[1];
		seg.data[pos.y*seg.step + pos.x*3 + 2] = col.val[2];
		pos = find_adj(out, pos);
		if (pos.x >= 0) {
			line->add_point(v_pt(pos.x, pos.y));
		}
		//printf("%i %i %i\n", pos.x, pos.y, out.data[pos.y*out.step + pos.x]);
	}
	return line;
}

void my_segment(Mat &out, Mat &seg, v_image &vect) {
	double max;
	Point max_pos;
	minMaxLoc(out, NULL, &max, NULL, &max_pos);

	int count = 0;
	while (max !=0) {
		v_line *last = trace(out, seg, max_pos, Scalar(255,0,0));
		if (last) {
			vect.add_line(*last);
			count ++;
		}
		minMaxLoc(out, NULL, &max, NULL, &max_pos);
	}
	printf("lines found: %i\n", count);
}

class v_image vectorize(const class pnm_image &image) {
	v_image vect = v_image(image.width, image.height);

	Mat source (image.height, image.width, CV_8UC(1));
	Mat bw     (image.height, image.width, CV_8UC(1));
	Mat out = Mat::zeros(image.height, image.width, CV_8UC(1));
	Mat seg    (image.height, image.width, CV_8UC(3));
	for (int j = 0; j < image.height; j++) {
		for (int i = 0; i<image.width; i++) {
			source.data[i+j*source.step] = image.data[i+j*image.width];
		}
	}
	imshow("source", source);
	waitKey(0);
	threshold(source, source, 127, 255, THRESH_BINARY);
	Mat kernel = getStructuringElement(MORPH_CROSS, Size(3,3));

	double max = 1;
	int iterace = 1;
	while (max !=0) {
		morphologyEx(source, bw, MORPH_OPEN, kernel);	//dilate(erode(source))
		bitwise_not(bw, bw);
		bitwise_and(source, bw, bw);
		bitwise_or(out, bw, out);
		//pridej(out, bw, iterace++);
		erode(source, source, kernel);
		minMaxLoc(source, NULL, &max, NULL, NULL);
	}

	imshow("source", out);
	waitKey(0);
	//cvDilate(out, out, kernel, 1);
	//normalize(out, iterace-1);
	my_threshold(out);
	//cvShowImage("source", out);
	//cvWaitKey(0);
	my_segment(out, seg, vect);
	//cvShowImage("source", seg);
	//cvWaitKey(0);

	imshow("source", seg);
	waitKey(0);
	printf("end of vectorization\n");

	return vect;
}

class pnm_image render(const class v_image &vector) {
	auto bitmap = pnm_image(vector.width, vector.height);
	bitmap.erase_image();
	for (v_line line: vector.line) {
		bezier_render(bitmap, line);
	}
	return bitmap;
}

void bezier_render(class pnm_image &bitmap, const class v_line &line) {
	if (bitmap.type != PNM_BINARY_PGM)
		vectorizer_error("Error: Image type %i not supported.\n", bitmap.type);
	auto two = line.segment.cbegin();
	auto one = two;
	if (two != line.segment.cend())
		two++;
	while (two != line.segment.cend()) {
		for (p u=0; u<=1; u+=0.0005) {
			p v = 1-u;
			p x = v*v*v*one->main.x + 3*v*v*u*one->control_next.x + 3*v*u*u*two->control_prev.x + u*u*u*two->main.x;
			p y = v*v*v*one->main.y + 3*v*v*u*one->control_next.y + 3*v*u*u*two->control_prev.y + u*u*u*two->main.y;
			p w = v*one->width + u*two->width;
			for (int j = y - w/2; j<= y + w/2; j++) {
				if (j<0 || j>=bitmap.height)
					continue;
				for (int i = x - w/2; i<= x + w/2; i++) {
					if (i<0 || i>=bitmap.width)
						continue;
					if ((x-i)*(x-i) + (y-j)*(y-j) <= w*w/4)
						bitmap.data[i+j*bitmap.width] = 0;
				}
			}
		}
		one=two;
		two++;
	}
}
