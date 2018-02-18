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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include "blocks.h"
#include "defaults.h"
#include "crypt.h"
#include "io.h"

int block_validate(const char *all_data, const struct bdl_header *master_header, int *result) {
	struct bdl_block_header *header = (struct bdl_block_header *) all_data;

	if (header->data_length > master_header->block_size - sizeof(header)) {
		return 1;
	}

	uint32_t hash_orig = header->hash;
	header->hash = 0;

	if (crypt_check_hash(
			all_data,
			sizeof(*header) + header->data_length,
			master_header->default_hash_algorithm,
			hash_orig,
			result) != 0
	) {
		fprintf (stderr, "Error while checking hash for block\n");
		return 1;
	}

	header->hash = hash_orig;

	return 0;
}

int block_validate_hint (const struct bdl_hint_block *header_orig, const struct bdl_header *master_header, int *result) {
	struct bdl_hint_block header = *header_orig;

	header.hash = 0;

	if (crypt_check_hash(
			(const char *) &header,
			sizeof(header),
			master_header->default_hash_algorithm,
			header_orig->hash,
			result) != 0
	) {
		fprintf (stderr, "Error while validating hash for hint block\n");
		return 1;
	}

	return 0;
}


int block_validate_header (const struct bdl_header *header, int file_size) {
	struct bdl_header header_copy = *header;
	header_copy.hash = 0; // Needs to be zero for hash to be valid

	int result;
	if (crypt_check_hash(
			(const char *) &header_copy,
			sizeof(header_copy),
			header->default_hash_algorithm,
			header->hash,
			&result) != 0
	) {
		return 1;
	}

	if (result != 0) {
		fprintf (stderr, "Header checksum of device was not valid, possible data corruption. Please re-initialize device.\n");
		return 1;
	}

	if (header->total_size < header->block_size * 2) {
		fprintf (stderr, "The total size specified in the header was too small. Please re-initialize device.\n");
		return 1;
	}

	if (header->total_size > file_size) {
		fprintf (stderr, "The file size specified in the header is larger than the file size. Please re-initialize device.\n");
		return 1;
	}

	if (header->total_size % header->block_size != 0) {
		fprintf (stderr, "The header total size was not dividable by block size. Please re-initialize device.\n");
		return 1;
	}

	if (header->version_major != BDL_CONFIG_VERSION_MAJOR || header->version_minor != BDL_CONFIG_VERSION_MINOR) {
		fprintf (stderr, "Incompatible header version. Header has version is V%u.%u, and we require V%u.%u.",
				header->version_major,
				header->version_minor,
				BDL_CONFIG_VERSION_MAJOR,
				BDL_CONFIG_VERSION_MINOR
		);
		return 1;
	}
	else if (header->block_size < BDL_MINIMUM_BLOCKSIZE) {
		fprintf (stderr, "The block size defined in the header was %" PRIu64 " but minimum size is %d\n", header->block_size, BDL_MINIMUM_BLOCKSIZE);
		return 1;
	}
	else if (header->block_size > BDL_MAXIMUM_BLOCKSIZE) {
		fprintf (stderr, "The block size defined in the header was %" PRIu64 " but maximum size is %d\n", header->block_size, BDL_MAXIMUM_BLOCKSIZE);
		return 1;
	}
	else if (header->block_size % BDL_BLOCKSIZE_DIVISOR) {
		fprintf (stderr, "The block size defined in the header (%" PRIu64 ") was not dividable with %d\n", header->block_size, BDL_BLOCKSIZE_DIVISOR);
		return 1;
	}

	return 0;
}

int block_get_master_header(struct io_file *file, struct bdl_header *header) {
	if (io_read_block(file, 0, (char *) header, sizeof(*header)) != 0) {
		fprintf (stderr, "Error while reading header from file\n");
		return 1;
	}

	return block_validate_header(header, file->size);
}
