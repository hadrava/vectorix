#ifndef VECTORIX__OPENCV_RENDER_H
#define VECTORIX__OPENCV_RENDER_H

// Render vector image to OpenCV matrix

#include <opencv2/opencv.hpp>
#include "v_image.h"

namespace vectorix {

void opencv_render(const v_image &vector, cv::Mat &output, parameters &params);

}; // namespace

#endif
