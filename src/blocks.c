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
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

#include "blocks.h"
#include "defaults.h"
#include "crypt.h"
#include "io.h"
#include "validate.h"


int block_hintblock_get_last_block (
		struct io_file *file,
		const struct bdl_hintblock_state *state,
		const struct bdl_header *master_header,
		struct bdl_block_header *block,
		int *result
) {
	char temp_block_data[master_header->block_size];
	struct bdl_block_header *block_tmp = (struct bdl_block_header *) temp_block_data;

	*result = 1;
	if (io_read_block (file, state->hintblock.previous_block_pos, temp_block_data, master_header->block_size) != 0) {
		fprintf (stderr, "Could not read block at %i\n", (int)state->hintblock.previous_block_pos);
		return 1;
	}

	if (validate_block(temp_block_data, master_header, result) != 0) {
		*result = 1;
		return 0;
	}

	memcpy (block, temp_block_data, sizeof(*block));

	return 0;
}

int block_get_valid_hintblock (
		struct io_file *file,
		unsigned long int pos,
		const struct bdl_header *master_header,
		struct bdl_hint_block *hintblock,
		int *result
) {
	*result = 0;

	if (io_read_block (file, pos, (char *) hintblock, sizeof(*hintblock)) != 0) {
		fprintf (stderr, "Error while reading hint block area at %lu\n", pos);
		return 1;
	}

	struct bdl_hint_block hintblock_copy = *hintblock;

	int hash_return;
	if (validate_hint (&hintblock_copy, master_header, result) != 0) {
		fprintf (stderr, "Error while checking hash for hint block at %lu\n", pos);
		return 1;
	}

	return 0;
}

int block_get_hintblock_state (
		struct io_file *file,
		unsigned long int pos,
		const struct bdl_header *master_header,
		unsigned long int blockstart_min,
		unsigned long int blockstart_max,
		struct bdl_hintblock_state *state
) {
	state->valid = 0;
	state->blockstart_min = blockstart_min;
	state->blockstart_max = blockstart_max;
	state->highest_timestamp = 0;
	state->location = pos;

	int result;
	if (block_get_valid_hintblock(file, pos, master_header, &state->hintblock, &result) != 0) {
		fprintf (stderr, "Error while reading hint block area at %lu\n", pos);
		return 1;
	}

	if (result != 0) {
		state->valid = 0;
		return 0;
	}
	else if (state->hintblock.previous_block_pos > blockstart_max || state->hintblock.previous_block_pos < blockstart_min) {
		state->valid = 0;
		return 0;
	}

	state->valid = 1;

	int block_result;
	struct bdl_block_header block;
	if (block_hintblock_get_last_block (file, state, master_header, &block, &block_result) != 0) {
		fprintf (stderr, "Error while getting last block before hintblock\n");
		return 1;
	}

	if (block_result != 0) {
		state->valid = 0;
		return 0;
	}

	state->highest_timestamp = block.timestamp;

#ifdef BDL_DBG_WRITE
		printf ("Highest timestamp of hint block was %" PRIu64 "\n", state->highest_timestamp);
#endif

	return 0;
}

int block_loop_hintblocks_large_device (
		struct io_file *file,
		const struct bdl_header *header,
		int (*callback)(struct bdl_hintblock_loop_callback_data *, int *result),
		struct bdl_hintblock_loop_callback_data *callback_data,
		struct bdl_hintblock_state *hintblock_state,
		struct bdl_block_location *location,
		int *result
) {
	location->block_location = 0;
	location->hintblock_location = 0;

	// Search for hint blocks

	callback_data->file = file;
	callback_data->master_header = header;
	callback_data->state = hintblock_state;
	callback_data->location = location;

	unsigned long int device_size = file->size;
	unsigned long int header_size = header->header_size;

	unsigned long int blockstart_min = header_size;

	unsigned long int loop_begin = header_size + BDL_DEFAULT_HINT_BLOCK_SPACING;
	unsigned long int loop_spacing = BDL_DEFAULT_HINT_BLOCK_SPACING;

	for (unsigned long int i = loop_begin; i < device_size; i += loop_spacing) {
		unsigned long int blockstart_max = i - header->block_size;

		if (block_get_hintblock_state (
				file, i, header,
				blockstart_min,
				blockstart_max,
				hintblock_state
				) != 0
		) {
			fprintf (stderr, "Error while reading hint block at %lu while looping\n", i);
			return 1;
		}

		callback_data->position = i;
		callback_data->blockstart_min = blockstart_min;
		callback_data->blockstart_max = blockstart_max;

		if (callback(callback_data, result) != 0) {
			fprintf (stderr, "Error in callback function for hint block loop\n");
			return 1;
		}

		if (*result == BDL_HINTBLOCK_LOOP_BREAK) {
			return 0;
		}
		else if (*result == BDL_HINTBLOCK_LOOP_ERR) {
			return 1;
		}

		blockstart_min = i + header->block_size;
	}

	// TODO: Code for hint block at the very end

	return 0;
}

int block_get_valid_master_header(struct io_file *file, struct bdl_header *header, int *result) {
	if (io_read_block(file, 0, (char *) header, sizeof(*header)) != 0) {
		fprintf (stderr, "Error while reading header from file\n");
		return 1;
	}

	if (validate_header(header, file->size, result) != 0) {
		fprintf (stderr, "Error while validating header\n");
		return 1;
	}

	return 0;
}
