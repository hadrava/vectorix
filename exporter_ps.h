/*
 * Vectorix -- line-based image vectorizer
 * (c) 2016 Jan Hadrava <had@atrey.karlin.mff.cuni.cz>
 */
#ifndef VECTORIX__EXPORTER_PS_H
#define VECTORIX__EXPORTER_PS_H

#include <cstdio>
#include "exporter.h"
#include "v_image.h"

// Post Script exporter

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
