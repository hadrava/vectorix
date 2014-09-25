all: main

main: main.o pnm_handler.o svg_handler.o vectorizer.o
	cc $^ -o $@ -lopencv_highgui -lopencv_core -lopencv_imgproc -lm

%.o: %.c
	cc -c -o $@ $< -std=c99 -DDEBUG -ggdb -I/usr/include/opencv/

clean:
	rm -f main *.o
