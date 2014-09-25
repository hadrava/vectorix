#include "svg_handler.h"
#include "pnm_handler.h"
#include "vectorizer.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <cv.h>
#include <highgui.h>

#define MORPH_CROSS 1

void vectorizer_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	return;
}

struct svg_line *create_bezier(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3) {
	struct svg_line *ret  = malloc(sizeof(struct svg_line));
	ret->width = 4.0;
	ret->opacity = 1;
	ret->x0 = x0;
	ret->y0 = y0;
	ret->x1 = x1;
	ret->y1 = y1;
	ret->x2 = x2;
	ret->y2 = y2;
	ret->x3 = x3;
	ret->y3 = y3;
	ret->next = NULL;
	return ret;
}

struct svg_image *vectorize_bare(const struct pnm_image * image) {
	struct svg_image * vect = malloc(sizeof(const struct svg_image));
	vect->width = image->width;
	vect->height = image->height;
	vect->data = create_bezier(0, 0, image->width/2, image->height/2, image->width, image->height/2, image->width, image->height);
	return vect;
}

struct svg_image *vectorize(const struct pnm_image * image) {
	struct svg_image * vect = malloc(sizeof(const struct svg_image));
	vect->width = image->width;
	vect->height = image->height;
	//vect->data = NULL;

	IplImage* source = cvCreateImage(cvSize(image->width, image->height), 8, 1);
	IplImage* bw = cvCreateImage(cvSize(image->width, image->height), 8, 1);
	IplImage* tmp = cvCreateImage(cvSize(image->width, image->height), 8, 1);
	IplImage* out = cvCreateImage(cvSize(image->width, image->height), 8, 1);
	IplImage* col = cvCreateImage(cvSize(image->width, image->height), 8, 3);
	for (int j = 0; j<image->height; j++) {
		for (int i = 0; i<image->width; i++) {
			source->imageData[i+j*source->widthStep] = 255 - image->data[i+j*image->width];
		}
	}
	cvShowImage("source", source);
	cvWaitKey(0);
	cvThreshold(source, source, 127, 255, CV_THRESH_BINARY);
	IplConvKernel* kernel = cvCreateStructuringElementEx(3,3, 1,1,MORPH_CROSS, NULL);

	double max = 1;
	while (max !=0) {
		cvMorphologyEx(source, bw, tmp, kernel, CV_MOP_OPEN, 1);
		cvNot(bw, bw);
		cvAnd(source, bw, bw, NULL);
		cvOr(out, bw, out, NULL);
		cvErode(source, source, kernel, 1);
		cvMinMaxLoc(source, NULL, &max, NULL, NULL, NULL);
	}

	cvShowImage("source", out);
	cvWaitKey(0);

	CvMemStorage* storage = cvCreateMemStorage(0);

	CvSeq *hough = cvHoughLines2( out, storage, CV_HOUGH_PROBABILISTIC, 1, CV_PI/180, 1, 15, 10 );



	struct svg_line *last_bezier = NULL;
	for(int i = 0; i < hough->total; i++ )
	{
		CvPoint* line = (CvPoint*)cvGetSeqElem(hough,i);
		cvLine( col, line[0], line[1], CV_RGB(255,0,0), 3, CV_AA, 0 );
		struct svg_line *new_bezier = create_bezier(line[0].x, line[0].y, line[0].x, line[0].y, line[1].x, line[1].y, line[1].x, line[1].y);
		new_bezier->next = last_bezier;
		last_bezier = new_bezier;
	}
	vect->data = last_bezier;
	cvShowImage("source", col);
	cvWaitKey(0);

	cvReleaseMemStorage(&storage);
	cvReleaseImage(&source);

	return vect;
}

void render(struct pnm_image *bitmap, const struct svg_image *vector) {
	pnm_erase_image(bitmap);
	struct svg_line * line = vector->data;
	while (line) {
		bezier_render(bitmap, line);
		line = line->next;
	}
}

void bezier_render(struct pnm_image * image, const struct svg_line *line) {
	if (image->type != PNM_BINARY_PGM)
		vectorizer_error("Error: Image type %i not supported.\n", image->type);
	for (float u=0; u<=1; u+=0.005) {
		float v = 1-u;
		float x = v*v*v*line->x0 + 3*v*v*u*line->x1 + 3*v*u*u*line->x2 + u*u*u*line->x3;
		float y = v*v*v*line->y0 + 3*v*v*u*line->y1 + 3*v*u*u*line->y2 + u*u*u*line->y3;
		for (int j = y - line->width/2; j<= y + line->width/2; j++) {
			if (j<0 || j>=image->height)
				continue;
			for (int i = x - line->width/2; i<= x + line->width/2; i++) {
				if (i<0 || i>=image->width)
					continue;
				if ((x-i)*(x-i) + (y-j)*(y-j) <= line->width*line->width/4)
					image->data[i+j*image->width] = 0;
			}
		}
	}
}
