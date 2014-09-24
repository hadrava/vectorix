#include "pnm_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define PNM_ASCII_PBM	1
#define PNM_ASCII_PGM	2
#define PNM_ASCII_PPM	3
#define PNM_BINARY_PBM	4
#define PNM_BINARY_PGM	5
#define PNM_BINARY_PPM	6

#define MAX_LINE_LENGTH	512

void pnm_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	return;
}

int fscanf_comment(FILE *stream, const char *format, ...) {
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

struct pnm_image *pnm_read_header(FILE *fd) {
	char magic, type;
	if (fscanf_comment(fd, "%c%c\n", &magic, &type) != 2 || magic != 'P' || type < '1' || type > '6') {
		pnm_error("Error reading image header!\n");
		return NULL;
	}
	int width, height, maxvalue = 1;
	if (fscanf_comment(fd, "%i%i\n", &width, &height) != 2) {
		pnm_error("Error reading image dimensions!\n");
		return NULL;
	}
	if (type != '1' && type != '4') {
		if (fscanf_comment(fd, "%i\n", &maxvalue) != 1) {
			pnm_error("Error reading image maxvalue!\n");
			return NULL;
		}
	}
	struct pnm_image *image = malloc(sizeof(struct pnm_image));
	image->type = type - '0';
	image->width = width;
	image->height = height;
	image->maxvalue = maxvalue;
	image->data = NULL;
}

void pnm_write_header(FILE *fd, const struct pnm_image * image) {
	fprintf(fd, "P%c\n", image->type + '0');
	fprintf(fd, "%i %i\n", image->width, image->height);
	if (image->type != 1 && image->type != 4) {
		fprintf(fd, "%i\n", image->maxvalue);
	}
}

int pnm_size(const struct pnm_image *image) {
	int size = 0;
	switch (image->type) {
		case PNM_ASCII_PBM: //(black/white)
			size = image->width * image->height;
			break;
		case PNM_BINARY_PBM: //(black/white)
			size = image->width * image->height / 8;
			break;
		case PNM_ASCII_PGM: //(gray scale)
		case PNM_BINARY_PGM: //(gray scale)
			size = image->width * image->height;
			break;
		case PNM_ASCII_PPM: //(RGB)
		case PNM_BINARY_PPM: //(RGB)
			size = image->width * image->height * 3;
			break;
	}

#ifdef PNM_DEBUG
	pnm_error("Debug: pnm_size = %i\n", size);
#endif
	return size;
}

struct pnm_image *pnm_read(FILE *fd) {
	struct pnm_image *image = pnm_read_header(fd);
	if (!image)
		return NULL;

	int size = pnm_size(image);

	if (size) {
		char scanf_string[8];
		if (image->type == PNM_ASCII_PBM)
			strcpy(scanf_string, "%1i");
		else if (image->type <= PNM_ASCII_PPM)
			strcpy(scanf_string, "%i");
		else if (image->type >= PNM_BINARY_PBM)
			strcpy(scanf_string, "%c");

		image->data = malloc(sizeof(int)*size);
		if (!image->data) {
			free(image);
			return NULL;
		}
		for (int i = 0; i < size; i++) {
			if (fscanf(fd, scanf_string, image->data+i) != 1) {
				pnm_error("Error: reading image data failed at position %i!\n", i);
				free(image->data);
				free(image);
				return NULL;
			}
		}
	}
	return image;
}

int pnm_write(FILE *fd, const struct pnm_image *image) {
#ifdef DEBUG
	if (!image || !image->data) {
		pnm_error("Error: trying to acces NULLpointer by pnm_write()\n");
		return 0;
	}
#endif
	pnm_write_header(fd, image);
	int size = pnm_size(image);
	if (size && (image->type <= PNM_ASCII_PPM)) {
		for (int i = 0; i < size; i++)
			fprintf(fd, "%i ", image->data[i]);
	}
	if (size && (image->type >= PNM_BINARY_PBM)) {
		for (int i = 0; i < size; i++)
			fprintf(fd, "%c", image->data[i]);
	}
}
