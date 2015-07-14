#ifndef _TIME_MEASUREMENT_H
#define _TIME_MEASUREMENT_H

#include "config.h"

#ifdef TIMER_MEASURE

#include <sys/time.h>

namespace tmea {

typedef long long int usec_t;

class timer {
public:
	timer(usec_t prewarm_time = 700000): prewarm_time_(prewarm_time) {};
	inline void start() {
		usec_t t = get_usec();
		while (get_usec() - t < prewarm_time_)
			;
		start_ = get_usec();
	};
	inline void stop() {
		stop_ = get_usec();
	};
	inline usec_t read() {
		return stop_ - start_;
	};
private:
	static inline usec_t get_usec() {
		struct timeval time;
		gettimeofday(&time, NULL);
		return 1000000 * time.tv_sec + time.tv_usec;
	}
	usec_t prewarm_time_;
	usec_t start_;
	usec_t stop_;
};

}; // namespace

#else

namespace tmea {

typedef long long int usec_t;

class timer {
public:
	timer(usec_t prewarm_time = 700000) {};
	inline void start() {};
	inline void stop() {};
	inline usec_t read() { return 0; };
};

}; // namespace

#endif

#endif
