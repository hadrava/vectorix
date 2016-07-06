#ifndef VECTORIX__FINISHER_H
#define VECTORIX__FINISHER_H

#include "v_image.h"
#include "parameters.h"

namespace vectorix {

class finisher {
public:
	void apply_settings(v_image &vector, const params &parameters);
};

}; // namespace

#endif
