all: vectorix

COMP=g++
L_FLAGS=

# clang is not fully supported, use at your own risk
#COMP=clang
#L_FLAGS=-lstdc++

OPENCV_LIBRARY2 = opencv_core opencv_imgproc opencv_highgui
OPENCV_LIBRARY3 = opencv_core opencv_imgproc opencv_highgui opencv_imgcodecs

L_OPENCV_SYSTEM := `pkg-config --libs --cflags opencv`
L_OPENCV_2.4.11 := -Wl,-rpath,$(abspath opencv/lib-2.4.11) -L $(abspath opencv/lib-2.4.11) $(patsubst %, opencv/lib-2.4.11/lib%.so, ${OPENCV_LIBRARY2}) $(patsubst %, -l%, ${OPENCV_LIBRARY2})
L_OPENCV_3.0.0 := -Wl,-rpath,$(abspath opencv/lib-3.0.0) -L $(abspath opencv/lib-3.0.0) $(patsubst %, opencv/lib-3.0.0/lib%.so, ${OPENCV_LIBRARY3}) $(patsubst %, -l%, ${OPENCV_LIBRARY3})

C_OPENCV_SYSTEM := `pkg-config --cflags opencv`
C_OPENCV_2.4.11 := -I opencv/include-2.4.11
C_OPENCV_3.0.0 := -I opencv/include-3.0.0

# select OpenCV library version
L_OPENCV=${L_OPENCV_3.0.0}
C_OPENCV=${C_OPENCV_3.0.0}

OBJS = main.o v_image.o export_svg.o pnm_handler.o vectorizer.o render.o potrace_handler.o custom_vectorizer.o opencv_render.o parameters.o export_ps.o geom.o offset.o least_squares.o
DEBUG = -ggdb

vectorix: ${OBJS}
	${COMP} $^ -o $@ ${L_OPENCV} -L potrace/lib/ -lpotrace -lm ${L_FLAGS}

%.o: %.cpp
	${COMP} -c -o $@ $< ${C_OPENCV} -std=c++11 ${DEBUG}

clean:
	rm -f vectorix ${OBJS}

remake: clean all

vectorix.tar.bz2:
	git archive master | bzip2 > $@

.PHONY: all clean remake vectorix.tar.bz2
