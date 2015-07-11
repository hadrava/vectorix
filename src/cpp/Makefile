all: main

#COMP=clang
COMP=g++

main: main.o lines.o export_svg.o pnm_handler.o vectorizer.o render.o
	${COMP} $^ -o $@ `pkg-config --libs --cflags opencv`

%.o: %.cpp
	${COMP} -c -o $@ $< -std=c++11 -ggdb -DDEBUG -DPNM_DEBUG

clean:
	rm -f main *.o
