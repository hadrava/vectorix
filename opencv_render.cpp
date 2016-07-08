#include <opencv2/opencv.hpp>
#include "v_image.h"
#include "geom.h"
#include "opencv_render.h"
#include "parameters.h"
#include <cmath>

// Render vector image to OpenCV matrix

using namespace cv;

namespace vectorix {

void opencv_render(const v_image &vector, Mat &output, const params &parameters) { // Render whole vector image
	output = Scalar(255, 255, 255); // Fill with white color
	Mat lout(output.rows, output.cols, CV_8UC(3)); // Temporary
	lout = Scalar(255, 255, 255);
	for (v_line line: vector.line) { // For every line in image...
		v_line l = line; // Copy line
		geom::chop_line(l, parameters.opencv_render.render_max_distance); // Chop line, so there is small length of each segment

		if (l.get_type() == v_line_type::stroke) { // Normal lines
			auto two = l.segment.cbegin();
			auto one = two;
			if (two != l.segment.cend())
				two++;
			while (two != l.segment.cend()) { // For each segment...
				// Segment is short enought, so we can ignore control points
				Point a,b;
				a.x = one->main.x;
				a.y = one->main.y;
				b.x = two->main.x;
				b.y = two->main.y;
				Scalar c(0,0,0);
				c.val[0] = (one->color.val[2] + two->color.val[2])/2; // Average colors, notice conversion from RGB to OpenCV's BGR
				c.val[1] = (one->color.val[1] + two->color.val[1])/2;
				c.val[2] = (one->color.val[0] + two->color.val[0])/2;
				int w = (one->width + two->width)/2; // average width
				cv::line(lout, a, b, c, w); // Draw line to temporary image

				one=two;
				two++;
			}
		}
		else {
			int count = l.segment.size();
			Point * pts = new Point[count]; // Create array with CV::Point
			int i = 0;
			Scalar c(0,0,0);
			for (v_point pt: l.segment) { // Average color
				pts[i].x = pt.main.x;
				pts[i].y = pt.main.y;
				c.val[0] += pt.color.val[2];
				c.val[1] += pt.color.val[1];
				c.val[2] += pt.color.val[0];
				i++;
			}
			c.val[0] /= i;
			c.val[1] /= i;
			c.val[2] /= i;
			const Point* fillpoints[1] = { &pts[0] };
			fillPoly(lout, fillpoints, &count, 1, c); // Draw just 1 polygon
			delete []pts;
		}

		if ((l.get_group() == v_line_group::group_normal) || (l.get_group() == v_line_group::group_last)) { // Copy image to output
			for (int i = 0; i < output.rows; i++) {
				for (int j = 0; j < output.cols; j++) {
					if (lout.at<Vec3b>(i,j) != Vec3b(255,255,255)) { // lout is not empty at this pixel
						output.at<Vec3b>(i,j) = lout.at<Vec3b>(i,j);
					}
				}
			}
			lout = Scalar(255, 255, 255); // Clean temporary image for next line
		}
	}
}

}; // namespace
