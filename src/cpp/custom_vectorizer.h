#ifndef _CUSTOM_VECTORIZER_H
#define _CUSTOM_VECTORIZER_H

#include <opencv2/opencv.hpp>
#include "pnm_handler.h"
#include "v_image.h"
#include "vectorizer.h"

namespace vect {

class custom : generic_vectorizer {
public:
	static vect::v_image vectorize(const pnm::pnm_image &image);
private:
	static uchar nullpixel;
	static uchar &safeat(const cv::Mat &image, int i, int j);
	static const int step = 2;
	static void vectorize_imshow(const cv::string& winname, cv::InputArray mat);
	static int vectorize_waitKey(int delay = 0);
	static void pridej(cv::Mat &out, cv::Mat &bw, int iterace);
	static void normalize(cv::Mat &out, int max);
	static void my_threshold(cv::Mat &out);
	static cv::Point find_adj(const cv::Mat &out, cv::Point pos);
	static vect::v_line *trace(cv::Mat &out, const cv::Mat &orig, cv::Mat &seg, cv::Point pos);
	static void my_segment(cv::Mat &out, const cv::Mat &orig, cv::Mat &seg, v_image &vect);
};

}; // namespace

#endif
