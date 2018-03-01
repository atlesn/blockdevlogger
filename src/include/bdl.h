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

#include <stdint.h>

/* ****
 * The following structs are usually only used internally
 * ****/
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
	unsigned long long int size;
	unsigned long int seek;
	unsigned long int unsynced_write_bytes;
	void *memorymap;
	struct bdl_io_sync_queue sync_queue;
};

/* ****
 * Struct for holding session data. Should not be modified manually.
 * ****/
struct bdl_session {
	struct bdl_io_file device;
	int usercount;
};

/* ****
 * First run these to open device and initialize session (not close untill the end doh).
 * Multiple start_session may be called on the same session, in which the same number
 * of close commands must be called before the session is actually closed.
 * ****/
void bdl_init_session (struct bdl_session *session);
int bdl_start_session (struct bdl_session *session, const char *device_path);
void bdl_close_session (struct bdl_session *session);

/* ****
 * Then these may be run after session is created. They return 1 on error
 * or 0 on success.
 *
 * Some also store a result in int *result. 0 means that the action was performed,
 * 1 means it was not (but not fatal error, like to tell us that validation succeeded
 * but device was found invalid).
 * ****/

/* This invalidates all hint blocks, effectively making all entries unreachable */
int bdl_clear_dev (struct bdl_session *session, int *result);

/* This checks if a device is initialized and read/writeable */
int bdl_validate_dev (struct bdl_session *session, int *result);

/* Read blocks to STDOUT */
int bdl_read_blocks (
		struct bdl_session *session,
		uint64_t timestamp_gteq, unsigned long int limit
);

// TODO : Read blocks to callback function

/* ****
 * Write a block and update hint block and backup hint block. Arguments after data_length
 * may be zero. Returns > 0 on error, see defines below.
 * ****/
int bdl_write_block (
		struct bdl_session *session,
		const char *data, unsigned long int data_length,
		uint64_t appdata, uint64_t timestamp, unsigned long int faketimestamp
);

#define BDL_WRITE_ERR				1 // Other error (often IO)
#define BDL_WRITE_ERR_TIMESTAMP		2 // Timestamp was smaller than the newest entry
#define BDL_WRITE_ERR_SIZE			3 // Size of block was too big
#define BDL_WRITE_ERR_IO			4 // IO error
#define BDL_WRITE_ERR_CORRUPT		5 // Corrupt device


/* ****
 * Initialize a new device (must be opened first with bdl_start_session). Device must
 * contain zeros at the beginning for a couple of kBs. Arguments after session may
 * be zero.
 * ****/
int bdl_init_dev (
		struct bdl_session *session,
		unsigned long int blocksize, unsigned long int header_pad, char padchar
);

/* ****
 * Call commands like if they were written on the command line. Usually only
 * used from BDL command line program.
 * ****/
int bdl_interpret_command (
		struct bdl_session *session,
		int argc, const char *argv[]
);

#endif
