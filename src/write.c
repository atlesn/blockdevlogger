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

#define BDL_DBG_WRITE

/* Default hint block spacing is 128MB, one is also placed at the very end */
//#define BDL_DEFAULT_HINT_BLOCK_SPACING 128 * 1024 * 1024;

/* For devices smaller than 256MB, use four blocks plus one at the end */
//#define BDL_SMALL_THRESHOLD 256 * 1024 * 1024;
//#define BDL_DEFAULT_HINT_BLOCK_COUNT_SMALL 4;

struct hintblock_state {
	int valid;
	int blockstart_min;
	int blockstart_max;
	uint64_t highest_timestamp;
	struct bdl_hint_block hintblock;
};

int write_hintblock_get_last_block (
		struct io_file *file,
		const struct hintblock_state *state,
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

	if (block_validate(temp_block_data, master_header, result) != 0) {
		fprintf (stderr, "Block validation failed\n");
		return 1;
	}

	return 0;
}

int write_get_hintblock_state (
		struct io_file *file,
		int pos,
		const struct bdl_header *master_header,
		int blockstart_min,
		int blockstart_max,
		struct hintblock_state *state
) {
	state->valid = 0;
	state->blockstart_min = blockstart_min;
	state->blockstart_max = blockstart_max;
	state->highest_timestamp = 0;

	if (io_read_block (file, pos, (char *) &state->hintblock, sizeof(state->hintblock)) != 0) {
		fprintf (stderr, "Error while reading hint block area at %i\n", pos);
		return 1;
	}

	struct bdl_hint_block hintblock_copy = state->hintblock;
	hintblock_copy.hash = 0;

	int hash_return;
	if (block_validate_hint (&hintblock_copy, master_header, &hash_return) != 0) {
		fprintf (stderr, "Error while checking hash for hint block at %i\n", pos);
		return 1;
	}

	if (state->hintblock.previous_block_pos > blockstart_max || state->hintblock.previous_block_pos < blockstart_min) {
		state->valid = 0;
		return 0;
	}

	state->valid = 1;

	int block_result;
	struct bdl_block_header block;
	if (write_hintblock_get_last_block (file, state, master_header, &block, &block_result) != 0) {
		fprintf (stderr, "Error while getting last block before hintblock\n");
		return 1;
	}

	if (block_result != 0) {
		return 0;
	}

	state->highest_timestamp = block.timestamp;

#ifdef BDL_DBG_WRITE
		printf ("Highest timestamp of hint block was %" PRIu64 "\n", state->highest_timestamp);
#endif

	return 0;
}

struct write_location {
	int block_location;
	int hintblock_location;
};

int write_find_location_small(struct io_file *file, const struct bdl_header *header, struct write_location *location) {
	location->block_location = 0;
	location->hintblock_location = 0;

	return 1;
}

int write_find_location(struct io_file *file, const struct bdl_header *header, struct write_location *location) {

	if (file->size < BDL_SMALL_SIZE_THRESHOLD) {
#ifdef BDL_DBG_WRITE
		printf ("Finding new write location for small device\n");
#endif
		return write_find_location_small(file, header, location);
	}

	location->block_location = 0;
	location->hintblock_location = 0;

#ifdef BDL_DBG_WRITE
	printf ("Finding new write location for large device\n");
#endif

	int device_size = file->size;
	int header_size = BDL_DEFAULT_HEADER_PAD;
	int block_size = header->block_size;

	struct hintblock_state hintblock_state;

	// Search for hint blocks
	int blockstart_min = header_size;
	struct bdl_hint_block hintblock;
	for (int i = header_size + BDL_DEFAULT_HINT_BLOCK_SPACING; i < device_size; i += BDL_DEFAULT_HINT_BLOCK_SPACING) {
#ifdef BDL_DBG_WRITE
		printf ("Searching for hint block at %i\n", i);
#endif
		int blockstart_max = i - header->block_size;

		if (write_get_hintblock_state (
				file, i, header,
				blockstart_min,
				blockstart_max,
				&hintblock_state
				) != 0
		) {
			fprintf (stderr, "Error while reading hint block at %i while finding write location\n", i);
			return 1;
		}

		// We write here if we find an invalid/unused hint block
		if (hintblock_state.valid == 0) {
#ifdef BDL_DBG_WRITE
			printf ("Hint block was free\n");
#endif
			location->block_location = blockstart_min;
			location->hintblock_location = i;

			return 0;
		}
		else if (hintblock_state.valid == 1) {
#ifdef BDL_DBG_WRITE
			printf ("Checking existing hint block\n");
#endif
		}

		// TODO - Not finished!

		blockstart_min = i + header->block_size;
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

int write_put_block (
		struct io_file *file,
		const char *data,
		int data_length,
		const struct write_location *location,
		const struct bdl_header *header
) {
	// Work on data block
	struct bdl_block_header block_header;
	memset (&block_header, '\0', sizeof(block_header));
	block_header.data_length = data_length;
	block_header.timestamp = time_get_64();

	if (data_length > header->block_size - sizeof(block_header)) {
		fprintf(stderr, "Length of data was to large to fit inside a block, length was %i while maximum size is %i\n", data_length, (int) (header->block_size - sizeof(block_header)));
		return 1;
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

	// Work on hint block
	struct bdl_hint_block hint_block;
	memset (&hint_block, '\0', sizeof(hint_block));
	hint_block.previous_block_pos = location->block_location;

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

int write_put_data(const char *device_path, const char *data, int data_length) {
	struct io_file file;

	if (io_open(device_path, "r+", &file) != 0) {
		fprintf (stderr, "Error while opening %s while placing data\n", device_path);
		return 1;
	}

	struct bdl_header header;

	if (block_get_master_header(&file, &header) != 0) {
		fprintf (stderr, "Could not get header from device %s while writing new data block\n", device_path);
		goto out_error;
	}

	struct write_location location;
	if (write_find_location (&file, &header, &location) != 0) {
		fprintf (stderr, "Error while finding write location for device %s\n", device_path);
		goto out_error;
	}

#ifdef BDL_DBG_WRITE
	printf ("Writing new block at location %i with hintblock at %i\n", location.block_location, location.hintblock_location);
#endif

	if (write_put_block(&file, data, data_length, &location, &header) != 0) {
		fprintf (stderr, "Error while placing data block to location %i\n", location.block_location);
		goto out_error;
	}

	success:
	io_close(&file);
	return 0;

	out_error:
	io_close(&file);
	return 1;
}
