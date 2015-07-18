#include <opencv2/opencv.hpp>
#include "v_image.h"
#include "opencv_render.h"
#include <string>

using namespace cv;

namespace vect {

p calc_distance_squared(const v_point &a, const v_point &b) {//TODO look at control points
	p x = (a.main.x - b.main.x);
	p y = (a.main.y - b.main.y);
	return x*x + y*y;
}

void prepare_for_render(v_line &line) {
	auto two = line.segment.begin();
	auto one = two;
	if (two != line.segment.end())
		++two;
	while (two != line.segment.end()) {
		while (calc_distance_squared(*one, *two) > 2) {
			v_point newpoint;
			newpoint.control_prev.x = ((one->control_next.x + one->main.x)/2 + (one->control_next.x + two->control_prev.x)/2)/2;
			newpoint.control_prev.y = ((one->control_next.y + one->main.y)/2 + (one->control_next.y + two->control_prev.y)/2)/2;

			newpoint.control_next.x = ((two->control_prev.x + two->main.x)/2 + (one->control_next.x + two->control_prev.x)/2)/2;
			newpoint.control_next.y = ((two->control_prev.y + two->main.y)/2 + (one->control_next.y + two->control_prev.y)/2)/2;

			newpoint.main.x = (newpoint.control_prev.x  + newpoint.control_next.x)/2;
			newpoint.main.y = (newpoint.control_prev.y  + newpoint.control_next.y)/2;

			newpoint.opacity = (one->opacity + two->opacity)/2;
			newpoint.width = (one->width + two->width)/2;
			newpoint.color = one->color;
			newpoint.color += two->color;
			newpoint.color /= 2;
			one->control_next.x = (one->control_next.x + one->main.x)/2;
			one->control_next.y = (one->control_next.y + one->main.y)/2;
			two->control_prev.x = (two->control_prev.x + two->main.x)/2;
			two->control_prev.y = (two->control_prev.y + two->main.y)/2;

			line.segment.insert(two, newpoint);
			--two;
		}
		one=two;
		two++;
	}

}

void opencv_render(const v_image &vector, Mat &output) {
	output = Scalar(255, 255, 255);
	Mat lout(output.rows, output.cols, CV_8UC(3));
	lout = Scalar(255, 255, 255);
	for (v_line line: vector.line) {
		v_line l = line;
		prepare_for_render(l);
		
		if (l.get_type() == stroke) {
			auto two = l.segment.cbegin();
			auto one = two;
			if (two != l.segment.cend())
				two++;
			while (two != l.segment.cend()) {
				Point a,b;
				a.x = one->main.x;
				a.y = one->main.y;
				b.x = two->main.x;
				b.y = two->main.y;
				Scalar c(0,0,0);
				c.val[0] = (one->color.val[2] + two->color.val[2])/2;
				c.val[1] = (one->color.val[1] + two->color.val[1])/2;
				c.val[2] = (one->color.val[0] + two->color.val[0])/2;
				int w = (one->width + two->width)/2;
				cv::line(lout, a, b, c, w);

				one=two;
				two++;
			}
		}
		else {
			int count = l.segment.size();
			Point * pts = new Point[count];
			int i = 0;
			Scalar c(0,0,0);
			for (v_point pt: l.segment) {
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
			fillPoly(lout, fillpoints, &count, 1, c);
			delete []pts;
		}

		if ((l.get_group() == group_normal) || (l.get_group() == group_last)) {
			for (int i = 0; i < output.rows; i++) {
				for (int j = 0; j < output.cols; j++) {
					if (lout.at<Vec3b>(i,j) != Vec3b(255,255,255)) {
						output.at<Vec3b>(i,j) = lout.at<Vec3b>(i,j);
					}
				}
			}
			lout = Scalar(255, 255, 255);
		}
	}
}

}; // namespace
