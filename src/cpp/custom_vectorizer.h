#ifndef _CUSTOM_VECTORIZER_H
#define _CUSTOM_VECTORIZER_H

#include <opencv2/opencv.hpp>
#include "pnm_handler.h"
#include "v_image.h"
#include "vectorizer.h"
#include "parameters.h"
#include <string>

namespace vect {

typedef struct {
	int min_x;
	int min_y;
	int max_x;
	int max_y;
} changed_pix_roi;

typedef struct {
	cv::Point pt;
	int val;
} start_point;

class custom : generic_vectorizer {
public:
	static vect::v_image vectorize(const pnm::pnm_image &image);
private:
	static uchar nullpixel;
public:
	static uchar &safeat(const cv::Mat &image, int i, int j);
private:
	static const int step = 2;
	static void vectorize_imshow(const std::string& winname, cv::InputArray mat);
	static int vectorize_waitKey(int delay = 0);
	static void vectorize_destroyWindow(const std::string& winname);
	static void add_to_skeleton(cv::Mat &out, cv::Mat &bw, int iteration);
	static void normalize(cv::Mat &out, int max);
public:
	static cv::Point find_adj(const cv::Mat &out, cv::Point pos);
private:
	static void step1_threshold(cv::Mat &to_threshold, step1_params &par);
	static void step2_skeletonization(const cv::Mat &binary_input, cv::Mat &skeleton, cv::Mat &distance, int &iteration, step2_params &par);
	static void step3_tracing(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, v_image &vectorization_output, step3_params &par);
	static void trace_part(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, cv::Point startpoint, v_line &line, step3_params &par);
};


int pix(const cv::Mat &img, const v_pt &point);
void spix(const cv::Mat &img, const v_pt &point, int value);
void spix(const cv::Mat &img, const cv::Point &point, int value);
int inc_pix_to(const cv::Mat &mask, int value, const v_pt &point, cv::Mat &used_pixels);

}; // namespace

#endif
