#ifndef _CONFIG_H
#define _CONFIG_H

namespace vect {

typedef float p; // Vector data precision. Float should me enought.
const char p_scanf[] = "%f";
const char p_printf[] = "%f";
const p epsilon = 0.00001f;

}; // namespace

#define TIMER_MEASURE // Enable time measurement with usec precision. (Requires <sys/time.h>.)
#define VECTORIZER_DEBUG // Just show vectorizer debug messages.
#define VECTORIZER_HIGHGUI // Use opencv/highgui windows.
#define VECTORIZER_USE_ROI
//#define DEBUG // Enable asserts.

#endif
