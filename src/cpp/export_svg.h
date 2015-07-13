#ifndef _EXPORT_SVG_H
#define _EXPORT_SVG_H

#include <cstdio>
#include <cstdarg>
#include <locale>
#include "lines.h"


namespace vect {

class editable {
public:
	static void write_line(FILE *fd, const v_line &line);
};

class grouped {
public:
	static void write_line(FILE *fd, const v_line &line);
};


template <class Exporter = editable>
class export_svg {
public:
	static void write(FILE *fd, const v_image & image) {
		std::locale("C");
		svg_write_header(fd, image);
		for (v_line line: image.line) {
			if (line.segment.empty())
				continue;
			Exporter::write_line(fd, line);
		}
		svg_write_footer(fd, image);
	};
private:
	static void svg_error(const char *format, ...) {
		va_list args;
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
	};

	static void svg_write_header(FILE *fd, const v_image &image) {
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
	};

	static void svg_write_footer(FILE *fd, const v_image &image) {
		fprintf(fd, "  </g>\n");
		fprintf(fd, "</svg>\n");
	};
};

}; // namespace

#endif
