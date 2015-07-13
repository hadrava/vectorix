all: main

#COMP=clang
COMP=g++

main: main.o v_image.o export_svg.o pnm_handler.o vectorizer.o render.o potrace_handler.o
	${COMP} $^ -o $@ `pkg-config --libs --cflags opencv` -L potrace/lib/ -lpotrace -lm

%.o: %.cpp
	${COMP} -c -o $@ $< -std=c++11 -ggdb -DDEBUG

clean:
	rm -f main *.o

remake: clean all

.PHONY: all clean remake
