/*
 ============================================================================
 Name        : bdl.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "cmdline.h"

int main(int argc, const char *argv[]) {
	if (cmd_match("help")) {
		printf ("Command was help\n");
	}

	return EXIT_SUCCESS;
}
