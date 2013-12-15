all: main

main: main.o pnm_handler.o

%.o: %.c
	cc -c -o $@ $<

clean:
	rm -f main *.o
