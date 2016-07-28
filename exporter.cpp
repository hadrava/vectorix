/*
 * Vectorix -- line-based image vectorizer
 * (c) 2016 Jan Hadrava <had@atrey.karlin.mff.cuni.cz>
 */
#include <cstdio>
#include <locale>
#include "v_image.h"
#include "exporter.h"

// Generic exporter

namespace vectorix {

void exporter::write(FILE *fd_, const v_image &image_) {
	fd = fd_;
	image = &image_;

	std::locale("C"); // Set locale for printing flaoting-point
	write_header();
	for (const v_line &line: image->line) {
		if (line.segment.empty()) // Skip empty lines
			continue;
		write_line(line);
	}
	write_footer();
}

}; // namespace
