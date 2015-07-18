all: vector

#COMP=clang
COMP=g++

vector: main.o v_image.o export_svg.o pnm_handler.o vectorizer.o render.o potrace_handler.o custom_vectorizer.o opencv_render.o
	${COMP} $^ -o $@ `pkg-config --libs --cflags opencv` -L potrace/lib/ -lpotrace -lm

%.o: %.cpp
	${COMP} -c -o $@ $< -std=c++11 -ggdb

doc.ps: doc.txt
	paps --font="Monospace 10" $< >$@

doc.pdf: doc.ps
	ps2pdf $<

clean:
	rm -f vector *.o doc.ps doc.pdf

remake: clean all

.PHONY: all clean remake
