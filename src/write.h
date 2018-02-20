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

#ifndef BDL_WRITE_H
#define BDL_WRITE_H

#include <stdint.h>

#include "io.h"

int write_put_block (
		struct io_file *session_file,
		const char *data, unsigned long int data_length,
		uint64_t appdata,
		uint64_t timestamp,
		unsigned long int faketimestamp
);

#endif
