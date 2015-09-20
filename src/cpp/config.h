#ifndef _CONFIG_H
#define _CONFIG_H

// Compiletime configuration

namespace vect {

typedef float p; // Vector data precision -- float should be enought
const char p_scanf[] = "%f"; // scanf string for loading type "p"
const char p_printf[] = "%f"; // printf string format for printing type "p"
const p epsilon = 0.00001f; // small value for comparision of two numbers

}; // namespace

#define TIMER_MEASURE // Enable time measurement with usec precision (requires <sys/time.h>)
#define VECTORIZER_DEBUG // Just show vectorizer debug messages
#define VECTORIZER_HIGHGUI // Use opencv/highgui windows
#define VECTORIZER_USE_ROI // Use "region of interest" optimization -- works better with larger images
#define VECTORIZER_STARTING_POINTS // Use starting points optimization -- use vector of all possible starting points
//#define DEBUG // Enable run-time asserts

#endif
