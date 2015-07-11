#include "lines.h"
#include "export_svg.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <locale.h>

void svg_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	return;
}

void svg_write_header(FILE *fd, const class v_image &image) {
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

void svg_write_footer(FILE *fd, const class v_image &image) {
	fprintf(fd, "  </g>\n");
	fprintf(fd, "</svg>\n");
}

void svg_write(FILE *fd, const class v_image &image) {
	setlocale(LC_ALL, "C");
	svg_write_header(fd, image);
	for (v_line line: image.line) {
		auto segment = line.segment.cbegin();
		fprintf(fd, "    <path\n");
		fprintf(fd, "       d=\"M %f %f C %f %f", segment->main.x, segment->main.y, segment->control_next.x, segment->control_next.y);
		segment++;
		auto segment2 = segment;
		segment2++;
		while (segment2 != line.segment.cend()) {
			fprintf(fd, " %f %f %f %f C %f %f", segment->control_prev.x, segment->control_prev.y, segment->main.x, segment->main.y, segment->control_next.x, segment->control_next.y);
			segment++;
			segment2++;
		}
		fprintf(fd, " %f %f %f %f", segment->control_prev.x, segment->control_prev.y, segment->main.x, segment->main.y);
		fprintf(fd, "\"\n");
		fprintf(fd, "       style=\"fill:none;stroke:#000000;stroke-width:%fpx;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:%f\" />\n", segment->width, segment->opacity);
	}
	svg_write_footer(fd, image);
}

void v_free(class v_image &&image) {
	for (v_line line: image.line) {
		line.segment.erase(line.segment.begin(), line.segment.end());
	}
	image.line.erase(image.line.begin(), image.line.end());
}
