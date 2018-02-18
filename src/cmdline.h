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

#ifndef BDL_CMDLINE_H
#define BDL_CMDLINE_H

#define BDL_ARGUMENT_MAX 16
#define BDL_ARGUMENT_SIZE 256

const char *cmd_get_value(const char *key);
int cmd_parse(const int argc, const char *argv[]);
int cmd_match(const char *test);
int cmd_convert_hex_16(const char *key);
int cmd_convert_integer_10(const char *key);
char cmd_get_hex(const char *key);
long int cmd_get_integer(const char *key);
int cmd_check_all_args_used();
const char *cmd_get_argument(int index);

#endif
