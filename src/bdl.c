/*

Block Device Logger

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
#include "io.h"
#include "validate.h"
#include "session.h"
#include "read.h"

void help() {
	printf ("Command was help\n");
}

int interpret_command (struct session *session, int argc, const char *argv[]) {
	if (cmd_parse(&session->cmd_data, argc, argv) != 0) {
		return 1;
	}

	if (argc == 1 || cmd_match(&session->cmd_data, "help")) {
		help();
	}
	else if (cmd_match(&session->cmd_data, "open")) {
		// Start a session, and write commands line by line (from STDIN)
		const char *device_path = cmd_get_value(&session->cmd_data, "dev");

		if (device_path == NULL) {
			fprintf(stderr, "Error: Device argument was missing for open command, use 'open dev=DEVICE'\n");
			return 1;
		}

		if (cmd_check_all_args_used(&session->cmd_data)) {
			return 1;
		}

		if (session->usercount != 0) {
			fprintf (stderr, "Error: A device was already open\n");
			return 1;
		}

		if (start_session(session, device_path) != 0) {
			fprintf (stderr, "Error while opening device for session use\n");
			return 1;
		}
	}
	else if (cmd_match(&session->cmd_data, "close")) {
		if (session->usercount == 0) {
			fprintf (stderr, "Attempted to close while no device was open\n");
			return 1;
		}

		close_session(session);

	}
	else if (cmd_match(&session->cmd_data, "validate")) {
		const char *device_string = cmd_get_value(&session->cmd_data, "dev");


		if (cmd_check_all_args_used(&session->cmd_data)) {
			return 1;
		}

		if (start_session (session, device_string) != 0) {
			fprintf (stderr, "Could not start session for validate command\n");
			return 1;
		}

		int result;
		if (validate_dev(&session->device, &result) != 0) {
			fprintf (stderr, "Error while validating device\n");
			close_session(session);
			return 1;
		}

		if (result == 0) {
			printf ("Device was valid\n");
		}
		else {
			printf ("Device was not valid, must be initialized\n");
		}

		close_session(session);
	}
	else if (cmd_match(&session->cmd_data, "read")) {
		const char *device_string = cmd_get_value(&session->cmd_data, "dev");
		const char *timestamp_gteq_string = cmd_get_value(&session->cmd_data, "ts_gteq");

		uint64_t timestamp_gteq = 0;

		if (timestamp_gteq_string != NULL) {
			if (cmd_convert_uint64_10(&session->cmd_data, "ts_gteq")) {
				fprintf(stderr, "Error: Could not interpret timestamp greater or equal argument, use ts_gteq=POSITIVE INTEGER\n");
				return 1;
			}
			timestamp_gteq = cmd_get_integer(&session->cmd_data, "ts_gteq");
			if (timestamp_gteq < 0) {
				fprintf(stderr, "Error: Timestamp greater or equal argument must be zero or greater, %lu was gives\n", timestamp_gteq);
				return 1;
			}
		}

		if (start_session (session, device_string) != 0) {
			fprintf (stderr, "Could not start session for read command\n");
			return 1;
		}

		if (read_blocks(&session->device, timestamp_gteq) != 0) {
			fprintf (stderr, "Error while reading blocks\n");
			close_session(session);
			return 1;
		}

		close_session(session);
	}
	else if (cmd_match(&session->cmd_data, "write")) {
		const char *device_string = cmd_get_value(&session->cmd_data, "dev");
		const char *appdata_string = cmd_get_value(&session->cmd_data, "appdata");
		const char *data = cmd_get_last_argument(&session->cmd_data);

		uint64_t appdata = 0;

		if (data == NULL || *data == '\0') {
			fprintf(stderr, "Error: Data argument was missing for write command, use 'write dev=DEVICE [...] DATA'\n");
			return 1;
		}
		if (appdata_string != NULL) {
			if (cmd_convert_hex_64(&session->cmd_data, "appdata")) {
				fprintf(stderr, "Error: Could not interpret pad charachter argument, use padchar=HEXNUMBER 1 BYTE\n");
				return 1;
			}

			appdata = cmd_get_hex_64(&session->cmd_data, "appdata");
		}

		// Check that the user hasn't specified anything funny at the command line
		if (cmd_check_all_args_used(&session->cmd_data)) {
			return 1;
		}

		if (start_session(session, device_string) != 0) {
			fprintf (stderr, "Could not start session for write command\n");
			return 1;
		}

		if (write_put_data(&session->device, data, strlen(data)+1, appdata) != 0) {
			close_session(session);
			return 1;
		}

		close_session(session);
	}
	else if (cmd_match(&session->cmd_data, "init")) {
		const char *device_string = cmd_get_value(&session->cmd_data, "dev");
		const char *bs_string = cmd_get_value(&session->cmd_data, "bs");
		const char *hpad_string = cmd_get_value(&session->cmd_data, "hpad");
		const char *padchar_string = cmd_get_value(&session->cmd_data, "padchar");

		long int blocksize = BDL_DEFAULT_BLOCKSIZE;
		long int header_pad = BDL_DEFAULT_HEADER_PAD;
		char padchar = BDL_DEFAULT_PAD_CHAR;


		// Parse block size argument
		if (bs_string != NULL) {
			if (cmd_convert_integer_10(&session->cmd_data, "bs")) {
				fprintf(stderr, "Error: Could not interpret block size argument, use bs=NUMBER\n");
				return 1;
			}

			blocksize = cmd_get_integer(&session->cmd_data, "bs");

			if (blocksize > BDL_MAXIMUM_BLOCKSIZE) {
				fprintf(stderr, "Error: Blocksize was too large, maximum is %i\n", BDL_MAXIMUM_BLOCKSIZE);
				return 1;
			}
			else if (blocksize < BDL_MINIMUM_BLOCKSIZE) {
				fprintf(stderr, "Error: Blocksize was too small, minimum is %i\n", BDL_MINIMUM_BLOCKSIZE);
				return 1;
			}
			else if (blocksize % BDL_BLOCKSIZE_DIVISOR != 0) {
				fprintf(stderr, "Error: Blocksize needs to be dividable by %i\n", BDL_BLOCKSIZE_DIVISOR);
				return 1;
			}
		}

		// Parse header pad argument
		if (hpad_string != NULL) {
			if (cmd_convert_integer_10(&session->cmd_data, "hpad")) {
				fprintf(stderr, "Error: Could not interpret header pad size argument, use hpad=NUMBER\n");
				return 1;
			}

			header_pad = cmd_get_integer(&session->cmd_data, "hpad");

			if (header_pad < BDL_MINIMUM_HEADER_PAD) {
				fprintf(stderr, "Error: Header pad was too small, minimum is %i\n", BDL_MINIMUM_HEADER_PAD);
				return 1;
			}
			if (header_pad % BDL_HEADER_PAD_DIVISOR != 0) {
				fprintf(stderr, "Error: Header pad needs to be dividable by %i\n", BDL_HEADER_PAD_DIVISOR);
				return 1;
			}
		}

		// Parse pad characher argument
		if (padchar_string != NULL) {
			if (cmd_convert_hex_byte(&session->cmd_data, "padchar")) {
				fprintf(stderr, "Error: Could not interpret pad charachter argument, use padchar=HEXNUMBER 1 BYTE\n");
				return 1;
			}

			padchar = cmd_get_hex_byte(&session->cmd_data, "padchar");
		}

		// Check that the user hasn't specified anything funny at the command line
		if (cmd_check_all_args_used(&session->cmd_data)) {
			return 1;
		}

		if (start_session (session, device_string) != 0) {
			fprintf (stderr, "Error while starting session for init command\n");
			return 1;
		}

		if (init_dev(&session->device, blocksize, header_pad, padchar)) {
			fprintf (stderr, "Device intialization failed\n");
			close_session(session);
			return 1;
		}
		close_session(session);
	}
	else {
		fprintf (stderr, "Unknown command\n");
		return 1;
	}

	return 0;
}

int main_loop(struct session *session, const char *program_name) {
	int ret = 1;

	while (session->usercount > 0 && !feof(stdin)) {
		char cmdline[BDL_MAXIMUM_CMDLINE_LENGTH];
		int argc_new = 0;
		const char *argv_new[BDL_MAXIMUM_CMDLINE_ARGS];

		argv_new[argc_new++] = program_name;

		fprintf (stderr, "Accepting commands delimeted by LF, CR or NULL\n");

		int first = 1;
		int i;
		for (i = 0; i < BDL_MAXIMUM_CMDLINE_LENGTH - 2 && !feof(stdin); i++) {
			char letter = getchar();

			if (first == 1) {
				if (letter == '\n' || letter == '\r' || letter == '\0') {
					i--;
					continue;
				}
			}

			first = 0;

			if (letter == '\n' || letter == '\r' || letter == '\0') {
				cmdline[i] = ' ';
				cmdline[i+1] = '\0';

				char *begin = cmdline;
				char *end = cmdline + i - 1;
				while ((end = strchr(begin, ' ')) != NULL) {
					*end = '\0';

					// Remove extra spaces
					while (*begin == ' ') {
						*begin = '\0';
						begin++;
					}

					if (argc_new == BDL_MAXIMUM_CMDLINE_ARGS) {
						fprintf (stderr, "Maximum command line arguments reached (%i)\n", BDL_MAXIMUM_CMDLINE_ARGS - 1);
						return 1;
					}

					argv_new[argc_new++] = begin;
					begin = end + 1;
				}

				ret = interpret_command(session, argc_new, argv_new);

				if (ret == 1) {
					return 1;
				}
				else if (session->usercount == 0) {
					return 0;
				}

				break;
			}

			cmdline[i] = letter;
		}
		if (i == BDL_MAXIMUM_CMDLINE_LENGTH - 2) {
			fprintf (stderr, "Maximum command line length reached (%i)\n", BDL_MAXIMUM_CMDLINE_LENGTH - 2);
			return 1;
		}
	}

	return ret;
}

int main(int argc, const char *argv[]) {
	struct session session;
	init_session (&session);

	int ret;

	ret = interpret_command(&session, argc, argv);

	/* And open command increments the user count. Run interactive. */
	if (session.usercount > 0) {
		ret = main_loop(&session, argv[0]);
	}

	// We consider it an error if filehandle is not cleaned up
	while (session.usercount > 0) {
		ret = 1;
		close_session(&session);
	}

	if (ret != 0) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
