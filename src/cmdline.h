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

#ifndef BDL_CMDLINE_H
#define BDL_CMDLINE_H

#include <stdint.h>

#include "defaults.h"

#define BDL_ARGUMENT_MAX BDL_MAXIMUM_CMDLINE_ARGS
#define BDL_ARGUMENT_SIZE BDL_MAXIMUM_CMDLINE_ARG_SIZE

struct cmd_arg_pair {
	char key[BDL_ARGUMENT_SIZE];
	char value[BDL_ARGUMENT_SIZE];
	long int value_int;
	unsigned char value_hex;
	uint64_t value_hex_64;
	uint64_t value_uint_64;
	int integer_is_converted;
	int hex_is_converted;
	int hex64_is_converted;
	int uint64_is_converted;
};

struct cmd_data {
	const char *program;
	const char *command;
	const char *args[BDL_ARGUMENT_MAX];
	int args_used[BDL_ARGUMENT_MAX];
	struct cmd_arg_pair arg_pairs[BDL_ARGUMENT_MAX];
};

int cmd_parse						(struct cmd_data *data, const int argc, const char *argv[]);
int cmd_match						(struct cmd_data *data, const char *test);

int cmd_convert_hex_64				(struct cmd_data *data, const char *key);
int cmd_convert_hex_byte			(struct cmd_data *data, const char *key);
int cmd_convert_integer_10			(struct cmd_data *data, const char *key);
int cmd_convert_uint64_10			(struct cmd_data *data, const char *key);

char cmd_get_hex_byte				(struct cmd_data *data, const char *key);
uint64_t cmd_get_hex_64				(struct cmd_data *data, const char *key);
long int cmd_get_integer			(struct cmd_data *data, const char *key);
uint64_t cmd_get_uint64				(struct cmd_data *data, const char *key);
const char *cmd_get_argument		(struct cmd_data *data, int index);
const char *cmd_get_last_argument	(struct cmd_data *data);
const char *cmd_get_value			(struct cmd_data *data, const char *key);

int cmd_check_all_args_used			(struct cmd_data *data);

#endif
