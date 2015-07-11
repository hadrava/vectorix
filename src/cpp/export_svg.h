#ifndef _EXPORT_SVG_H
#define _EXPORT_SVG_H

#include "lines.h"
#include <stdio.h>

namespace vect {

void svg_write(FILE *fd, const class v_image & image);

};

#endif
