#include <cstdio>
#include "v_image.h"
#include "export_svg.h"

namespace vect {

void editable::write_line(FILE *fd, const v_line &line) {
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
	if (line.get_type() == stroke)
		fprintf(fd, "\"\n       style=\"fill:none;stroke:#%02x%02x%02x;stroke-width:%fpx;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:%f\" />\n", color.val[0], color.val[1], color.val[2], width/count, opacity/count);
	else
		fprintf(fd, " Z\"\n       style=\"fill:#%02x%02x%02x;stroke:none\" />\n", color.val[0], color.val[1], color.val[2]);
};

void grouped::write_line(FILE *fd, const v_line &line) {
	if (line.get_type() == fill)
		return editable::write_line(fd, line);

	auto segment = line.segment.cbegin();
	fprintf(fd, "    <g>\n");

	v_pt cn = segment->control_next;
	v_pt ma = segment->main;

	p width = segment->width;
	p opacity = segment->opacity;
	v_co color = segment->color;
	segment++;
	while (segment != line.segment.cend()) {
		width += segment->width;
		opacity += segment->opacity;
		color += segment->color;
		color /= 2;

		fprintf(fd, "    <path d=\"M %f %f", ma.x, ma.y);
		fprintf(fd, " C %f %f %f %f %f %f\"", cn.x, cn.y, segment->control_prev.x, segment->control_prev.y, segment->main.x, segment->main.y);
		fprintf(fd, "       style=\"fill:none;stroke:#%02x%02x%02x;stroke-width:%fpx;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:%f\" />\n", color.val[0], color.val[1], color.val[2], width/2, opacity/2);
		cn = segment->control_next;
		ma = segment->main;
		width = segment->width;
		opacity = segment->opacity;
		color = segment->color;
		segment++;
	}
	fprintf(fd, "    </g>\n");
};

}; // namespace
