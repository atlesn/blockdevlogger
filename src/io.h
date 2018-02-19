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

#ifndef BDL_IO_H
#define BDL_IO_H

#define BDL_IO_SYNC_QUEUE_MAX 16

struct io_sync_queue_entry {
	void *start_address;
	void *end_address;
};

struct io_sync_queue {
	struct io_sync_queue_entry entries[BDL_IO_SYNC_QUEUE_MAX];
	int count;
};

struct io_file {
	FILE *file;
	unsigned long int size;
	unsigned long int seek;
	unsigned long int unsynced_write_bytes;
	void *memorymap;
	struct io_sync_queue sync_queue;
};

int io_close (struct io_file *file);
int io_open(const char *path, struct io_file *file);
int io_write_block(struct io_file *file, unsigned long int position, const char *data, unsigned long int data_length, const char *padding, unsigned long int padding_length, int verbose);
int io_read_block(struct io_file *file, unsigned long int position, char *data, unsigned long int data_length);

#endif
