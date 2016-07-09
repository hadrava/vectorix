#ifndef VECTORIX__EXPORTER_H
#define VECTORIX__EXPORTER_H

#include <cstdio>
#include "v_image.h"

// Generic exporter

namespace vectorix {

class exporter {
public:
	void write(FILE *fd_, const v_image &image_); // Write image to open filedescriptor
private:
	virtual void write_header() = 0;
	virtual void write_line(const v_line &line) = 0;
	virtual void write_footer() = 0;
protected:
	exporter() = default;
	FILE *fd;
	v_image const *image;
};

}; // namespace

#endif
