/*
 * Vectorix -- line-based image vectorizer
 * (c) 2016 Jan Hadrava <had@atrey.karlin.mff.cuni.cz>
 */
#ifndef VECTORIX__ZOOM_WINDOW_H
#define VECTORIX__ZOOM_WINDOW_H

#include <opencv2/opencv.hpp>
#include <string>
#include <tuple>
#include "parameters.h"

namespace vectorix {

void zoom_imshow(const std::string &winname, const cv::Mat &mat, bool overview = false);
void zoom_set_params(parameters &params);

class zoom_window {
	// OpenCV imshow + zoom/crop image
	// We use const cv::Mat instead of more generic cv::InputArray. (For compatibility with more versions of OpenCV.)
	friend void zoom_imshow(const std::string &winname, const cv::Mat &mat, bool overview);
	friend void zoom_set_params(parameters &params);
private:
	zoom_window() = default;
	static zoom_window &instance() {
		static zoom_window i;
		return i;
	};

	void imshow(const std::string &winname, const cv::Mat &mat, bool overview);
	void set_params(parameters &params) {
		par = &params;
		par->add_comment("Maximal size of window NxN: (0 = unlimited)");
		par->bind_param(param_max_window_size, "max_window_size", 640);
		par->add_comment("Scale images before viewing in window: 0: small picture, 100: original resolution");
		par->bind_param(param_zoom_level, "zoom_level", 0);
		pos_x = 0.5;
		pos_y = 0.5;
	}
	void render(std::unordered_map<std::string, std::tuple<cv::Mat, bool>>::iterator ref);
	void refresh();

	static void mouse_callback(int event, int x, int y, int, void *object);
	static void trackbar_callback(int, void *object);

	int *param_max_window_size;
	parameters *par;
	std::unordered_map<std::string, std::tuple<cv::Mat, bool>> displayed_images;

	/*
	 * Zoom state
	 */
	int *param_zoom_level;
	double pos_x, pos_y;
	cv::Size overview_size;
	cv::Size scaled_roi_size;
};

}; // namespace

#endif
