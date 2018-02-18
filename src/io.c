#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"

#define BDL_DEBUG_IO

int io_get_file_size(FILE *file, int *size) {
	if (fseek(file, 0, SEEK_END)) {
		fprintf (stderr, "Could not seek to end of device to get it's size: %s\n", strerror(errno));
		return 1;
	}

	*size = ftell(file);

	rewind (file);

	return 0;
}

void io_close (struct io_file *file) {
	fclose (file->file);
}

int io_open(const char *path, const char *mode, struct io_file *file) {
	file->file = fopen(path, mode);

	if (file->file == NULL) {
		fprintf (stderr, "Could not open device %s in mode %s: %s\n", path, mode, strerror(errno));
		return 1;
	}

	if (io_get_file_size(file->file, &file->size) != 0) {
		fprintf (stderr, "Error while getting file size of %s\n", path);
		return 1;
	}

	return 0;
}

int io_seek(struct io_file *file, int pos) {
	int new_pos = fseek (file->file, pos, SEEK_SET);

	if (new_pos == -1) {
		fprintf (stderr, "Error while telling position in file: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

int io_seek_checked(struct io_file *file, int pos) {
	if (fseek (file->file, pos, SEEK_SET) != 0) {
		fprintf (stderr, "Error while seeking to position %i in file: %s\n", pos, strerror(errno));
		return 1;
	}

	int new_pos = ftell(file->file);
	if (new_pos == -1) {
		fprintf (stderr, "Error while telling position in file: %s\n", strerror(errno));
		return 1;
	}
	else if (new_pos != pos) {
		fprintf (stderr, "Bug: Attempted to seek outside of file in checked seek\n");
		exit (EXIT_FAILURE);
	}

	return 0;
}

int io_read_checked (char *target, int size, struct io_file *file) {
	int bytes = fread(target, 1, size, file->file);

#ifdef BDL_DEBUG_IO
	printf ("IO: Bytes read: %i, Expected: %i\n", bytes, size);
#endif

	if (bytes < size) {
		int err = ferror(file->file);
		if (err) {
			fprintf (stderr, "Could not read data: %s\n", strerror(err));
		}
		else if (feof(file->file)) {
			fprintf (stderr, "Could read data, device was too small\n");
		}
		else {
			fprintf (stderr, "Could not read data: Unknown error, too few bytes was read\n");
		}

		return 1;
	}

	return 0;
}

int io_seek_to_next_block(struct io_file *file, int header_size, int block_size, int block_pos) {
	int pos = header_size + block_size * block_pos;

	// Wrap around if we reach the end
	if (pos > file->size) {
		pos = header_size;
	}

	if (io_seek_checked(file, pos) != 0) {
		fprintf (stderr, "Error while seeking to next block at position %i\n", pos);
		return 1;
	}

	return 0;
}

int io_write_block(struct io_file *file, int position, const char *data, int data_length, const char *padding, int padding_length, int verbose) {
	if (io_seek_checked (file, position) != 0) {
		fprintf (stderr, "Seek failed while writing block to position %i\n", position);
		return 1;
	}

	size_t bytes;

	// Write data
	bytes = fwrite (data, 1, data_length, file->file);
	if (bytes == -1) {
		fprintf (stderr, "Error while writing data: %s\n", strerror(errno));
		return 1;
	}
	else if (bytes < data_length) {
		return 1;
	}

	// Write padding
	if (padding_length > 0) {
		bytes = fwrite (padding, 1, padding_length, file->file);
		if (bytes == -1) {
			fprintf (stderr, "Error while writing data to file: %s\n", strerror(errno));
			return 1;
		}
		else if (bytes < data_length) {
			return 1;
		}
	}

	success:
	return 0;
}

int io_read_block(struct io_file *file, int position, char *data, int data_length) {
	if (io_seek_checked (file, position) != 0) {
		fprintf (stderr, "Error while seeking to read area at %i\n", position);
		return 1;
	}

	if (io_read_checked (data, data_length, file)) {
		fprintf (stderr, "Error while reading area at %i\n", position);
		return 1;
	}

	return 0;
}
