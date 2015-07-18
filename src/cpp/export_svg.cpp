#include <cstdio>
#include "v_image.h"
#include "export_svg.h"

namespace vect {

v_co editable::group_col;

void editable::write_line(FILE *fd, const v_line &line) { // Write one `v_line' in svg format to output in editable way - one path.
	auto segment = line.segment.cbegin();
	if ((line.get_group() == group_normal) || (line.get_group() == group_first)) {
		fprintf(fd, "    <path\n");
		fprintf(fd, "       d=\"M %f %f", segment->main.x, segment->main.y);
	}
	else if ((line.get_group() == group_continue) || (line.get_group() == group_last)) {
		fprintf(fd, " Z\n");
		fprintf(fd, "          M %f %f", segment->main.x, segment->main.y);
	}

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
	else {
		if ((line.get_group() == group_normal) || (line.get_group() == group_first)) {
			group_col = color;
		}
		if ((line.get_group() == group_normal) || (line.get_group() == group_last)) {
			fprintf(fd, " Z\"\n       style=\"fill:#%02x%02x%02x;stroke:none\" />\n", group_col.val[0], group_col.val[1], group_col.val[2]); // filled regions are closed (Z) and have no stroke.
		}
	}
};

void grouped::write_line(FILE *fd, const v_line &line) { // Write one `v_line' in svg format to output with changing colors and width using group tag.
	if (line.get_type() == fill)
		return editable::write_line(fd, line); // Filled regions should be rendered in `editable' way.

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
		fprintf(fd, "\n       style=\"fill:none;stroke:#%02x%02x%02x;stroke-width:%fpx;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:%f\" />\n", color.val[0], color.val[1], color.val[2], width/2, opacity/2);
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
