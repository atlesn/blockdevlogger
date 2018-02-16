#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "cmdline.h"

const char *command_help = "help";

const char *command = NULL;
const char *arg1 = NULL;

void cmd_parse(const int argc, const char *argv[]) {
	command = command_help;

	if (argc <= 0) {
		return;
	}

	command = argv[argc--];
}

int cmd_match(const char *test) {
	return strcmp(command, test) == 0;
}

