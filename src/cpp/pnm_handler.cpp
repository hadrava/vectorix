#include <utility>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <stdexcept>
#include "pnm_handler.h"

// Manipulation with Netpbm format images

namespace pnm {

#define MAX_LINE_LENGTH	512 // Maximal length of image line in header (extra long comments are not allowed) TODO zařídit, aby to nebyla pravda

void pnm_image::pnm_error(const char *format, ...) { // Write error to stderr
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

int pnm_image::fscanf_comment(FILE *stream, const char *format, ...) { // Read line from PNM header, ignore comments
	char line[MAX_LINE_LENGTH];
	for (;;) {
		fgets(line, MAX_LINE_LENGTH, stream); // Read max one line
		if (!line) // TODO change to (!line[0])
			return 0;
		if (line[0] != '#') // Line starting with '#' is commented and should be skipped
			break;
		// TODO opravit strlen
	}
	va_list args;
	va_start(args, format);
	int ret = vsscanf(line, format, args);
	va_end(args);
	return ret;
}

bool pnm_image::read_header(FILE *fd) { // Read image header and clear old data.
	char magic, ntype;
	if (fscanf_comment(fd, "%c%c\n", &magic, &ntype) != 2 || magic != 'P' || ntype < '1' || ntype > '6') { // First line contains magicvalue P and type (1-6)
		pnm_error("Error reading image header.\n");
		return false;
	}
	if (fscanf_comment(fd, "%i%i\n", &width, &height) != 2) { // Image dimensions width and height
		pnm_error("Error reading image dimensions.\n");
		return false;
	}
	if (ntype != '1' && ntype != '4') { // Everything except bitmap (0/1) images
		if (fscanf_comment(fd, "%i\n", &maxvalue) != 1) { // Maximal value of pixel
			pnm_error("Error reading image maxvalue.\n");
			return false;
		}
	}
	type = ntype - '0'; // Convert type to int
	if (data)
		delete[] data; // Drop old data
	data = NULL;
	return true;
}

void pnm_image::write_header(FILE *fd) { // Write image header
	fprintf(fd, "P%c\n", type + '0'); // Image type
	fprintf(fd, "%i %i\n", width, height); // Dimensions
	if (type != 1 && type != 4) {
		fprintf(fd, "%i\n", maxvalue); // Maxvalue is only for non-bitmap images
	}
}

void pnm_image::read(FILE *fd) { // Read image from file
	if (!read_header(fd))
		throw std::underflow_error("Unable to read image header.");

	int nsize = size(); // Calculate size for data
#ifdef PNM_DEBUG
	write_header(stderr);
#endif

	if (nsize) {
		char scanf_string[8];
		if (type == PNM_ASCII_PBM)
			strcpy(scanf_string, "%1i"); // Sometimes there are no spaces between numbers
		else if (type <= PNM_ASCII_PPM)
			strcpy(scanf_string, "%i"); // Whitespace between numbers are mandatory
		else if (type >= PNM_BINARY_PBM)
			strcpy(scanf_string, "%c"); // Binary data

		data = new pnm_data_t [nsize]; // Alocate buffer
		for (int i = 0; i < nsize; i++) {
			int read;
			if (fscanf(fd, scanf_string, &read) != 1) { // Read image data
				pnm_error("Error: reading image data failed at position %i.\n", i);
				throw std::underflow_error("Unable to read image data.");
			}
			data[i] = read;
		}
	}
}

void pnm_image::write(FILE *fd) { // Save whole image
#ifdef DEBUG
	if (!data) { // In debug mode refuses to write empty image
		pnm_error("Error: trying to acces NULLpointer by pnm_image::write().\n");
		throw std::length_error("Error: trying to acces NULLpointer by pnm_image::write().");
	}
#endif
	write_header(fd);
	int nsize = size();
	if (nsize && (type <= PNM_ASCII_PPM)) { // All ASCII images
		for (int i = 0; i < nsize; i++)
			fprintf(fd, "%i ", data[i]); // We are printing spaces in ASCII_PBM images even it is not necesary
	}
	if (nsize && (type >= PNM_BINARY_PBM)) { // All binary images
		for (int i = 0; i < nsize; i++) {
			fprintf(fd, "%c", data[i]);
		}
	}
}

void pnm_image::convert(int new_type) {
	if (type == new_type) // Nothing to convert, we are already there
		return;

	auto dest = pnm_image(width, height, new_type);
	int new_size = dest.size();

	int convert_type = (type <= PNM_ASCII_PPM) ? type : type - 3; // Forget about binary/ascii
	convert_type |= ((dest.type <= PNM_ASCII_PPM) ? dest.type : dest.type - 3) << 4; // lower two bits: original type; upper two bits: destination type

	if (((convert_type&3) == PNM_ASCII_PGM) && (dest.type == PNM_BINARY_PBM)) { // Convert (ascii/binary) grayscale to binary bitmap image //TODO add ascii PBM
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
	else if (((convert_type&3) == PNM_ASCII_PPM) && (dest.type == PNM_BINARY_PBM)) { // Convert (ascii/binary) color image to binary bitmap image
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
	else if (type == PNM_BINARY_PBM) { // Unpacking of PGM images is unimplemented (well, it is not needed) TODO implement unefficient by converting to grayscale.
		pnm_error("Conversion from binary PBM is not implemented.\n");
		throw std::invalid_argument("Conversion from binary PBM is not implemented.");
	}
	else {
		switch (convert_type) {
			case (PNM_ASCII_PBM << 4) | PNM_ASCII_PBM: // TODO should not we delete it?
			case (PNM_ASCII_PGM << 4) | PNM_ASCII_PGM: // Same type to same type, only change binary to ascii or vice versa
			case (PNM_ASCII_PPM << 4) | PNM_ASCII_PPM:
				std::memcpy(dest.data, data, sizeof(pnm_data_t)*new_size);
				break;
			case (PNM_ASCII_PGM << 4) | PNM_ASCII_PBM: // Scale up from binary to grayscale
				for (int i = 0; i < new_size; i++)
					dest.data[i] = dest.maxvalue * (1-data[i]);
				break;
			case (PNM_ASCII_PPM << 4) | PNM_ASCII_PBM: // Scale up from binary to color
				for (int i = 0; i < new_size; i+=3) {
					dest.data[i+0] = dest.maxvalue * (1-data[i/3]);
					dest.data[i+1] = dest.maxvalue * (1-data[i/3]);
					dest.data[i+2] = dest.maxvalue * (1-data[i/3]);
				}
				break;
			case (PNM_ASCII_PBM << 4) | PNM_ASCII_PGM: // Threshold from grayscale to binary
				for (int i = 0; i < new_size; i++)
					dest.data[i] = (data[i] >= 128) ? 0 : 1;
				break;
			case (PNM_ASCII_PPM << 4) | PNM_ASCII_PGM: // Copy from grayscale to color
				for (int i = 0; i < new_size; i+=3) {
					dest.data[i+0] = data[i/3];
					dest.data[i+1] = data[i/3];
					dest.data[i+2] = data[i/3];
				}
				break;
			case (PNM_ASCII_PBM << 4) | PNM_ASCII_PPM: // Threshold from color to binary
				for (int i = 0; i < new_size; i++)
					dest.data[i] = (data[i*3] + data[i*3 + 1] + data[i*3 + 2] >= 128*3) ? 0 : 1;
				break;
			case (PNM_ASCII_PGM << 4) | PNM_ASCII_PPM: // Average from color to grayscale
				for (int i = 0; i < new_size; i++)
					dest.data[i] = (data[i*3] + data[i*3 + 1] + data[i*3 + 2]) / 3;
				break;
			default:
				pnm_error("Unknown conversion types (from %i, to %i).\n", type, dest.type); // This is not going to happen if datastructures are ok
				throw std::invalid_argument("Invalid conversion types.");
		}
	}
	*this = std::move(dest);
}

pnm_image::~pnm_image() {
	if (data) // Non-empty image
		delete[] data;
}

void pnm_image::erase_image() {
	std::memset(data, 255, size()*sizeof(pnm_data_t)); // Set everything to white
}

int pnm_image::size() { // Calculate size for image storing
	int size = 0;
	switch (type) {
		case PNM_ASCII_PBM: // bitmap (black/white)
			size = width * height;
			break;
		case PNM_BINARY_PBM: // bitmap (black/white)
			size = ((width - 1) / 8 + 1) * height; // Binary PBM images has 8 pixels packed in one byte. Bits at the end of each row are unused
			break;
		case PNM_ASCII_PGM: // grayscale
		case PNM_BINARY_PGM: // grayscale
			size = width * height;
			break;
		case PNM_ASCII_PPM: // RGB
		case PNM_BINARY_PPM: // RGB
			size = width * height * 3;
			break;
	}
#ifdef PNM_DEBUG
	pnm_error("Debug: size = %i\n", size);
#endif
	return size;
}

int pnm_image::guess_maxvalue() { // Return usual maxvalue for given imagetype
	int maxvalue = 0;
	switch (type) {
		case PNM_ASCII_PBM: // Binary bitmap
		case PNM_BINARY_PBM:
			maxvalue = 1;
			break;
		default:
			maxvalue = 255;
	}
	return maxvalue;
}

pnm_image &pnm_image::operator=(pnm_image &&move) { // Move image
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
