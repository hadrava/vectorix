#ifndef VECTORIX__CONFIG_H
#define VECTORIX__CONFIG_H

// Compiletime configuration

namespace vectorix {

typedef float p; // Vector data precision -- float should be enought
const char p_scanf[] = "%f"; // scanf string for loading type "p"
const char p_printf[] = "%f"; // printf string format for printing type "p"
const p epsilon = 0.00001f; // small value for comparision of two numbers

}; // namespace

#define VECTORIX_MAX_VERBOSITY   (log_level::debug) // debug = everything can be logged

#define VECTORIX_PNM_VERBOSITY   (log_level::warning)

#define VECTORIX_VECTORIZER_VERBOSITY   (log_level::debug)

#define VECTORIX_HIGHGUI // Use opencv/highgui windows
//#define VECTORIX_OUTLINE_DEBUG // Show convert to outline debug messages
//#define VECTORIX_DEBUG // Enable run-time asserts

#endif
