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
#include "init.h"
#include "defaults.h"
#include "blocks.h"

void help() {
	printf ("Command was help\n");
}

int main(int argc, const char *argv[]) {
	if (cmd_parse(argc, argv) != 0) {
		return EXIT_FAILURE;
	}

	if (cmd_match("help")) {
		help();
	}
	else if (cmd_match("init")) {
		const char *device = cmd_get_value("dev");
		const char *bs_string = cmd_get_value("bs");
		const char *hpad_string = cmd_get_value("hpad");
		const char *padchar_string = cmd_get_value("pad");

		long int blocksize = BDL_DEFAULT_BLOCKSIZE;
		long int header_pad = BDL_DEFAULT_HEADER_PAD;
		char pad_char = BDL_DEFAULT_PAD_CHAR;

		if (device == NULL) {
			fprintf(stderr, "Error: Argument dev was not set, specify with dev=DEVICE\n");
			return EXIT_FAILURE;
		}

		// Parse block size argument
		if (bs_string != NULL) {
			if (cmd_convert_integer_10("bs")) {
				fprintf(stderr, "Error: Could not interpret block size argument, use bs=NUMBER\n");
				return EXIT_FAILURE;
			}

			blocksize = cmd_get_integer("bs");

			if (blocksize < BDL_MINIMUM_BLOCKSIZE) {
				fprintf(stderr, "Error: Blocksize was too small, minimum is %i\n", BDL_MINIMUM_BLOCKSIZE);
				return EXIT_FAILURE;
			}
			if (blocksize % BDL_BLOCKSIZE_DIVISOR != 0) {
				fprintf(stderr, "Error: Blocksize needs to be dividable by %i\n", BDL_BLOCKSIZE_DIVISOR);
				return EXIT_FAILURE;
			}
		}

		// Parse header pad argument
		if (hpad_string != NULL) {
			if (cmd_convert_integer_10("hpad")) {
				fprintf(stderr, "Error: Could not interpret header pad size argument, use hpad=NUMBER\n");
				return EXIT_FAILURE;
			}

			header_pad = cmd_get_integer("hpad");

			if (header_pad < BDL_MINIMUM_HEADER_PAD) {
				fprintf(stderr, "Error: Header pad was too small, minimum is %i\n", BDL_MINIMUM_HEADER_PAD);
				return EXIT_FAILURE;
			}
			if (header_pad % BDL_HEADER_PAD_DIVISOR != 0) {
				fprintf(stderr, "Error: Header pad needs to be dividable by %i\n", BDL_HEADER_PAD_DIVISOR);
				return EXIT_FAILURE;
			}
		}

		// Parse default pad char argument
		if (cmd_check_all_args_used()) {
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
