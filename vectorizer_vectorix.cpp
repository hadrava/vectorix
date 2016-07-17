#include <opencv2/opencv.hpp>
#include "v_image.h"
#include "geom.h"
#include "pnm_handler.h"
#include "vectorizer.h"
#include "vectorizer_vectorix.h"
#include "timer.h"
#include "parameters.h"
#include <string>
#include <cmath>
#include <algorithm>
#include <tuple>
#include <vector>
#include "logger.h"
#include "thresholder.h"
#include "skeletonizer.h"
#include "tracer.h"
#include "zoom_window.h"

// Vectorizer

using namespace cv;

namespace vectorix {

int vectorizer_vectorix::interactive(int state, int key) { // Process key press and decide what to do
	int ret = state;
	switch (key & 0xFF) {
		case 0:
		case 0xFF:
		case -1:
			break; // Nothing
		case 'q':
		case 'Q':
		case 27: // Esc
			ret = 0; // Quit program
			break;
		case 'r':
		case 'R':
			ret = 1; // Rerun from step 1
			break;
		case '\n':
			ret++; // Next vectorization step
			break;
		case 'h':
		case 'H':
		default:
			log.log<log_level::info>("Help:\n");
			log.log<log_level::info>("\tEnter\tContinue with next step\n");
			log.log<log_level::info>("\tr\tRerun vectorization from begining\n");
			log.log<log_level::info>("\tq, Esc\tQuit\n");
			log.log<log_level::info>("\th\tHelp\n");
			break;
	}
	return ret;
}

void vectorizer_vectorix::step1_changed(int, void *ptr) { // Parameter in step 1 changed, rerun
	volatile int *state = static_cast<volatile int *> (ptr);
	*state = 2; // first step
}

void vectorizer_vectorix::step2_changed(int, void *ptr) { // Parameter in step 2 changed, rerun from this state
	volatile int *state = static_cast<volatile int *> (ptr);
	if (*state >= 4)
		*state = 2; // second step
}

void vectorizer_vectorix::load_image(const pnm_image &original) {
	// Original should be PPM image (color)
	orig = Mat(original.height, original.width, CV_8UC(3));
	if (param_custom_input_name->empty()) {
		for (int j = 0; j < original.height; j++) { // Copy data from PNM image to OpenCV image structures
			for (int i = 0; i<original.width; i++) {
				orig.at<Vec3b>(j, i)[2] = original.data[(i+j*original.width)*3 + 0];
				orig.at<Vec3b>(j, i)[1] = original.data[(i+j*original.width)*3 + 1];
				orig.at<Vec3b>(j, i)[0] = original.data[(i+j*original.width)*3 + 2];
			}
		}
	}
	else {
		// Configuration tell us to read image from file directly by OpenCV
		orig = imread(*param_custom_input_name, CV_LOAD_IMAGE_COLOR);
	}
}

v_image vectorizer_vectorix::vectorize(const pnm_image &original) {
	load_image(original);

	v_image vect = v_image(orig.cols, orig.rows); // Vector output
	thresholder thr(*par);
	skeletonizer ske(*par);
	tracer tra(*par);

	if (*param_interactive) {
		timer threshold_timer;
		timer skeletonization_timer;
		timer tracing_timer;
		volatile int state = 2; // State of vectorizer, remembers in which step we are

		while (state) { // state 0 = end
			switch (state) {
				case 2: // First step
					zoom_imshow("Original", orig, true); // Show original color image
					//createTrackbar("Zoom out", "Original", param_zoom_level, 100, step1_changed, (void*) &state);
					waitKey(1); // interactive == 1: wait until the key is pressed; interactive == 0: Continue after one milisecond

					threshold_timer.start();
					thr.run(orig, binary);
					threshold_timer.stop();
					log.log<log_level::info>("Threshold time: %fs\n", threshold_timer.read());
					thr.interactive(step1_changed, (void*) &state);

					state++; // ... and wait in odd state for Enter
					break;
				case 4:
					skeletonization_timer.start();
					ske.run(binary, skeleton, distance); // Second step -- skeletonization
					skeletonization_timer.stop();
					log.log<log_level::info>("Skeletonization time: %fs\n", skeletonization_timer.read());
					ske.interactive(step2_changed, (void*) &state);

					state++; // ... and wait in odd state for Enter
					break;
				case 6:
					tracing_timer.start();
					tra.run(orig, skeleton, distance, vect);
					tracing_timer.stop();
					log.log<log_level::info>("Tracing time: %fs\n", tracing_timer.read());

					state++; // ... and wait in odd state for Enter
					break;
				case 8:
					state = 0;
					break;
				default:
					int key = waitKey(1);
					if (key >= 0)
						log.log<log_level::debug>("Key: %i\n", key);
					state = interactive(state, key);
			}
		}
	}
	else {
		thr.run(orig, binary);
		ske.run(binary, skeleton, distance); // Second step -- skeletonization
		tra.run(orig, skeleton, distance, vect);
	}
	log.log<log_level::debug>("end of vectorization\n");

	return vect;
}

}; // namespace
