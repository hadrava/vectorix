all: main

main: main.o pnm_handler.o svg_handler.o vectorizer.o

%.o: %.c
	cc -c -o $@ $< -std=c99 -DDEBUG -ggdb

clean:
	rm -f main *.o
