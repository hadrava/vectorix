#ifndef _PNM_HANDLER_H
#define _PNM_HANDLER_H

#include <stdio.h>
#include <string.h>

namespace pnm {

const int PNM_ASCII_PBM  = 1;
const int PNM_ASCII_PGM  = 2;
const int PNM_ASCII_PPM  = 3;
const int PNM_BINARY_PBM = 4;
const int PNM_BINARY_PGM = 5;
const int PNM_BINARY_PPM = 6;

class pnm_image {
public:
	pnm_image(): width(0), height(0), type(1), maxvalue(1), data(NULL) {};
	pnm_image(FILE *fd) { read(fd); };
	pnm_image(int _width, int _height, int _type=PNM_BINARY_PGM): width(_width), height(_height), type(_type) {
		maxvalue = guess_maxvalue();
		data = new int[size()];
	};
	pnm_image(const pnm_image & copy): width(copy.width), height(copy.height), type(copy.type), maxvalue(copy.maxvalue) {
		data = new int[size()];
		memcpy(data, copy.data, sizeof(int) * size());
	};
	~pnm_image();
	void read(FILE *fd);
	void write(FILE *fd);
	void convert(int new_type);
	void erase_image();
	int width;
	int height;
	int type;
	int maxvalue;
	int *data;
	pnm_image(pnm_image &&) = default;
	pnm_image &operator=(pnm_image &&move);
private:
	void pnm_error(const char *format, ...);
	int fscanf_comment(FILE *stream, const char *format, ...);
	void read_header(FILE *fd);
	void write_header(FILE *fd);
	int size();
	int guess_maxvalue();
};

};

#endif
