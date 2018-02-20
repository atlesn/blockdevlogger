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

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/mman.h>

#include "io.h"
#include "defaults.h"

//#define BDL_DEBUG_IO

int io_get_file_size(FILE *file, const char *filepath, unsigned long int *size) {
	struct stat params;

	if (stat(filepath, &params) != 0) {
		fprintf (stderr, "Could not stat file/device: %s\n", strerror(errno));
		return 1;
	}

	if (S_ISBLK(params.st_mode)) {
		unsigned long int buf;
		if (ioctl(fileno(file), BLKGETSIZE64, &buf) != 0) {
			fprintf (stderr, "Error while getting size of block device: %s\n", strerror(errno));
			return 1;
		}
		*size = buf;
	}
	else if (S_ISREG(params.st_mode)) {
		if (fseek(file, 0, SEEK_END)) {
			fprintf (stderr, "Could not seek to end of device to get it's size: %s\n", strerror(errno));
			return 1;
		}

		*size = ftell(file);

		rewind (file);
	}
	else {
		fprintf (stderr, "Unknown file type, must be regular or block device\n");
		return 1;
	}

	return 0;
}

int io_close (struct io_file *file) {
	int ret = 0;
	if (msync(file->memorymap, file->size, MS_SYNC) != 0) {
		ret = 1;
		fprintf (stderr, "Warning: Error while syncing with device, changes might have been lost: %s\n", strerror(errno));
	}

	munmap(file->memorymap, file->size);
	fclose (file->file);
	return ret;
}

int io_open(const char *path, struct io_file *file) {
	file->file = fopen(path, "r+");
	file->seek = 0;
	file->unsynced_write_bytes = 0;

	file->sync_queue.count = 0;

	if (file->file == NULL) {
		fprintf (stderr, "Could not open device %s in mode r/w: %s\n", path, strerror(errno));
		return 1;
	}

	if (io_get_file_size(file->file, path, &file->size) != 0) {
		fprintf (stderr, "Error while getting file size of %s\n", path);
		return 1;
	}

	file->memorymap = mmap(NULL, file->size, PROT_READ|PROT_WRITE, MAP_SHARED, fileno(file->file), 0);
	if (file->memorymap == MAP_FAILED) {
		fprintf (stderr, "Memory mapping failed: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

int io_seek(struct io_file *file, unsigned long int pos) {
	if (pos >= file->size) {
		fprintf (stderr, "Attempted to seek outside file\n");
		return 1;
	}

	file->seek = pos;

	return 0;
}

int io_read(struct io_file *file, void *target, unsigned int length) {
	if (file->seek + length >= file->size) {
		fprintf (stderr, "Attempted to read outside file\n");
		return 1;
	}

	memcpy (target, file->memorymap+file->seek, length);

	return 0;
}

int io_update_sync_queue(struct io_sync_queue *sync_queue, void *start, void *end) {
	if (sync_queue->count == 0) {
		sync_queue->entries[0].start_address = start;
		sync_queue->entries[0].end_address = end;
		sync_queue->count++;
		return 0;
	}

	struct io_sync_queue_entry *entry;
	for (int i = 0; i < sync_queue->count; i++) {
		entry = &sync_queue->entries[i];
		if (start == entry->start_address || (start > entry->start_address && start <= entry->end_address)) {
			if (end > entry->end_address) {
				entry->end_address = end;
			}
			return 0;
		}
		if (end == entry->end_address || (end < entry->end_address && end >= entry->start_address)) {
			if (start < entry->start_address) {
				entry->start_address = start;
			}
			return 0;
		}
	}

	if (sync_queue->count == BDL_IO_SYNC_QUEUE_MAX) {
		return 1;
	}

	entry = &sync_queue->entries[sync_queue->count++];

	entry->start_address = start;
	entry->end_address = end;

	return 0;
}

int io_write(struct io_file *file, const void *source, unsigned int length) {
	if (file->seek + length >= file->size) {
		fprintf (stderr, "Attempted to write outside file\n");
		return 1;
	}

	void *write_location = file->memorymap + file->seek;
	memcpy (write_location, source, length);

	file->unsynced_write_bytes += length;
	if (io_update_sync_queue(&file->sync_queue, write_location, write_location + length) || file->unsynced_write_bytes >= BDL_MMAP_SYNC_SIZE) {
		for (int i = 0; i < file->sync_queue.count; i++) {
			struct io_sync_queue_entry *entry = &file->sync_queue.entries[i];

			// Align to page size
			uintptr_t address_fix = (uintptr_t) entry->start_address;

			// Mask with 10 ones at the end if page size is 4096
			uintptr_t address_mask = getpagesize() - 1;

			// Invert mask
			address_mask = ~address_mask;

			// Round address with the mask by removing the end
			address_fix = address_fix & address_mask;
			entry->start_address = (void *) address_fix;

			printf ("B: Sync %llu - %lu\n", (long long unsigned int) entry->start_address, entry->end_address - entry->start_address);

			if (entry->start_address < file->memorymap) {
				entry->start_address = file->memorymap;
			}

			if (msync(entry->start_address, entry->end_address - entry->start_address, MS_SYNC) != 0) {
				fprintf (stderr, "Warning: Error while syncing with device, changes might have been lost: %s\n", strerror(errno));
			}
		}
		file->unsynced_write_bytes = 0;
		file->sync_queue.count = 0;
	}

	return 0;
}

int io_write_block(
		struct io_file *file,
		unsigned long int position,
		const char *data, unsigned long int data_length,
		const char *padding, unsigned long int padding_length,
		int verbose
) {
#ifdef BDL_DEBUG_IO
	printf ("Write block to pos %lu total size %lu\n", position, data_length+padding_length);
#endif
	// Write data
	if (io_seek (file, position) != 0) {
		fprintf (stderr, "Seek failed while writing block to position %lu\n", position);
		return 1;
	}
	if (io_write (file, data, data_length) != 0) {
		fprintf (stderr, "Error while writing block at position %lu\n", position);
		return 1;
	}

	// Write padding
	unsigned long int padding_pos = position + data_length;
	if (io_seek (file, padding_pos) != 0) {
		fprintf (stderr, "Seek failed while writing block padding to position %lu\n", padding_pos);
		return 1;
	}
	if (io_write (file, padding, padding_length) != 0) {
		fprintf (stderr, "Error while writing block at position %lu\n", position);
		return 1;
	}

	return 0;
}

int io_read_block(struct io_file *file, unsigned long int position, char *data, unsigned long int data_length) {
	if (io_seek (file, position) != 0) {
		fprintf (stderr, "Error while seeking to read area at %lu\n", position);
		return 1;
	}

	if (io_read (file, data, data_length)) {
		fprintf (stderr, "Error while reading area at %lu\n", position);
		return 1;
	}

	return 0;
}
