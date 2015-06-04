#include "svg_handler.h"
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

void svg_write_header(FILE *fd, const struct svg_image * image) {
	fprintf(fd, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
	fprintf(fd, "<svg\n");
	//fprintf(fd, "   xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n");
	//fprintf(fd, "   xmlns:cc=\"http://creativecommons.org/ns#\"\n");
	//fprintf(fd, "   xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n");
	fprintf(fd, "   xmlns:svg=\"http://www.w3.org/2000/svg\"\n");
	fprintf(fd, "   xmlns=\"http://www.w3.org/2000/svg\"\n");
	fprintf(fd, "   version=\"1.1\"\n");
	fprintf(fd, "   width=\"%i\"\n", image->width);
	fprintf(fd, "   height=\"%i\"\n", image->height);
	fprintf(fd, "   id=\"svg\">\n");
	fprintf(fd, "  <g\n");
	fprintf(fd, "     id=\"layer1\">\n");

}

void svg_write_footer(FILE *fd, const struct svg_image * image) {
	fprintf(fd, "  </g>\n");
	fprintf(fd, "</svg>\n");
}

void svg_write(FILE *fd, const struct svg_image * image) {
	setlocale(LC_ALL, "C");
	svg_write_header(fd, image);
	struct svg_line * line = image->data;
	while (line) {
		struct svg_segment * segment = line->segment;
		fprintf(fd, "    <path\n");
		fprintf(fd, "       d=\"M %f %f C %f %f %f %f %f %f", segment->x0, segment->y0, segment->x1, segment->y1, segment->x2, segment->y2, segment->x3, segment->y3);
		segment = segment->next_segment;
		while (segment) {
			fprintf(fd, " C %f %f %f %f %f %f", segment->x1, segment->y1, segment->x2, segment->y2, segment->x3, segment->y3);
			segment = segment->next_segment;
		}
		fprintf(fd, "\"\n");
		fprintf(fd, "       style=\"fill:none;stroke:#000000;stroke-width:%fpx;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:%f\" />\n", line->width, line->opacity);
		line = line->next;
	}
	svg_write_footer(fd, image);
}

void svg_free(struct svg_image *image) {
#ifdef DEBUG
	if (!image) {
		svg_error("Error: trying to acces NULLpointer by svg_free()\n");
		return;
	}
#endif
	struct svg_line * line = image->data;
	while (line) {
		line = line->next;
		free(image->data);
		image->data = line;
	}
	free(image);
}
