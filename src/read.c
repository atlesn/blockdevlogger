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

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "io.h"
#include "blocks.h"

struct read_block_loop_data {
	uint64_t timestamp_gteq;
};

// #define BDL_READ_DEBUG

int read_block_loop_callback(struct bdl_block_loop_callback_data *data, int *result) {
	const struct read_block_loop_data *loop_data = (struct read_block_loop_data *) data->argument_ptr;
	const struct bdl_block_header *block_header = data->block;

#ifdef BDL_READ_DEBUG
	printf ("Check block at %lu\n", data->location->block_location);
#endif

	if (block_header->timestamp >= loop_data->timestamp_gteq) {
		block_dump(block_header, data->block_data);
	}

	return 0;
}

int read_hintblock_loop_callback(struct bdl_hintblock_loop_callback_data *data, int *result) {
	struct read_block_loop_data *loop_data = (struct read_block_loop_data *) data->argument_ptr;
	const struct bdl_hintblock_state *hintblock_state = &data->location->hintblock_state;
	const struct bdl_header *master_header = data->master_header;

#ifdef BDL_READ_DEBUG
	printf ("Checking if hintblock highest timestamp %" PRIx64 " is >= %" PRIx64 "\n",
				hintblock_state->highest_timestamp,
				loop_data->timestamp_gteq
	);
#endif

	if (hintblock_state->valid != 1) {
		#ifdef BDL_READ_DEBUG
			printf ("- Hint block was not valid, ending here\n");
		#endif
		*result = BDL_BLOCK_LOOP_BREAK;
		return 0;
	}

	if (hintblock_state->highest_timestamp < loop_data->timestamp_gteq) {
		#ifdef BDL_READ_DEBUG
			printf ("- Timestamp outside range\n");
		#endif
		*result = BDL_BLOCK_LOOP_OK;
		return 0;
	}

	*result = BDL_BLOCK_LOOP_OK;

#ifdef BDL_READ_DEBUG
	printf ("- Hintblock matched\n");
#endif

	char block_buf[master_header->block_size];
	struct bdl_block_header *block_header;
	char *block_data;

	struct bdl_block_loop_callback_data callback_data;
	unsigned long int block_position;

	callback_data.argument_int = 0;
	callback_data.argument_ptr = (void *) loop_data;

	if (block_loop_blocks(
			data->file, master_header, hintblock_state,
			read_block_loop_callback,
			block_buf, master_header->block_size,
			&block_header, &block_data,
			&callback_data,
			result
	) != 0) {
		fprintf (stderr, "Error while looping blocks in hintblock loop\n");
		return 1;
	}

	return 0;
}

int read_blocks (struct io_file *device, uint64_t timestamp_gteq) {
	struct bdl_header master_header;
	int result;

	if (block_get_validate_master_header(device, &master_header, &result) != 0) {
		fprintf (stderr, "Error while getting master header before reading\n");
		return 1;

	}
	if (result != 0) {
		fprintf (stderr, "Master header of device was not valid before reading\n");
		return 1;
	}

	struct read_block_loop_data loop_data;
	loop_data.timestamp_gteq = timestamp_gteq;

	struct bdl_hintblock_loop_callback_data callback_data;
	struct bdl_block_location location;

	callback_data.argument_int = 0;
	callback_data.argument_ptr = (void *) &loop_data;

	if (block_loop_hintblocks_large_device (
			device, &master_header,
			read_hintblock_loop_callback, &callback_data,
			&location,
			&result
	) != 0) {
		fprintf (stderr, "Error while looping hintblocks while reading blocks\n");
		return 1;
	}

	return 0;
}
