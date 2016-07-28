/*
 * Vectorix -- line-based image vectorizer
 * (c) 2016 Jan Hadrava <had@atrey.karlin.mff.cuni.cz>
 */
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
				switch (L) {
					case log_level::error:
						fprintf(stderr, "<Error>: ");
						break;
					case log_level::warning:
						fprintf(stderr, "<Warning>: ");
						break;
					case log_level::info:
						fprintf(stderr, "<Info>: ");
						break;
					case log_level::debug:
						fprintf(stderr, "<Debug>: ");
						break;
					default:
						fprintf(stderr, "<Unknown>: ");
						break;
				}
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
