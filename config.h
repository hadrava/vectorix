#ifndef VECTORIX__CONFIG_H
#define VECTORIX__CONFIG_H

// Compiletime configuration

namespace vectorix {

typedef float p; // Vector data precision -- float should be enought
const char p_scanf[] = "%f"; // scanf string for loading type "p"
const char p_printf[] = "%f"; // printf string format for printing type "p"
const p epsilon = 0.00001f; // small value for comparision of two numbers

}; // namespace

#define VECTORIX_TIMER_MEASURE // Enable time measurement with usec precision (requires <sys/time.h>)
#define VECTORIX_VECTORIZER_DEBUG // Just show vectorizer debug messages
#define VECTORIX_HIGHGUI // Use opencv/highgui windows
//#define VECTORIX_OUTLINE_DEBUG // Show convert to outline debug messages
//#define VECTORIX_DEBUG // Enable run-time asserts

#endif
