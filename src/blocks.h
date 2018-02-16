#ifndef BDL_BLOCKS_H
#define BDL_BLOCKS_H

#include <stdint.h>

struct bdl_header {
	char header_begin_message[32];

	uint16_t version_major;
	uint16_t version_minor;

	uint64_t block_size;
	uint64_t block_count;

	/* Usually 0xff or 0x00 */
	char pad_character;
};

struct bdl_header_pad {
	char pad;
};

#endif
