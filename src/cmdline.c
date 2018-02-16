#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "cmdline.h"

#define BDL_DBG_CMDLINE

const char *command_help = "help";

const char *program = NULL;
const char *command = NULL;
const char *args[BDL_ARGUMENT_MAX];
int args_used[BDL_ARGUMENT_MAX];

struct arg_pair {
	char key[BDL_ARGUMENT_SIZE];
	char value[BDL_ARGUMENT_SIZE];
	long int value_int;
	char value_hex;
	int integer_is_converted;
	int hex_is_converted;
};

struct arg_pair arg_pairs[BDL_ARGUMENT_MAX];

void cmd_init() {
	for (int i = 0; i < BDL_ARGUMENT_MAX; i++) {
		args[i] = NULL;
		arg_pairs[i].key[0] = '\0';
		arg_pairs[i].value[0] = '\0';
		arg_pairs[i].value_int = 0;
		arg_pairs[i].value_hex = 0x00;
		arg_pairs[i].integer_is_converted = 0;
		arg_pairs[i].hex_is_converted = 0;
		args_used[i] = 0;
	}
}

int cmd_check_all_args_used() {
	int err = 0;
	for (int i = 0; i < BDL_ARGUMENT_MAX && args[i] != NULL; i++) {
		if (args_used[i] != 1) {
			fprintf (stderr, "Error: Argument %i ('%s') was not used, possible junk or typo\n", i, args[i]);
			err = 1;
		}
	}
	return err;
}

int cmd_get_value_index(const char *key) {
	for (int i = 0; i < BDL_ARGUMENT_MAX && arg_pairs[i].key[0] != '\0'; i++) {
		if (strcmp(arg_pairs[i].key, key) == 0) {
			return i;
		}
	}

	return -1;
}

int cmd_convert_hex_16(const char *key) {
	int index = cmd_get_value_index(key);
	if (index == -1) {
		return 1;
	}

	if (arg_pairs[index].hex_is_converted) {
		return 0;
	}

	char *err;
	long int intermediate = strtol(arg_pairs[index].value, &err, 16);

	if (err[0] != '\0' || intermediate < 0 || intermediate > 0xff) {
		return 1;
	}

	arg_pairs[index].value_hex = intermediate;
	arg_pairs[index].hex_is_converted = 1;

	#ifdef BDL_DBG_CMDLINE

	printf ("Converted argument with key '%s' to hex '%02x'\n", key, arg_pairs[index].value_hex);

	#endif

	return 0;
}

int cmd_convert_integer_10(const char *key) {
	int index = cmd_get_value_index(key);
	if (index == -1) {
		return 1;
	}

	if (arg_pairs[index].integer_is_converted) {
		return 0;
	}

	char *err;
	arg_pairs[index].value_int = strtol(arg_pairs[index].value, &err, 10);

	if (err[0] != '\0') {
		return 1;
	}

	arg_pairs[index].integer_is_converted = 1;

	#ifdef BDL_DBG_CMDLINE

	printf ("Converted argument with key '%s' to integer '%ld'\n", key, arg_pairs[index].value_int);

	#endif

	return 0;
}

long int cmd_get_integer(const char *key) {
	int index = cmd_get_value_index(key);

	if (index == -1) {
		fprintf(stderr, "Bug: Called cmd_convert_integer with unknown key '%s'\n", key);
		exit (EXIT_FAILURE);
	}

	#ifdef BDL_DBG_CMDLINE
	if (arg_pairs[index].integer_is_converted != 1) {
		fprintf(stderr, "Bug: Called cmd_get_integer without cmd_convert_integer being called first\n");
		exit (EXIT_FAILURE);
	}
	#endif

	args_used[index] = 1;

	return arg_pairs[index].value_int;
}

const char *cmd_get_value(const char *key) {
	for (int i = 0; i < BDL_ARGUMENT_MAX && arg_pairs[i].key[0] != '\0'; i++) {
		if (strcmp(arg_pairs[i].key, key) == 0) {
			#ifdef BDL_DBG_CMDLINE
			printf ("Retrieve string argument %s: %s\n", key, arg_pairs[i].value);
			#endif

			args_used[i] = 1;
			return arg_pairs[i].value;
		}
	}

	return NULL;
}


int cmd_parse(int argc, const char *argv[]) {
	program = argv[0];
	command = command_help;

	cmd_init();

	if (argc <= 1) {
		return 0;
	}

	command = argv[1];

	// Store pointers to all arguments
	int arg_pos = 2;
	for (int i = 0; i < BDL_ARGUMENT_MAX && arg_pos < argc; i++) {
		arg_pos = 2 + i;
		args[i] = argv[arg_pos];
	}

	// Parse key-value pairs separated by =
	int pairs_pos = 0;
	for (int i = 0; i < BDL_ARGUMENT_MAX && args[i] != NULL; i++) {
			const char *pos;
			if ((pos = strstr(args[i], "=")) != NULL) {
				const char *value = pos + 1;
				int key_length = pos - args[i];
				int value_length = strlen(value);

				if (key_length == 0 || value_length == 0) {
					fprintf (stderr, "Error: Syntax error with = syntax in argument %i ('%s'), use key=value\n", i, args[i]);
					return 1;
				}

				if (key_length > BDL_ARGUMENT_SIZE - 1) {
					fprintf (stderr, "Error: Argument key %i too long ('%s'), maximum size is %i\n", i, args[i], BDL_ARGUMENT_SIZE - 1);
					return 1;
				}
				if (value_length > BDL_ARGUMENT_SIZE - 1) {
					fprintf (stderr, "Error: Argument value %i too long ('%s'), maximum size is %i\n", i, args[i], BDL_ARGUMENT_SIZE - 1);
					return 1;
				}

				strncpy(arg_pairs[pairs_pos].key, args[i], key_length);
				arg_pairs[pairs_pos].key[key_length] = '\0';

				strncpy(arg_pairs[pairs_pos].value, value, value_length);
				arg_pairs[pairs_pos].value[value_length] = '\0';

				pairs_pos++;
			}
	}

	#ifdef BDL_DBG_CMDLINE

	printf ("Program: %s\n", program);
	printf ("Command: %s\n", command);

	for (int i = 0; i < BDL_ARGUMENT_MAX && args[i] != NULL; i++) {
		printf ("Argument %i: %s\n", i, args[i]);
	}

	for (int i = 0; i < BDL_ARGUMENT_MAX && arg_pairs[i].key[0] != '\0'; i++) {
		printf ("Argument %i key: %s\n", i, arg_pairs[i].key);
		printf ("Argument %i value: %s\n", i, arg_pairs[i].value);
	}

	#endif

	return 0;
}

int cmd_match(const char *test) {
	return strcmp(command, test) == 0;
}

