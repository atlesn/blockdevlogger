/*

Block Device Loggeer

Copyright (C) 2018 Atle Solbakken atle@goliathdns.no

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "init.h"
#include "defaults.h"
#include "blocks.h"
#include "write.h"

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
	else if (cmd_match("write")) {
		const char *device = cmd_get_argument(0);
		const char *data = cmd_get_argument(1);

		if (device == NULL) {
			fprintf(stderr, "Error: Device argument was missing for write command, use 'write DEVICE DATA'\n");
			return EXIT_FAILURE;
		}
		if (data == NULL) {
			fprintf(stderr, "Error: Data argument was missing for write command, use 'write DEVICE DATA'\n");
			return EXIT_FAILURE;
		}

		// Check that the user hasn't specified anything funny at the command line
		if (cmd_check_all_args_used()) {
			return EXIT_FAILURE;
		}

		if (write_put_data(device, data, strlen(data)+1)) {
			return EXIT_FAILURE;
		}
	}
	else if (cmd_match("init")) {
		const char *device = cmd_get_value("dev");
		const char *bs_string = cmd_get_value("bs");
		const char *hpad_string = cmd_get_value("hpad");
		const char *padchar_string = cmd_get_value("padchar");

		long int blocksize = BDL_DEFAULT_BLOCKSIZE;
		long int header_pad = BDL_DEFAULT_HEADER_PAD;
		char padchar = BDL_DEFAULT_PAD_CHAR;

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

			if (blocksize > BDL_MAXIMUM_BLOCKSIZE) {
				fprintf(stderr, "Error: Blocksize was too large, maximum is %i\n", BDL_MAXIMUM_BLOCKSIZE);
				return EXIT_FAILURE;
			}
			else if (blocksize < BDL_MINIMUM_BLOCKSIZE) {
				fprintf(stderr, "Error: Blocksize was too small, minimum is %i\n", BDL_MINIMUM_BLOCKSIZE);
				return EXIT_FAILURE;
			}
			else if (blocksize % BDL_BLOCKSIZE_DIVISOR != 0) {
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

		// Parse pad characher argument
		if (padchar_string != NULL) {
			if (cmd_convert_hex_16("padchar")) {
				fprintf(stderr, "Error: Could not interpret pad charachter argument, use padchar=HEXNUMBER 1 BYTE\n");
				return EXIT_FAILURE;
			}

			padchar = cmd_get_hex("padchar");
		}

		// Check that the user hasn't specified anything funny at the command line
		if (cmd_check_all_args_used()) {
			return EXIT_FAILURE;
		}

		if (init_dev(device, blocksize, header_pad, padchar)) {
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
