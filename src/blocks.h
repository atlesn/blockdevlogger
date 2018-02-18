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

#ifndef BDL_BLOCKS_H
#define BDL_BLOCKS_H

#include <stdint.h>

#include "io.h"

struct bdl_header {
	uint8_t header_begin_message[32];

	uint8_t version_major;
	uint8_t version_minor;

	/* Usually 0xff or 0x00 */
	uint8_t pad_character;

	/* For future use, currently 0=CRC32 */
	uint8_t default_hash_algorithm;

	/* Size of a block in bytes */
	uint64_t block_size;

	/* Total size to write after the header before we wrap */
	uint64_t total_size;

	/* Future use? */
	uint32_t padding;

	/* Hash of all parameters with hash being zero */
	uint32_t hash;
};

struct bdl_block_header {
	uint64_t timestamp;

	/* Usable for applications */
	uint64_t application_data;

	/* Length of actual data, the rest up to the block size defined in the header is padded */
	uint64_t data_length;

	/* Future use? */
	uint32_t pad;

	/* Hash of header and data with hash itself being zero */
	uint32_t hash;
};

struct bdl_hint_block {
	int64_t previous_block_pos;

	/* Future use? */
	uint32_t pad;

	/* Hash of header and data with hash itself being zero */
	uint32_t hash;
};

struct bdl_header_pad {
	uint8_t pad;
};

int block_validate(const char *all_data, const struct bdl_header *master_header, int *result);
int block_get_master_header(struct io_file *file, struct bdl_header *header);
int block_validate_hint (const struct bdl_hint_block *header_orig, const struct bdl_header *master_header, int *result);

#endif
