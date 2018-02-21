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

#ifndef BDL_H
#define BDL_H

#define BDL_IO_SYNC_QUEUE_MAX 16

struct bdl_io_sync_queue_entry {
	void *start_address;
	void *end_address;
};

struct bdl_io_sync_queue {
	struct bdl_io_sync_queue_entry entries[BDL_IO_SYNC_QUEUE_MAX];
	int count;
};

struct bdl_io_file {
	FILE *file;
	unsigned long int size;
	unsigned long int seek;
	unsigned long int unsynced_write_bytes;
	void *memorymap;
	struct bdl_io_sync_queue sync_queue;
};

struct bdl_session {
	struct bdl_io_file device;
	int usercount;
};

void bdl_init_session (struct bdl_session *session);
int bdl_start_session (struct bdl_session *session, const char *device_path);
void bdl_close_session (struct bdl_session *session);

int interpret_command (struct bdl_session *session, int argc, const char *argv[]);

#endif
