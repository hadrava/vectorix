#ifndef VECTORIX__LOGGER_H
#define VECTORIX__LOGGER_H

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include "config.h"

// LOGGER

namespace vectorix {

enum class log_level {
	error   = 0,
	warning = 1,
	info    = 2,
	debug   = 3
};

class logger {
public:
	logger() = default;
	logger(log_level verbosity): verbosity_(verbosity) {};
	void set_verbosity(log_level verbosity) {
		verbosity_ = verbosity;
	}

	template <log_level L> void log(const char *format, ...) {
		if (L <= VECTORIX_MAX_VERBOSITY) {
			if (L <= verbosity_) {
				fprintf(stderr, "<%d, %d>: ", L, verbosity_);
				va_list args;
				va_start(args, format);
				vfprintf(stderr, format, args);
				va_end(args);
			}
		}
	};

private:
	log_level verbosity_;
};

}; // namespace

#endif
