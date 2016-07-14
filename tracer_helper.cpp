#include <opencv2/opencv.hpp>
#include "parameters.h"
#include "logger.h"
#include "tracer_helper.h"

using namespace cv;

namespace vectorix {

/*
 * Starting points for tracer
 */

// Find all possible startingpoints
void starting_point::prepare(const Mat &skeleton) {
	for (int i = 0; i < skeleton.rows; i++) {
		for (int j = 0; j < skeleton.cols; j++) {
			if (skeleton.data[i*skeleton.step + j]) { // Pixel is in skeleton
				st_point pt;
				pt.val = skeleton.data[i*skeleton.step + j];
				pt.pt = Point(j, i);
				queue.push_back(pt); // Add it to queue (std::vector)
			}
		}
	}
	std::sort(queue.begin(), queue.end(), [&](st_point a, st_point b)->bool {
			return a.val < b.val;
			}); // Sort pixels by distance to object borders
}

// Find first unused starting point in queue
int starting_point::get_max(const Mat &used_pixels, Point &max_pos) {
	int max = 0;
	while ((max == 0) && !queue.empty()) {
		st_point pt = queue.back();
		queue.pop_back();
		if (!used_pixels.data[pt.pt.y*used_pixels.step + pt.pt.x]) { // Pixel was not used
			max = pt.val;
			max_pos = pt.pt;
		}
	}
	log.log<log_level::debug>("New start point value: %i\n", max);
	return max;
}


/*
 * Regions of interest
 */

void changed_pix_roi::clear(int x, int y) { // remove all pixels from roi
	min_x = x;
	min_y = y;
	max_x = -1;
	max_y = -1;
}

Rect changed_pix_roi::get() const{ // Return OpenCV roi
	if ((min_x <= max_x) && (min_y <= max_y))
		return Rect(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
	else
		return Rect(0, 0, 0, 0); // Empty roi
}

void changed_pix_roi::update(int x, int y) { // Add pixel (x,y) to roi
	min_x = min(min_x, x);
	min_y = min(min_y, y);
	max_x = max(max_x, x);
	max_y = max(max_y, y);
}

}; // namespace (vectorix)
