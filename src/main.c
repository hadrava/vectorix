#include <stdio.h>
#include "pnm_handler.h"

int main(int argc, char **argv) {
    if (argc == 1)
        pnm_read(stdin);
    else {
        FILE *input = fopen(argv[1], "r");
	pnm_read(input);
	fclose(input);
    }
}
