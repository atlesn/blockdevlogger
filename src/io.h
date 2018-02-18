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

#ifndef BDL_IO_H
#define BDL_IO_H

struct io_file {
	FILE *file;
	int size;
};

void io_close (struct io_file *file);
int io_open(const char *path, const char *mode, struct io_file *file);
int io_write_block(struct io_file *file, int position, const char *data, int data_length, const char *padding, int padding_length, int verbose);
int io_read_block(struct io_file *file, int position, char *data, int data_length);

#endif
