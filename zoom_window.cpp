#include <opencv2/opencv.hpp>
#include <string>
#include <tuple>
#include "zoom_window.h"
#include "parameters.h"

namespace vectorix {

// Display (zoomed/cropped) image in highgui named window.
void zoom_imshow(const std::string &winname, const cv::Mat &mat, bool overview) {
	zoom_window::instance().imshow(winname, mat, overview);
}

void zoom_set_params(parameters &params) {
	zoom_window::instance().set_params(params);
}

void zoom_window::imshow(const std::string &winname, const cv::Mat &mat, bool overview) {
	bool first_time = false;
	auto old = displayed_images.find(winname);
	if (old == displayed_images.end()) {
		auto ins = displayed_images.insert({winname, std::make_tuple(mat, overview)});
		old = ins.first;
		first_time = true;
	}
	mat.convertTo(std::get<0>(old->second), CV_8U);

	render(old);
	if (overview && first_time) {
		// Set callbacks
		cv::setMouseCallback(old->first, mouse_callback, (void*) this);
		cv::createTrackbar("Zoom", old->first, param_zoom_level, 100, trackbar_callback, (void*) this);
	}
}

void zoom_window::mouse_callback(int event, int x, int y, int, void *object) {
	static bool pressed = false;
	if (event == cv::EVENT_LBUTTONUP) {
		pressed = false;
		return;
	}
	else if (event == cv::EVENT_LBUTTONDOWN) {
		pressed = true;
	}
	else if (pressed && (event == cv::EVENT_MOUSEMOVE)) {
	}
	else {
		return;
	}

	zoom_window *obj = (zoom_window *) object;

	obj->pos_x = (double) (x - obj->scaled_roi_size.width / 2)  / (obj->overview_size.width - obj->scaled_roi_size.width);
	obj->pos_y = (double) (y - obj->scaled_roi_size.height / 2) / (obj->overview_size.height - obj->scaled_roi_size.height);
	if (obj->pos_x < 0)
		obj->pos_x = 0;
	if (obj->pos_x > 1)
		obj->pos_x = 1;
	if (obj->pos_y < 0)
		obj->pos_y = 0;
	if (obj->pos_y > 1)
		obj->pos_y = 1;

	obj->refresh();
}

void zoom_window::trackbar_callback(int, void *object) {
	zoom_window *obj = (zoom_window *) object;
	obj->refresh();
}

void zoom_window::render(std::unordered_map<std::string, std::tuple<cv::Mat, bool>>::iterator ref) {
	cv::Mat *img = &std::get<0>(ref->second);

	int window_w, window_h;
	if (img->cols > img->rows) {
		window_w = *param_max_window_size;
		window_h = *param_max_window_size * img->rows / img->cols;
	}
	else {
		window_w = *param_max_window_size * img->cols / img->rows;
		window_h = *param_max_window_size;
	}

	int view_w, view_h;
	view_w = (*param_zoom_level * window_w + (100 - *param_zoom_level) * img->cols) / 100;
	view_h = (*param_zoom_level * window_h + (100 - *param_zoom_level) * img->rows) / 100;

	int position_x, position_y;
	position_x = pos_x * (img->cols - view_w);
	position_y = pos_y * (img->rows - view_h);

	cv::Rect roi(position_x, position_y, view_w, view_h);

	if (std::get<1>(ref->second)) {
		cv::Mat overview = (*img).clone();
		rectangle(overview, roi, cv::Scalar(0, 0, 255), 3);
		resize(overview, overview, cv::Size(window_w, window_h));
		cv::imshow(ref->first, overview);

		overview_size = cv::Size(window_w, window_h);
		scaled_roi_size = cv::Size(view_w * window_w / img->cols, view_h * window_h / img->rows);
	}
	else {
		cv::Mat scaled;
		resize((*img)(roi), scaled, cv::Size(window_w, window_h));
		cv::imshow(ref->first, scaled);
	}
}

void zoom_window::refresh() {
	for (auto i = displayed_images.begin(); i != displayed_images.end(); ++i) {
		render(i);
	}
}

}; // namespace
