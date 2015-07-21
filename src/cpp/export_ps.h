#ifndef _EXPORT_PS_H
#define _EXPORT_PS_H

#include <cstdio>
#include "v_image.h"

namespace vect {

class export_ps {
public:
	static void write(FILE *fd, const v_image & image);
private:
	static void ps_write_header(FILE *fd, const v_image &image);
	static void ps_write_footer(FILE *fd, const v_image &image);
	static void write_line(FILE *fd, const v_line &line);
	static v_co group_col;
	static p height;
};

}; // namespace

#endif
