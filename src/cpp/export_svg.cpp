#include "lines.h"
#include "export_svg.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <locale.h>

namespace vect {

void svg_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	return;
}

void svg_write_header(FILE *fd, const v_image &image) {
	fprintf(fd, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
	fprintf(fd, "<svg\n");
	fprintf(fd, "   xmlns:svg=\"http://www.w3.org/2000/svg\"\n");
	fprintf(fd, "   xmlns=\"http://www.w3.org/2000/svg\"\n");
	fprintf(fd, "   version=\"1.1\"\n");
	fprintf(fd, "   width=\"%f\"\n", image.width);
	fprintf(fd, "   height=\"%f\"\n", image.height);
	fprintf(fd, "   id=\"svg\">\n");
	fprintf(fd, "  <g\n");
	fprintf(fd, "     id=\"layer1\">\n");

}

void svg_write_footer(FILE *fd, const v_image &image) {
	fprintf(fd, "  </g>\n");
	fprintf(fd, "</svg>\n");
}

void svg_write(FILE *fd, const v_image &image) {
	setlocale(LC_ALL, "C");
	svg_write_header(fd, image);
	for (v_line line: image.line) {
		if (line.segment.empty())
			continue;
		auto segment = line.segment.cbegin();
		fprintf(fd, "    <path\n");
		fprintf(fd, "       d=\"M %f %f", segment->main.x, segment->main.y);

		v_pt cn = segment->control_next;
		int count = 1;
		p width = segment->width;
		p opacity = segment->opacity;
		v_co color = segment->color;
		segment++;
		while (segment != line.segment.cend()) {
			fprintf(fd, " C %f %f %f %f %f %f", cn.x, cn.y, segment->control_prev.x, segment->control_prev.y, segment->main.x, segment->main.y);
			cn = segment->control_next;
			count ++;
			width += segment->width;
			opacity += segment->opacity;
			color += segment->color;
			segment++;
		}
		color /= count;
		fprintf(fd, "\"\n");
		fprintf(fd, "       style=\"fill:none;stroke:#%02x%02x%02x;stroke-width:%fpx;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:%f\" />\n", color.val[0], color.val[1], color.val[2], width/count, opacity/count);
	}
	svg_write_footer(fd, image);
}

};
