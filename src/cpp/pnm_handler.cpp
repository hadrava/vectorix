#include <utility>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <stdexcept>
#include "pnm_handler.h"

namespace pnm {

#define MAX_LINE_LENGTH	512

void pnm_image::pnm_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

int pnm_image::fscanf_comment(FILE *stream, const char *format, ...) { // Read line from PNM header, ignore comments.
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

bool pnm_image::read_header(FILE *fd) { // Read image header and clear old data.
	char magic, ntype;
	if (fscanf_comment(fd, "%c%c\n", &magic, &ntype) != 2 || magic != 'P' || ntype < '1' || ntype > '6') {
		pnm_error("Error reading image header.\n");
		return false;
	}
	if (fscanf_comment(fd, "%i%i\n", &width, &height) != 2) {
		pnm_error("Error reading image dimensions.\n");
		return false;
	}
	if (ntype != '1' && ntype != '4') {
		if (fscanf_comment(fd, "%i\n", &maxvalue) != 1) {
			pnm_error("Error reading image maxvalue.\n");
			return false;
		}
	}
	type = ntype - '0';
	if (data)
		delete[] data;
	data = NULL;
	return true;
}

void pnm_image::write_header(FILE *fd) { // Write image header.
	fprintf(fd, "P%c\n", type + '0');
	fprintf(fd, "%i %i\n", width, height);
	if (type != 1 && type != 4) {
		fprintf(fd, "%i\n", maxvalue);
	}
}


void pnm_image::read(FILE *fd) { // REad image from file.
	if (!read_header(fd))
		throw std::underflow_error("Unable to read image header.");

	int nsize = size(); // Calculate size for data.
#ifdef PNM_DEBUG
	write_header(stderr);
#endif

	if (nsize) {
		char scanf_string[8];
		if (type == PNM_ASCII_PBM)
			strcpy(scanf_string, "%1i"); // Sometimes there are no spaces between numbers.
		else if (type <= PNM_ASCII_PPM)
			strcpy(scanf_string, "%i"); // Whitespace between numbers
		else if (type >= PNM_BINARY_PBM)
			strcpy(scanf_string, "%c"); // Binary data.

		data = new pnm_data_t [nsize];
		for (int i = 0; i < nsize; i++) {
			int read;
			if (fscanf(fd, scanf_string, &read) != 1) {
				pnm_error("Error: reading image data failed at position %i.\n", i);
				throw std::underflow_error("Unable to read image data.");
			}
			data[i] = read;
		}
	}
}

void pnm_image::write(FILE *fd) {
#ifdef DEBUG
	if (!data) {
		pnm_error("Error: trying to acces NULLpointer by pnm_image::write().\n");
		throw std::length_error("Error: trying to acces NULLpointer by pnm_image::write().");
	}
#endif
	write_header(fd);
	int nsize = size();
	if (nsize && (type <= PNM_ASCII_PPM)) {
		for (int i = 0; i < nsize; i++)
			fprintf(fd, "%i ", data[i]); // We are printing spaces even ib PBM images.
	}
	if (nsize && (type >= PNM_BINARY_PBM)) {
		for (int i = 0; i < nsize; i++) {
			fprintf(fd, "%c", data[i]);
		}
	}
}

void pnm_image::convert(int new_type) {
	if (type == new_type) // Nothing to convert.
		return;

	auto dest = pnm_image(width, height, new_type);
	int new_size = dest.size();

	int convert_type = (type <= PNM_ASCII_PPM) ? type : type - 3;
	convert_type |= ((dest.type <= PNM_ASCII_PPM) ? dest.type : dest.type - 3) << 4;

	if (((convert_type&3) == PNM_ASCII_PGM) && (dest.type == PNM_BINARY_PBM)) { // Bianry PGM images are packed.
		for (int r = 0; r < height; r++) {
			for (int i = 0; i <= (width - 1)/8; i++) {
				dest.data[r*((width-1)/8+1) + i] = \
					(((i*8+0 >= width) || (data[r*width+i*8+0] > maxvalue/2)) ? 0x00 : 0x80) | \
					(((i*8+1 >= width) || (data[r*width+i*8+1] > maxvalue/2)) ? 0x00 : 0x40) | \
					(((i*8+2 >= width) || (data[r*width+i*8+2] > maxvalue/2)) ? 0x00 : 0x20) | \
					(((i*8+3 >= width) || (data[r*width+i*8+3] > maxvalue/2)) ? 0x00 : 0x10) | \
					(((i*8+4 >= width) || (data[r*width+i*8+4] > maxvalue/2)) ? 0x00 : 0x08) | \
					(((i*8+5 >= width) || (data[r*width+i*8+5] > maxvalue/2)) ? 0x00 : 0x04) | \
					(((i*8+6 >= width) || (data[r*width+i*8+6] > maxvalue/2)) ? 0x00 : 0x02) | \
					(((i*8+7 >= width) || (data[r*width+i*8+7] > maxvalue/2)) ? 0x00 : 0x01);
			}
		}
	}
	else if (((convert_type&3) == PNM_ASCII_PPM) && (dest.type == PNM_BINARY_PBM)) {
		for (int r = 0; r < height; r++) {
			for (int i = 0; i <= (width - 1)/8; i++) {
				dest.data[r*((width-1)/8+1) + i] = \
					(((i*8+0 >= width) || (data[(r*width+i*8+0)*3+0] + data[(r*width+i*8+0)*3+1] + data[(r*width+i*8+0)*3+2] > maxvalue*3/2)) ? 0x00 : 0x80) | \
					(((i*8+1 >= width) || (data[(r*width+i*8+1)*3+0] + data[(r*width+i*8+1)*3+1] + data[(r*width+i*8+1)*3+2] > maxvalue*3/2)) ? 0x00 : 0x40) | \
					(((i*8+2 >= width) || (data[(r*width+i*8+2)*3+0] + data[(r*width+i*8+2)*3+1] + data[(r*width+i*8+2)*3+2] > maxvalue*3/2)) ? 0x00 : 0x20) | \
					(((i*8+3 >= width) || (data[(r*width+i*8+3)*3+0] + data[(r*width+i*8+3)*3+1] + data[(r*width+i*8+3)*3+2] > maxvalue*3/2)) ? 0x00 : 0x10) | \
					(((i*8+4 >= width) || (data[(r*width+i*8+4)*3+0] + data[(r*width+i*8+4)*3+1] + data[(r*width+i*8+4)*3+2] > maxvalue*3/2)) ? 0x00 : 0x08) | \
					(((i*8+5 >= width) || (data[(r*width+i*8+5)*3+0] + data[(r*width+i*8+5)*3+1] + data[(r*width+i*8+5)*3+2] > maxvalue*3/2)) ? 0x00 : 0x04) | \
					(((i*8+6 >= width) || (data[(r*width+i*8+6)*3+0] + data[(r*width+i*8+6)*3+1] + data[(r*width+i*8+6)*3+2] > maxvalue*3/2)) ? 0x00 : 0x02) | \
					(((i*8+7 >= width) || (data[(r*width+i*8+7)*3+0] + data[(r*width+i*8+7)*3+1] + data[(r*width+i*8+7)*3+2] > maxvalue*3/2)) ? 0x00 : 0x01);
			}
		}
	}
	else if (type == PNM_BINARY_PBM) { // Unpacking of PGM images is unimplemented.
		pnm_error("Conversion from binary PBM is not implemented.\n");
		throw std::invalid_argument("Conversion from binary PBM is not implemented.");
	}
	else {
		switch (convert_type) {
			case (PNM_ASCII_PBM << 4) | PNM_ASCII_PBM: // Ascii to binary or vice versa.
			case (PNM_ASCII_PGM << 4) | PNM_ASCII_PGM:
			case (PNM_ASCII_PPM << 4) | PNM_ASCII_PPM:
				std::memcpy(dest.data, data, sizeof(pnm_data_t)*new_size);
				break;
			case (PNM_ASCII_PGM << 4) | PNM_ASCII_PBM: // Scale up from binary to grayscale.
				for (int i = 0; i < new_size; i++)
					dest.data[i] = dest.maxvalue * (1-data[i]);
				break;
			case (PNM_ASCII_PPM << 4) | PNM_ASCII_PBM: // Scale up from binary to color.
				for (int i = 0; i < new_size; i+=3) {
					dest.data[i+0] = dest.maxvalue * (1-data[i/3]);
					dest.data[i+1] = dest.maxvalue * (1-data[i/3]);
					dest.data[i+2] = dest.maxvalue * (1-data[i/3]);
				}
				break;
			case (PNM_ASCII_PBM << 4) | PNM_ASCII_PGM: // Threshold from grayscale to binary.
				for (int i = 0; i < new_size; i++)
					dest.data[i] = (data[i] >= 128) ? 0 : 1;
				break;
			case (PNM_ASCII_PPM << 4) | PNM_ASCII_PGM: // Copy from grayscale to color.
				for (int i = 0; i < new_size; i+=3) {
					dest.data[i+0] = data[i/3];
					dest.data[i+1] = data[i/3];
					dest.data[i+2] = data[i/3];
				}
				break;
			case (PNM_ASCII_PBM << 4) | PNM_ASCII_PPM: // Threshold from color to binary.
				for (int i = 0; i < new_size; i++)
					dest.data[i] = (data[i*3] + data[i*3 + 1] + data[i*3 + 2] >= 128*3) ? 0 : 1;
				break;
			case (PNM_ASCII_PGM << 4) | PNM_ASCII_PPM: // Average from color to grayscale.
				for (int i = 0; i < new_size; i++)
					dest.data[i] = (data[i*3] + data[i*3 + 1] + data[i*3 + 2]) / 3;
				break;
			default:
				pnm_error("Unknown conversion types (from %i, to %i).\n", type, dest.type);
				throw std::invalid_argument("Invalid conversion types.");
		}
	}
	*this = std::move(dest);
}

pnm_image::~pnm_image() {
	if (data)
		delete[] data;
}

void pnm_image::erase_image() {
	std::memset(data, 255, size()*sizeof(pnm_data_t));
}

int pnm_image::size() { // Calculate size for image storing.
	int size = 0;
	switch (type) {
		case PNM_ASCII_PBM: //(black/white)
			size = width * height;
			break;
		case PNM_BINARY_PBM: //(black/white)
			size = ((width - 1) / 8 + 1) * height; // Binary PBM images have 8 pixels packed in one byte. Bits at the end of each row are unused.
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

int pnm_image::guess_maxvalue() { // Calculate usual maxvalue.
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

pnm_image &pnm_image::operator=(pnm_image &&move) { // Move image.
	width = move.width;
	height = move.height;
	type = move.type;
	maxvalue = move.maxvalue;
	if (data)
		delete[] data;
	data = move.data;
	move.data = NULL;
}

}; // namespace
