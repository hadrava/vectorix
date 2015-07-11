#include "pnm_handler.h"
#include <utility>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define MAX_LINE_LENGTH	512

void pnm_image::pnm_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	return;
}

int pnm_image::fscanf_comment(FILE *stream, const char *format, ...) {
	char line[MAX_LINE_LENGTH];
	for (;;) {
		fgets(line, MAX_LINE_LENGTH, stream);
		if (!line)
			return 0;
		if (line[0] != '#')
			break;
	}
	va_list args;
	va_start(args, format);
	int ret = vsscanf(line, format, args);
	va_end(args);
	return ret;
}

void pnm_image::read_header(FILE *fd) {
	char magic, ntype;
	if (fscanf_comment(fd, "%c%c\n", &magic, &ntype) != 2 || magic != 'P' || ntype < '1' || ntype > '6') {
		pnm_error("Error reading image header!\n");
		return;
	}
	if (fscanf_comment(fd, "%i%i\n", &width, &height) != 2) {
		pnm_error("Error reading image dimensions!\n");
		return;
	}
	if (ntype != '1' && ntype != '4') {
		if (fscanf_comment(fd, "%i\n", &maxvalue) != 1) {
			pnm_error("Error reading image maxvalue!\n");
			return;
		}
	}
	type = ntype - '0';
	if (data)
		delete[] data;
	data = NULL;
}

void pnm_image::write_header(FILE *fd) {
	fprintf(fd, "P%c\n", type + '0');
	fprintf(fd, "%i %i\n", width, height);
	if (type != 1 && type != 4) {
		fprintf(fd, "%i\n", maxvalue);
	}
}


void pnm_image::read(FILE *fd) {
	read_header(fd);

	int nsize = size();
#ifdef PNM_DEBUG
	write_header(stderr);
#endif

	if (nsize) {
		char scanf_string[8];
		if (type == PNM_ASCII_PBM)
			strcpy(scanf_string, "%1i");
		else if (type <= PNM_ASCII_PPM)
			strcpy(scanf_string, "%i");
		else if (type >= PNM_BINARY_PBM)
			strcpy(scanf_string, "%c");

		data = new int [nsize];
		if (!data) {
			return;
		}
		for (int i = 0; i < nsize; i++) {
			data[i] = 0;
			if (fscanf(fd, scanf_string, data+i) != 1) {
				pnm_error("Error: reading image data failed at position %i!\n", i);
				return;
			}
		}
	}
}

void pnm_image::write(FILE *fd) {
#ifdef DEBUG
	if (!data) {
		pnm_error("Error: trying to acces NULLpointer by pnm_write()\n");
		return;
	}
#endif
	write_header(fd);
	int nsize = size();
	if (nsize && (type <= PNM_ASCII_PPM)) {
		for (int i = 0; i < nsize; i++)
			fprintf(fd, "%i ", data[i]);
	}
	if (nsize && (type >= PNM_BINARY_PBM)) {
		for (int i = 0; i < nsize; i++)
			fprintf(fd, "%c", data[i]);
	}
}

void pnm_image::convert(int new_type) {
	auto dest = pnm_image(width, height, new_type);

	int convert_type = (type <= PNM_ASCII_PPM) ? type : type - 3;
	convert_type |= ((dest.type <= PNM_ASCII_PPM) ? dest.type : dest.type - 3) << 4;

	if ((type == PNM_BINARY_PBM) || (dest.type == PNM_BINARY_PBM)) {
		pnm_error("Conversion from/to binary PBM is not implemented!\n");
		return;
	}

	int new_size = dest.size();

	switch (convert_type) {
		case (PNM_ASCII_PBM << 4) | PNM_ASCII_PBM:
		case (PNM_ASCII_PGM << 4) | PNM_ASCII_PGM:
		case (PNM_ASCII_PPM << 4) | PNM_ASCII_PPM:
			memcpy(dest.data, data, sizeof(int)*new_size);
			break;
		case (PNM_ASCII_PGM << 4) | PNM_ASCII_PBM:
			for (int i = 0; i < new_size; i++)
				dest.data[i] = dest.maxvalue * (1-data[i]);
			break;
		case (PNM_ASCII_PPM << 4) | PNM_ASCII_PBM:
			for (int i = 0; i < new_size; i+=3) {
				dest.data[i+0] = dest.maxvalue * (1-data[i/3]);
				dest.data[i+1] = dest.maxvalue * (1-data[i/3]);
				dest.data[i+2] = dest.maxvalue * (1-data[i/3]);
			}
			break;
		case (PNM_ASCII_PBM << 4) | PNM_ASCII_PGM:
			for (int i = 0; i < new_size; i++)
				dest.data[i] = (data[i] >= 128) ? 0 : 1;
			break;
		case (PNM_ASCII_PPM << 4) | PNM_ASCII_PGM:
			for (int i = 0; i < new_size; i+=3) {
				dest.data[i+0] = data[i/3];
				dest.data[i+1] = data[i/3];
				dest.data[i+2] = data[i/3];
			}
			break;
		case (PNM_ASCII_PBM << 4) | PNM_ASCII_PPM:
			for (int i = 0; i < new_size; i++)
				dest.data[i] = (data[i*3] + data[i*3 + 1] + data[i*3 + 2] >= 128*3) ? 0 : 1;
			break;
		case (PNM_ASCII_PGM << 4) | PNM_ASCII_PPM:
			for (int i = 0; i < new_size; i++)
				dest.data[i] = (data[i*3] + data[i*3 + 1] + data[i*3 + 2]) / 3;
			break;
		default:
			pnm_error("Unknown conversion types (from %i, to %i).\n", type, dest.type);
			return;
			break;
	}

	*this = std::move(dest);
}

pnm_image::~pnm_image() {
	if (data)
		delete[] data;
}

void pnm_image::erase_image() {
	memset(data, 255, size()*sizeof(int));
}

int pnm_image::size() {
	int size = 0;
	switch (type) {
		case PNM_ASCII_PBM: //(black/white)
			size = width * height;
			break;
		case PNM_BINARY_PBM: //(black/white)
			size = width * height / 8;
			break;
		case PNM_ASCII_PGM: //(gray scale)
		case PNM_BINARY_PGM: //(gray scale)
			size = width * height;
			break;
		case PNM_ASCII_PPM: //(RGB)
		case PNM_BINARY_PPM: //(RGB)
			size = width * height * 3;
			break;
	}
#ifdef PNM_DEBUG
	pnm_error("Debug: size = %i\n", size);
#endif
	return size;
}

int pnm_image::guess_maxvalue() {
	int maxvalue = 0;
	switch (type) {
		case PNM_ASCII_PBM:
		case PNM_BINARY_PBM:
			maxvalue = 1;
			break;
		default:
			maxvalue = 255;
	}
	return maxvalue;
}

class pnm_image &pnm_image::operator=(class pnm_image &&move) {
	width = move.width;
	height = move.height;
	type = move.type;
	maxvalue = move.maxvalue;
	if (data)
		delete[] data;
	data = move.data;
	move.data = NULL;
}
