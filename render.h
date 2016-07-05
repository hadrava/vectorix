#ifndef VECTORIX__RENDER_H
#define VECTORIX__RENDER_H

// Simple renderer from v_image to pnm bitmap
// Does not need OpenCV
// Warning: Ignores fill type -- renders everything as lines

#include "pnm_handler.h"
#include "v_image.h"

namespace vectorix {

class renderer {
public:
	static pnm_image render(const v_image &vector);
protected:
	static void render_error(const char *format, ...);
	static void bezier_render(pnm_image &bitmap, const v_line &line); // render one line
};

}; // namespace

#endif
