#include <cstdio>
#include <locale>
#include "v_image.h"
#include "export_ps.h"

namespace vect {

v_co export_ps::group_col;
p export_ps::height;

void export_ps::write(FILE *fd, const v_image & image) {
	std::locale("C");
	ps_write_header(fd, image);
	for (v_line line: image.line) {
		if (line.segment.empty())
			continue;
		write_line(fd, line);
	}
	ps_write_footer(fd, image);
};

void export_ps::ps_write_header(FILE *fd, const v_image &image) {
	fprintf(fd, "%%!PS-Adobe-3.0 EPSF-3.0\n");
	int w = image.width;
	int h = image.height;
	height = image.height;
	fprintf(fd, "%%%%BoundingBox: 0 0 %d %d\n", w, h);
	fprintf(fd, "gsave\n");
};

void export_ps::ps_write_footer(FILE *fd, const v_image &image) {
	fprintf(fd, "grestore\n");
	fprintf(fd, "%%EOF\n");
};

void export_ps::write_line(FILE *fd, const v_line &line) {
	auto segment = line.segment.cbegin();
	if ((line.get_group() == group_normal) || (line.get_group() == group_first)) {
		fprintf(fd, "%f %f moveto\n", segment->main.x, height - segment->main.y);
	}
	else if ((line.get_group() == group_continue) || (line.get_group() == group_last)) {
		fprintf(fd, "%f %f moveto\n", segment->main.x, height - segment->main.y);
	}

	v_pt cn = segment->control_next;
	int count = 1;
	p width = segment->width;
	p opacity = segment->opacity;
	v_co color = segment->color;
	segment++;
	while (segment != line.segment.cend()) {
		fprintf(fd, "%f %f %f %f %f %f curveto\n", cn.x, height - cn.y, segment->control_prev.x, height - segment->control_prev.y, segment->main.x, height - segment->main.y);
		cn = segment->control_next;
		count ++;
		width += segment->width;
		opacity += segment->opacity;
		color += segment->color;
		segment++;
	}
	color /= count;
	if (line.get_type() == stroke) {
		fprintf(fd, "1 setlinecap\n");
		fprintf(fd, "1 setlinejoin\n");
		fprintf(fd, "%f setlinewidth\n", width/count);
		fprintf(fd, "%f %f %f setrgbcolor stroke\n", color.val[0]/255.f, color.val[1]/255.f, color.val[2]/255.f);
	}
	else {
		if ((line.get_group() == group_normal) || (line.get_group() == group_first)) {
			group_col = color;
		}
		if ((line.get_group() == group_normal) || (line.get_group() == group_last)) {
			fprintf(fd, "%f %f %f setrgbcolor fill\n", group_col.val[0]/255.f, group_col.val[1]/255.f, group_col.val[2]/255.f);
		}
	}
};

}; // namespace
