#ifndef _OPENCV_RENDER_H
#define _OPENCV_RENDER_H

// Render vector image to OpenCV matrix

#include <opencv2/opencv.hpp>
#include "v_image.h"

namespace vect {

void opencv_render(const v_image &vector, cv::Mat &output, const params &parameters);

}; // namespace

#endif
