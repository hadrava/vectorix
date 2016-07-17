#ifndef VECTORIX__CONFIG_H
#define VECTORIX__CONFIG_H

// Compiletime configuration

namespace vectorix {

typedef double p; // Vector data precision -- float is working too, but double is much safer
const p epsilon = 0.00001f; // small value for comparision of two numbers

}; // namespace

#define VECTORIX_MAX_VERBOSITY   (log_level::debug) // debug = everything can be logged

//#define VECTORIX_OUTLINE_DEBUG // Show convert to outline debug messages
//#define VECTORIX_DEBUG // Enable run-time asserts

#endif
