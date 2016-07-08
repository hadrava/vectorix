#include <cstdio>
#include "v_image.h"
#include "geom.h"
#include "exporter_svg.h"

// SVG exporter

namespace vectorix {

void exporter_svg::write_header() { // Write image header
	fprintf(fd, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
	fprintf(fd, "<svg\n");
	fprintf(fd, "   xmlns:svg=\"http://www.w3.org/2000/svg\"\n");
	fprintf(fd, "   xmlns=\"http://www.w3.org/2000/svg\"\n");
	if (!image->underlay_path.empty()) // Original (or other) image can be linked and displayed in background, needs xlink extension
		fprintf(fd, "   xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n");
	fprintf(fd, "   version=\"1.1\"\n");
	fprintf(fd, "   width=\"%f\"\n", image->width);
	fprintf(fd, "   height=\"%f\"\n", image->height);
	fprintf(fd, "   id=\"svg\">\n");
	fprintf(fd, "  <g\n");
	fprintf(fd, "     id=\"layer1\">\n");
	if (!image->underlay_path.empty()) {
		fprintf(fd, "    <image\n");
		fprintf(fd, "       y=\"0\" x=\"0\"\n");
		fprintf(fd, "       xlink:href=\"file://%s\"\n", image->underlay_path.c_str()); // Display background image
		fprintf(fd, "       width=\"%f\"\n", image->width);
		fprintf(fd, "       height=\"%f\" />\n", image->height);
	}
}

void exporter_svg::write_footer() {
	fprintf(fd, "  </g>\n"); // Closes layer
	fprintf(fd, "</svg>\n");
}

void exporter_svg::write_line(const v_line &line) { // Write one `v_line' in svg format to output in editable way - one path.
	auto segment = line.segment.cbegin();
	if ((line.get_group() == v_line_group::group_first) && (line.get_type() == v_line_type::stroke)) {
		fprintf(fd, "    <g>\n"); // SVG group is used with stroke only
	}
	if ((line.get_type() == v_line_type::stroke) || (line.get_group() == v_line_group::group_normal) || (line.get_group() == v_line_group::group_first)) {
		fprintf(fd, "    <path\n");
		fprintf(fd, "       d=\"M %f %f", segment->main.x, segment->main.y);
	}
	else if ((line.get_group() == v_line_group::group_continue) || (line.get_group() == v_line_group::group_last)) { // type is "fill" and we are not the first in a group
		fprintf(fd, " Z\n"); // close path to form region
		fprintf(fd, "          M %f %f", segment->main.x, segment->main.y); // move to next
	}

	v_pt cn = segment->control_next;
	int count = 1;
	p width = segment->width;
	p opacity = segment->opacity;
	v_co color = segment->color;
	segment++;
	while (segment != line.segment.cend()) {
		fprintf(fd, " C %f %f %f %f %f %f", cn.x, cn.y, segment->control_prev.x, segment->control_prev.y, segment->main.x, segment->main.y); // write next point
		cn = segment->control_next;
		// average width, opacity and color
		count ++;
		width += segment->width;
		opacity += segment->opacity;
		color += segment->color;
		segment++;
	}
	color /= count;
	if (line.get_type() == v_line_type::stroke) {
		fprintf(fd, "\"\n       style=\"fill:none;stroke:#%02x%02x%02x;stroke-width:%fpx;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:%f\" />\n", color.val[0], color.val[1], color.val[2], width/count, opacity/count); // write etyle
		if (line.get_group() == v_line_group::group_last) {
			fprintf(fd, "    </g>\n"); // end group
		}
	}
	else { // style == fill
		if ((line.get_group() == v_line_group::group_normal) || (line.get_group() == v_line_group::group_first)) {
			group_col = color; // save color of first line
		}
		if ((line.get_group() == v_line_group::group_normal) || (line.get_group() == v_line_group::group_last)) {
			fprintf(fd, " Z\"\n       style=\"fill:#%02x%02x%02x;stroke:none\" />\n", group_col.val[0], group_col.val[1], group_col.val[2]); // filled regions are closed (Z) and have no stroke.
		}
	}
}


}; // namespace
