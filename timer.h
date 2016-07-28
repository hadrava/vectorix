/*
 * Vectorix -- line-based image vectorizer
 * (c) 2016 Jan Hadrava <had@atrey.karlin.mff.cuni.cz>
 */
#ifndef VECTORIX__TIME_MEASUREMENT_H
#define VECTORIX__TIME_MEASUREMENT_H

// Time measurement

#include "config.h"
#include <chrono>
#include <ctime>

namespace vectorix {

class timer {
public:
	timer(double prewarm_time = 0.): prewarm_time_(prewarm_time) {}; // Prewarm the CPU for prewarm_time (in seconds)

	inline void start() { // Start measuring time
		auto time = std::chrono::system_clock::now();

		while (std::chrono::system_clock::now() - time < prewarm_time_)
			;
		start_ = std::chrono::system_clock::now();
	};

	inline void stop() { // Stop measuring time
		stop_ = std::chrono::system_clock::now();
	};

	inline double read() { // Get time spent (in seconds)
		std::chrono::duration<double> duration = stop_ - start_;
		return duration.count();
	};

private:
	std::chrono::time_point<std::chrono::system_clock> start_, stop_;
	std::chrono::duration<double> prewarm_time_;
};

}; // namespace

#endif
