#ifndef BDL_CMDLINE_H
#define BDL_CMDLINE_H

#define BDL_ARGUMENT_MAX 16
#define BDL_ARGUMENT_SIZE 256

const char *cmd_get_value(const char *key);
int cmd_parse(const int argc, const char *argv[]);
int cmd_match(const char *test);
int cmd_convert_integer_10(const char *key);
int cmd_convert_hex_16(const char *key);
long int cmd_get_integer(const char *key);
int cmd_check_all_args_used();

#endif
