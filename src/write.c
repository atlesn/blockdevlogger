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
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>

#include "write.h"
#include "blocks.h"
#include "defaults.h"
#include "io.h"
#include "crypt.h"
#include "validate.h"
#include "bdltime.h"

#define BDL_DBG_WRITE

/* Default hint block spacing is 128MB, one is also placed at the very end */
//#define BDL_DEFAULT_HINT_BLOCK_SPACING 128 * 1024 * 1024;

/* For devices smaller than 256MB, use four blocks plus one at the end */
//#define BDL_SMALL_THRESHOLD 256 * 1024 * 1024;
//#define BDL_DEFAULT_HINT_BLOCK_COUNT_SMALL 4;

int write_find_location_small(struct io_file *file, const struct bdl_header *header, struct bdl_block_location *location) {
	location->block_location = 0;
	location->hintblock_location = 0;

	return 1;
}

int write_check_free_hintblock (
		struct io_file *file,
		int position,
		const struct bdl_header *master_header,
		int blockstart_min,
		int blockstart_max,
		struct bdl_hintblock_state *state,
		struct bdl_block_location *location,
		int *result
) {
	*result = 1;

	// We write here if we find an invalid/unused hint block
	if (state->valid == 0) {

#ifdef BDL_DBG_WRITE
		printf ("Hint block was unused\n");
#endif

		location->block_location = blockstart_min;
		location->hintblock_location = position;

		*result = 0;
	}
	else {
		if (state->hintblock.previous_block_pos + master_header->block_size <= blockstart_max) {

#ifdef BDL_DBG_WRITE
			printf ("Hint block has free room\n");
#endif

			location->block_location = state->hintblock.previous_block_pos + master_header->block_size;
			location->hintblock_location = position;

			*result = 0;
		}
		else {

#ifdef BDL_DBG_WRITE
				printf ("Hint block is full\n");
#endif

			location->block_location = blockstart_min;
			location->hintblock_location = position;

			*result = 1;
		}
	}

	return 0;
}

struct hintblock_timestamp {
	struct bdl_hintblock_state state;
	int initialized;
	uint64_t min_timestamp;
};

int write_hintblock_check_timestamps (struct bdl_hintblock_loop_callback_data *data, int *result) {
	struct hintblock_timestamp *timestamp = (struct hintblock_timestamp *) data->argument_ptr;

	if (data->state->highest_timestamp < timestamp->min_timestamp) {
		timestamp->min_timestamp = data->state->highest_timestamp;
		timestamp->state = *data->state;
		timestamp->initialized = 1;
	}

	*result = BDL_HINTBLOCK_LOOP_OK;

	return 0;
}

int write_hintblock_check_free_callback (struct bdl_hintblock_loop_callback_data *data, int *result) {
	*result = BDL_HINTBLOCK_LOOP_ERR;

	if (write_check_free_hintblock (
			data->file,
			data->position,
			data->master_header,
			data->blockstart_min,
			data->blockstart_max,
			data->state,
			data->location,
			result
			) != 0
	) {
		fprintf (stderr, "Error while checking for free hintblock\n");
		return 1;
	}

	if (*result == 0) {
		// Hint block has free room
		*result = BDL_HINTBLOCK_LOOP_BREAK;
		data->argument_int = 0;
	}
	else if (*result == 1) {
		// Hint block is full
		*result = BDL_HINTBLOCK_LOOP_OK;
		data->argument_int = 1;
	}

	return 0;
}

int write_find_location(struct io_file *file, const struct bdl_header *header, struct bdl_block_location *location) {
	if (file->size < BDL_SMALL_SIZE_THRESHOLD) {
#ifdef BDL_DBG_WRITE
		printf ("Finding new write location for small device\n");
#endif
		return write_find_location_small(file, header, location);
	}

	location->block_location = 0;
	location->hintblock_location = 0;

	// Search for hint blocks
	struct bdl_hintblock_state hintblock_state;
	struct bdl_hintblock_loop_callback_data callback_data;

	callback_data.argument_int = 0;
	callback_data.argument_ptr = NULL;

	// Attempt to find a hint block with free room
	int result;
	if (block_loop_hintblocks_large_device (
			file,
			header,
			write_hintblock_check_free_callback,
			&callback_data,
			&hintblock_state,
			location,
			&result
	) != 0) {
		fprintf (stderr, "Error while looping hint blocks in write operation\n");
		return 1;
	}

	if (callback_data.argument_int == 0) {
		// Hint block with free room found
		return 0;
	}

	// Find hint block with lowest time stamp and invalidate
	struct hintblock_timestamp timestamp;
	timestamp.initialized = 0;
	timestamp.min_timestamp = 0xffffffffffffffff;

	callback_data.argument_ptr = &timestamp;

	if (block_loop_hintblocks_large_device (
			file,
			header,
			write_hintblock_check_timestamps,
			&callback_data,
			&hintblock_state,
			location,
			&result
	) != 0) {
		fprintf (stderr, "Error while looping hint blocks in write operation to find timestamps\n");
		return 1;
	}

	if (timestamp.initialized == 1) {
		location->hintblock_location = timestamp.state.location;
		location->block_location = timestamp.state.blockstart_min;
		return 0;
	}

	fprintf (stderr, "Bug: Reached end of write_find_location()\n");
	exit(EXIT_FAILURE);
}

int write_put_and_pad_block (struct io_file *file, int pos, const char *data, int data_length, char pad, int total_size) {
	int padding_length = total_size - data_length;

	char padding[padding_length];
	memset (padding, pad, padding_length);

	if (io_write_block(file, pos, data, data_length, padding, padding_length, 1) != 0) {
		fprintf (stderr, "Error while writing data and padding to location %i\n", pos);
		return 1;
	}

	return 0;
}

int write_update_hintblock (
		struct io_file *file,
		const struct bdl_block_location *location,
		const struct bdl_header *header

) {
	// Work on hint block
	struct bdl_hint_block hint_block;
	int result;

	// Preserve information from exisiting hint block
	if (block_get_valid_hintblock(file, location->hintblock_location, header, &hint_block, &result)) {
		fprintf (stderr, "Error while getting hint block at %lu\n", location->hintblock_location);
		return 1;
	}

	if (result != 0) {
		memset (&hint_block, '\0', sizeof(hint_block));
	}

	hint_block.previous_block_pos = location->block_location;
	hint_block.hash = 0;

	if (crypt_hash_data(
			(const char *) &hint_block,
			sizeof(hint_block),
			header->default_hash_algorithm,
			&hint_block.hash) != 0
	) {
		fprintf (stderr, "Error while hashing hint block\n");
		return 1;
	}

	if (write_put_and_pad_block(
			file,
			location->hintblock_location,
			(const char *) &hint_block, sizeof(hint_block),
			header->pad_character,
			header->block_size) != 0
	) {
		fprintf (stderr, "Error while putting hint block\n");
		return 1;
	}

	return 0;
}

int write_put_block (
		struct io_file *file,
		const char *data,
		int data_length,
		uint64_t appdata,
		const struct bdl_block_location *location,
		const struct bdl_header *header
) {
	// Work on data block
	struct bdl_block_header block_header;
	memset (&block_header, '\0', sizeof(block_header));
	block_header.data_length = data_length;
	block_header.timestamp = time_get_64();
	block_header.application_data = appdata;

	if (data_length > header->block_size - sizeof(block_header)) {
		fprintf(stderr, "Length of data was to large to fit inside a block, length was %i while maximum size is %i\n", data_length, (int) (header->block_size - sizeof(block_header)));
		return 1;
	}

	if (location->block_location < header->header_size) {
		fprintf (stderr, "Bug: Block location was inside header on write\n");
		exit (EXIT_FAILURE);
	}

	if (location->hintblock_location < header->header_size + header->block_size) {
		fprintf (stderr, "Bug: Hint block location was too early on write\n");
		exit (EXIT_FAILURE);
	}

	int block_total_size = sizeof(block_header) + data_length;
	char hash_data[block_total_size];
	memcpy (hash_data, &block_header, sizeof(block_header));
	memcpy (hash_data + sizeof(block_header), data, data_length);

	if (crypt_hash_data(
			hash_data,
			block_total_size,
			header->default_hash_algorithm,
			&block_header.hash) != 0
	) {
		fprintf (stderr, "Error while hashing block\n");
		return 1;
	}

	memcpy (hash_data, &block_header, sizeof(block_header));

	if (write_put_and_pad_block(
			file,
			location->block_location,
			hash_data, block_total_size,
			header->pad_character,
			header->block_size) != 0
	) {
		fprintf (stderr, "Error while putting block\n");
		return 1;
	}

	return write_update_hintblock(file, location, header);
}

int write_put_data(struct io_file *session_file, const char *data, int data_length, uint64_t appdata) {
	struct bdl_header header;
	int result;

	if (block_get_valid_master_header(session_file, &header, &result) != 0) {
		fprintf (stderr, "Could not get header from device while writing new data block\n");
		return 1;
	}

	if (result != 0) {
		fprintf (stderr, "Invalid header of device while writing new data block\n");
		return 1;
	}

	struct bdl_block_location location;
	if (write_find_location (session_file, &header, &location) != 0) {
		fprintf (stderr, "Error while finding write location for device\n");
		return 1;
	}

#ifdef BDL_DBG_WRITE
	printf ("Writing new block at location %lu with hintblock at %lu\n", location.block_location, location.hintblock_location);
#endif

	if (write_put_block(session_file, data, data_length, appdata, &location, &header) != 0) {
		fprintf (stderr, "Error while placing data block to location %lu\n", location.block_location);
		return 1;
	}

	return 0;
}
