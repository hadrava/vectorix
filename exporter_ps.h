#ifndef VECTORIX__EXPORTER_PS_H
#define VECTORIX__EXPORTER_PS_H

//Post Script exporter

#include <cstdio>
#include "exporter.h"
#include "v_image.h"

namespace vectorix {

class exporter_ps: public exporter {
private:
	virtual void write_header();
	virtual void write_line(const v_line &line);
	virtual void write_footer();

	v_co group_col; // Color of current group
};

}; // namespace

#endif
