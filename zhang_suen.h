#ifndef VECTORIX__ZHANG_SUEN_H
#define VECTORIX__ZHANG_SUEN_H

#include <opencv2/opencv.hpp>
#include <vector>

namespace vectorix {

class zhang_suen {
public:
	int skeletonize(const cv::Mat &inupt, cv::Mat &skeleton, cv::Mat &distance);
private:
	cv::Mat *itp;
	cv::Mat inq;
	cv::Mat *skel;
	std::vector<cv::Point> border_queue;
	std::vector<cv::Point> delete_queue;

	int B(cv::Point pt) const;
	int A(cv::Point pt) const;
	void init_queue();
};

}; // namespace

#endif
