#include <opencv2/opencv.hpp>
#include "v_image.h"
#include "geom.h"
#include "opencv_render.h"
#include "parameters.h"
#include <cmath>

// Render vector image to OpenCV matrix

using namespace cv;

namespace vectorix {

void opencv_render(const v_image &vector, Mat &output, parameters &params) { // Render whole vector image
	p *param_render_max_distance;
	params.add_comment("Phase 5: Export");
	params.add_comment("Maximal allowed error in OpenCV rendering in pixels");
	params.bind_param(param_render_max_distance, "render_max_distance", (p) 1);

	output = Scalar(255, 255, 255); // Fill with white color
	for (v_line line: vector.line) { // For every line in image...
		v_line l = line; // Copy line
		geom::chop_line(l, *param_render_max_distance); // Chop line, so there is small length of each segment

		if (l.get_type() == v_line_type::stroke) { // Normal lines
			auto two = l.segment.cbegin();
			auto one = two;
			if (two != l.segment.cend())
				two++;
			if (two == l.segment.cend()) // Just one point, draw circle
				two--;
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
				cv::line(output, a, b, c, w); // Draw line to temporary image

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
			fillPoly(output, fillpoints, &count, 1, c); // Draw just 1 polygon
			delete []pts;
		}
	}
}

}; // namespace
