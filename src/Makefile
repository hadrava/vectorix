all: main

main: main.o pnm_handler.o

%.o: %.c
	cc -c -o $@ $< -std=c99 -DPNM_DEBUG -DDEBUG

clean:
	rm -f main *.o
