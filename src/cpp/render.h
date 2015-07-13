#ifndef _RENDER_H
#define _RENDER_H

#include "pnm_handler.h"
#include "v_image.h"

namespace vect {

class renderer {
public:
	static pnm::pnm_image render(const vect::v_image &vector);
protected:
	static void render_error(const char *format, ...);
	static void bezier_render(pnm::pnm_image &bitmap, const v_line &line);
};

}; // namespace

#endif
