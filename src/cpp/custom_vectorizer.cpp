#include <opencv2/opencv.hpp>
#include "v_image.h"
#include "pnm_handler.h"
#include "vectorizer.h"
#include "custom_vectorizer.h"

using namespace cv;
using namespace pnm;

namespace vect {

uchar custom::nullpixel; // Virtual pixel for safe access to image matrix.

uchar &custom::safeat(const Mat &image, int i, int j) { // Safely acces image data.
	if (i>=0 && i<image.rows && j>=0 && j<image.step)
		return image.data[i*image.step + j];
	else {
		nullpixel = 0;
		return nullpixel;
	}
}

#ifdef VECTORIZER_HIGHGUI
void custom::vectorize_imshow(const string& winname, InputArray mat) { // Display image in highgui named window.
	return imshow(winname, mat);
}
int custom::vectorize_waitKey(int delay) { // Wait for key press in any window.
	return waitKey(delay);
}
#else
void custom::vectorize_imshow(const string& winname, InputArray mat) { // Highgui disabled, do nothing.
	return;
}
int custom::vectorize_waitKey(int delay) { // Highgui disabled, return `no key pressed'.
	return -1;
}
#endif

void custom::add_to_skeleton(Mat &out, Mat &bw, int iteration) { // Add pixels from `bw' to `out'. Something like image `or', but with more information.
	for (int i = 0; i < out.rows; i++) {
		for (int j = 0; j < out.cols; j++) {
			if (!out.data[i*out.step + j]) {
				out.data[i*out.step + j] = (!!bw.data[i*bw.step + j]) * iteration;
			}
		}
	}
}

void custom::normalize(Mat &out, int max) { // Normalize grayscale image for displaying (0-255).
	for (int i = 0; i < out.rows; i++) {
		for (int j = 0; j < out.cols; j++) {
			out.data[i*out.step + j] *= 255/max;
		}
	}
}

void custom::my_threshold(Mat &out) { // Simple threshold function.
	for (int i = 0; i < out.rows; i++) {
		for (int j = 0; j < out.cols; j++) {
			out.data[i*out.step + j] = (!!out.data[i*out.step + j])*255;
		}
	}
}

Point custom::find_adj(const Mat &out, Point pos) { // Find adjacent point for tracing in square neighbourhood defined by step constant.
	int max = 0;
	int max_x = -1;
	int max_y = -1;
	for (int y = -step; y<=step; y++) {
		for (int x = -step; x<=step; x++) {
			int i = pos.y + y;
			int j = pos.x + x;
			if (max < (unsigned char)safeat(out, i, j)) { // Point should be in center of traced line.
				max = (unsigned char)safeat(out, i, j);
				max_x = j;
				max_y = i;
				vectorizer_debug("find_adj: %i %i %i\n", i, j, safeat(out, i, j));
			}
		}
	}
	Point ret;
	ret.x = max_x;
	ret.y = max_y;
	return ret;
}

v_line *custom::trace(Mat &out, const Mat &orig, Mat &seg, Point pos) { // Trace line in bitmap `out' from point `pos'. Orig is used for colors only. Seg is auxiliary output for debuging.
	v_line *line = new v_line;
	line->add_point(v_pt(pos.x, pos.y));
	int first = 1;
	while (pos.x >= 0 && safeat(out, pos.y, pos.x)) {
		p width = safeat(out, pos.y, pos.x) * 2; // Compute `width' from distance of a skeleton pixel to boundary in the original image.
		safeat(out, pos.y, pos.x) = 0; // Delete pixel from source to prevent cycling.
		safeat(seg, pos.y, pos.x*3 + 0) = safeat(orig, pos.y, pos.x*3 + 0); // Copy pixel to output.
		safeat(seg, pos.y, pos.x*3 + 1) = safeat(orig, pos.y, pos.x*3 + 1);
		safeat(seg, pos.y, pos.x*3 + 2) = safeat(orig, pos.y, pos.x*3 + 2);
		pos = find_adj(out, pos);
		if (pos.x >= 0) {
			line->add_point(v_pt(pos.x, pos.y), v_co(safeat(orig, pos.y, pos.x*3 + 2), safeat(orig, pos.y, pos.x*3 + 1), safeat(orig, pos.y, pos.x*3 + 0)), width); // Add pixel to output line with color from `orig'.
		}
		vectorizer_debug("trace: %i %i %i\n", pos.x, pos.y, out.data[pos.y*out.step + pos.x]);
	}
	return line;
}

void custom::my_segment(Mat &out, const Mat &orig, Mat &seg, v_image &vect) { // Trace whole image `out', write output to `vect'.
	double max;
	Point max_pos;
	minMaxLoc(out, NULL, &max, NULL, &max_pos);

	int count = 0;
	while (max !=0) { // While we have unused pixel.
		vectorizer_debug("my_segment: %i %i\n", max_pos.x, max_pos.y);
		v_line *last = trace(out, orig, seg, max_pos); // Trace whole line.
		if (last) {
			vect.add_line(*last);
			delete last;
			count ++;
		}
		minMaxLoc(out, NULL, &max, NULL, &max_pos);
	}
	vectorizer_debug("lines found: %i\n", count);
}

v_image custom::vectorize(const pnm_image &original) { // Original should be PPM image (color).
	pnm_image image = original;
	image.convert(PNM_BINARY_PGM); // Convert to grayscale for thresholding and skeletonization.

	v_image vect = v_image(image.width, image.height);

	Mat source (image.height, image.width, CV_8UC(1));
	Mat orig   (image.height, image.width, CV_8UC(3));
	Mat bw     (image.height, image.width, CV_8UC(1));
	Mat out = Mat::zeros(image.height, image.width, CV_8UC(1));
	Mat seg    (image.height, image.width, CV_8UC(3));
	for (int j = 0; j < image.height; j++) { // Copy data from PNM images to OpenCV image structures.
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
	vectorize_imshow("vectorizer", orig); // Show original color image.
	vectorize_waitKey(0);
	vectorize_imshow("vectorizer", source); // Show grayscale input image.
	vectorize_waitKey(0);
	threshold(source, source, 127, 255, THRESH_BINARY | THRESH_OTSU); // Apply thresholdnig with threshold found by Otsu's algorithm.
	Mat kernel = getStructuringElement(MORPH_CROSS, Size(3,3)); // Kernel for morphological operations -- skeletonization.

	double max = 1;
	int iteration = 1;
	while (max !=0) { // CAlculate image skeleton (with boundary peeling
		morphologyEx(source, bw, MORPH_OPEN, kernel); // Morphological open = dilate(erode(source)).
		bitwise_not(bw, bw);
		bitwise_and(source, bw, bw);
		add_to_skeleton(out, bw, iteration++); // Almost same as bitwise_or(out, bw, out). Save distance from object boudary (interation number).
		erode(source, source, kernel);
		minMaxLoc(source, NULL, &max, NULL, NULL);
	}

	bw = out.clone();
	normalize(bw, iteration-1); // Normalize skeleton image for displaying on screen.
	vectorize_imshow("vectorizer", bw);
	vectorize_waitKey(0);
	//dilate(out, out, kernel, 1);
	////normalize(out, iteration-1);
	////my_threshold(out);
	my_segment(out, orig, seg, vect); // Trace skeleton.
	//vectorize_imshow("vectorizer", seg);
	//vectorize_waitKey(0);

	vectorize_imshow("vectorizer", seg);
	vectorize_waitKey(0);
	vectorizer_debug("end of vectorization\n");

	return vect;
}

}; // namespace
