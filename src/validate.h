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

#include "io.h"

int validate_dev (const char *device_path, struct io_file *session_file);
int validate_block(const char *all_data, const struct bdl_header *master_header, int *result);
int validate_hint (const struct bdl_hint_block *header_orig, const struct bdl_header *master_header, int *result);
int validate_header (const struct bdl_header *header, int file_size, int *result);
