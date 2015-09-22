#!/bin/bash
dir=`mktemp -d`
make clean
make dokumentace.pdf
rm -rf vector.tar.gz

mkdir -p "$dir/vector/"
cp \
config.h \
custom_vectorizer.cpp \
custom_vectorizer.h \
export_ps.cpp \
export_ps.h \
export_svg.cpp \
export_svg.h \
main.cpp \
opencv_render.cpp \
opencv_render.h \
parameters.cpp \
parameters.h \
pnm_handler.cpp \
pnm_handler.h \
potrace_handler.cpp \
potrace_handler.h \
render.cpp \
render.h \
time_measurement.h \
vectorizer.cpp \
vectorizer.h \
v_image.cpp \
v_image.h \
Makefile \
doc.txt \
dokumentace.pdf \
"$dir/vector/"

mkdir -p "$dir/vector/opencv/"
cp \
opencv/Makefile \
"$dir/vector/opencv/"

mkdir -p "$dir/vector/potrace/"
cp \
potrace/Makefile \
potrace/README.txt \
"$dir/vector/potrace/"

tar -czf vector.tar.gz -C "$dir" ./

rm -rf "$dir"
