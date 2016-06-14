#ifndef _EXPORT_PS_H
#define _EXPORT_PS_H

//Post Script export

#include <cstdio>
#include "v_image.h"

namespace vectorix {

class export_ps {
public:
	static void write(FILE *fd, const v_image & image); // Write image to open filedescriptor
private:
	static void ps_write_header(FILE *fd, const v_image &image); // Header
	static void ps_write_footer(FILE *fd, const v_image &image); // Footer
	static void write_line(FILE *fd, const v_line &line); // One line
	static v_co group_col; // Color of current group
	static p height; // Height is needed for transforming coordinates
};

}; // namespace

#endif
