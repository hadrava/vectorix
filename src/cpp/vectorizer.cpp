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
using namespace vect;
using namespace pnm;

uchar nullpixel;

uchar &safeat(const Mat &image, int i, int j) {
	if (i>=0 && i<image.rows && j>=0 && j<image.step)
		return image.data[i*image.step + j];
	else {
		nullpixel = 0;
		return nullpixel;
	}
}

void vectorizer_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	return;
}

v_image vectorize_bare(const pnm_image &image) {
	auto out = v_image(image.width, image.height);
	auto line = v_line(0, 0, image.width/2, image.height/2, image.width, image.height/2, image.width, image.height);
	line.add_point(v_pt(image.width, image.height/2*3), v_pt(-image.width/2, -image.height/2), v_pt(0,0), v_co(255, 0, 0), 20);
	out.add_line(line);
	return out;
}

void pridej(Mat &out, Mat &bw, int iterace) {
	for (int i = 0; i < out.rows; i++) {
		for (int j = 0; j < out.cols; j++) {
			if (!out.data[i*out.step + j]) {
				out.data[i*out.step + j] = (!!bw.data[i*bw.step + j]) * iterace;
			}
		}
	}
}

void normalize(Mat &out, int max) {
	for (int i = 0; i < out.rows; i++) {
		for (int j = 0; j < out.cols; j++) {
			out.data[i*out.step + j] *= 255/max;
		}
	}
}

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
			if (max < (unsigned char)safeat(out, i, j)) {
				max = (unsigned char)safeat(out, i, j);
				max_x = j;
				max_y = i;
				printf("find_adj: %i %i %i\n", i, j, safeat(out, i, j));
			}
		}
	}
	Point ret;
	ret.x = max_x;
	ret.y = max_y;
	return ret;
}

v_line *trace(Mat &out, const Mat &orig, Mat &seg, Point pos) {
	v_line *line = new v_line;
	line->add_point(v_pt(pos.x, pos.y));
	int first = 1;
	while (pos.x >= 0 && safeat(out, pos.y, pos.x)) {
		p width = safeat(out, pos.y, pos.x) * 2;
		safeat(out, pos.y, pos.x) = 0;
		safeat(seg, pos.y, pos.x*3 + 0) = safeat(orig, pos.y, pos.x*3 + 0);
		safeat(seg, pos.y, pos.x*3 + 1) = safeat(orig, pos.y, pos.x*3 + 1);
		safeat(seg, pos.y, pos.x*3 + 2) = safeat(orig, pos.y, pos.x*3 + 2);
		pos = find_adj(out, pos);
		if (pos.x >= 0) {
			line->add_point(v_pt(pos.x, pos.y), v_co(safeat(orig, pos.y, pos.x*3 + 2), safeat(orig, pos.y, pos.x*3 + 1), safeat(orig, pos.y, pos.x*3 + 0)), width);
		}
		printf("trace: %i %i %i\n", pos.x, pos.y, out.data[pos.y*out.step + pos.x]);
	}
	return line;
}

void my_segment(Mat &out, const Mat &orig, Mat &seg, v_image &vect) {
	double max;
	Point max_pos;
	minMaxLoc(out, NULL, &max, NULL, &max_pos);

	int count = 0;
	while (max !=0) {
		printf("my_segment: %i %i\n", max_pos.x, max_pos.y);
		v_line *last = trace(out, orig, seg, max_pos);
		if (last) {
			vect.add_line(*last);
			delete last;
			count ++;
		}
		minMaxLoc(out, NULL, &max, NULL, &max_pos);
	}
	printf("lines found: %i\n", count);
}

v_image vectorize(const pnm_image &original) {
	pnm_image image = original;
	image.convert(PNM_BINARY_PGM);

	v_image vect = v_image(image.width, image.height);

	Mat source (image.height, image.width, CV_8UC(1));
	Mat orig   (image.height, image.width, CV_8UC(3));
	Mat bw     (image.height, image.width, CV_8UC(1));
	Mat out = Mat::zeros(image.height, image.width, CV_8UC(1));
	Mat seg    (image.height, image.width, CV_8UC(3));
	for (int j = 0; j < image.height; j++) {
		for (int i = 0; i<image.width; i++) {
			source.data[i+j*source.step] = 255 - image.data[i+j*image.width];
			orig.data[i*3+j*orig.step+2] = original.data[(i+j*original.width)*3 + 0];
			orig.data[i*3+j*orig.step+1] = original.data[(i+j*original.width)*3 + 1];
			orig.data[i*3+j*orig.step+0] = original.data[(i+j*original.width)*3 + 2];
			seg.data[i*3+j*orig.step+2] = 255;
			seg.data[i*3+j*orig.step+1] = 255;
			seg.data[i*3+j*orig.step+0] = 255;
		}
	}
	imshow("vectorizer", orig);
	waitKey(0);
	imshow("vectorizer", source);
	waitKey(0);
	threshold(source, source, 127, 255, THRESH_BINARY | THRESH_OTSU);
	Mat kernel = getStructuringElement(MORPH_CROSS, Size(3,3));

	double max = 1;
	int iterace = 1;
	while (max !=0) {
		morphologyEx(source, bw, MORPH_OPEN, kernel);	//dilate(erode(source))
		bitwise_not(bw, bw);
		bitwise_and(source, bw, bw);
		//bitwise_or(out, bw, out);
		pridej(out, bw, iterace++);
		erode(source, source, kernel);
		minMaxLoc(source, NULL, &max, NULL, NULL);
	}

	bw = out.clone();
	normalize(bw, iterace-1);
	imshow("vectorizer", bw);
	waitKey(0);
	//cvDilate(out, out, kernel, 1);
	////normalize(out, iterace-1);
	////my_threshold(out);
	my_segment(out, orig, seg, vect);
	//cvShowImage("source", seg);
	//cvWaitKey(0);

	imshow("vectorizer", seg);
	waitKey(0);
	printf("end of vectorization\n");

	return vect;
}
