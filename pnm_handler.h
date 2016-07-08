#ifndef VECTORIX__PNM_HANDLER_H
#define VECTORIX__PNM_HANDLER_H

// Manipulation with Netpbm format images

#include <cstdio>
#include <cstring>
#include "logger.h"
#include "config.h"

namespace vectorix {

typedef unsigned char pnm_data_t; // PNM image data type for one pixel (1 channel)

enum class pnm_variant_type {
	ascii_pbm  = 1, // ASCII bitmap
	ascii_pgm  = 2, // ASCII grayscale image
	ascii_ppm  = 3, // ASCII 3channel image
	binary_pbm = 4, // binary bitmap
	binary_pgm = 5, // binary grayscale image
	binary_ppm = 6  // binary 3channel image
};

class pnm_image {
public:
	pnm_image(): width(0), height(0), type(pnm_variant_type::ascii_pbm), maxvalue(1), data(NULL), log(VECTORIX_PNM_VERBOSITY) {}; // empty image
	pnm_image(FILE *fd): log(VECTORIX_PNM_VERBOSITY) { read(fd); }; // load image from open filedescriptor
	pnm_image(int _width, int _height, pnm_variant_type _type=pnm_variant_type::binary_pgm): width(_width), height(_height), type(_type), log(VECTORIX_PNM_VERBOSITY) { // create empty image with given image size and type
		maxvalue = guess_maxvalue();
		data = new pnm_data_t[size()];
	};
	pnm_image(const pnm_image & copy): width(copy.width), height(copy.height), type(copy.type), maxvalue(copy.maxvalue), log(VECTORIX_PNM_VERBOSITY) { // copy constructor
		data = new pnm_data_t[size()];
		std::memcpy(data, copy.data, sizeof(pnm_data_t) * size());
	};
	~pnm_image();
	void read(FILE *fd);
	void write(FILE *fd);
	void convert(pnm_variant_type new_type); // Convert between two image types
	void erase_image(); // Fill image with white
	int width;
	int height;
	pnm_variant_type type;
	pnm_data_t maxvalue; // Maximal value of one pixel
	pnm_data_t *data; // Raw data
	pnm_image(pnm_image &&) = default;
	pnm_image &operator=(pnm_image &&move);
private:
	int fscanf_comment(FILE *stream, const char *format, ...); // Read line from filedescriptor, ignore comments (#)
	bool read_header(FILE *fd); // Parse header
	void write_header(FILE *fd); // Write image header
	int size(); // Calculate buffersize for new data (given type and image dimensions)
	int guess_maxvalue(); // Get common maxvalue for image type (1 for bitmap, 255 for others)

	logger log;
};

}; // namespace

#endif
