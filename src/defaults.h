#ifndef BDL_DEFAULTS_H
#define BDL_DEFAULTS_H

/* Blocks are allocated on the stack, don't make them too big */
#define BDL_DEFAULT_BLOCKSIZE 512
#define BDL_MINIMUM_BLOCKSIZE 512
#define BDL_MAXIMUM_BLOCKSIZE 8096
#define BDL_BLOCKSIZE_DIVISOR 256

/* Default header pad 2kB */
#define BDL_DEFAULT_HEADER_PAD (256*1024)
#define BDL_MINIMUM_HEADER_PAD 1024
#define BDL_HEADER_PAD_DIVISOR 256

/*
 * Hint blocks are spread around on the device and tells us where we wrote
 * the last block. The hint block after an area contains information about
 * the last block written between it and the previous hint block. The hint
 * blocks are not initialized before used the first write, and if an invalid
 * hint block is found, we assume unused space and start to write directly
 * after the previous hint block.
 */

/* Default hint block spacing is 128MB, one is also placed at the very end */
#define BDL_DEFAULT_HINT_BLOCK_SPACING (128 * 1024 * 1024)

/* For devices smaller than 256MB, use up to four blocks plus one at the end */
#define BDL_SMALL_SIZE_THRESHOLD (256 * 1024 * 1024)
#define BDL_DEFAULT_HINT_BLOCK_COUNT_SMALL 4

#define BDL_DEFAULT_PAD_CHAR 0xff

#define BDL_NEW_DEVICE_BLANK_START_SIZE 1024

#endif
