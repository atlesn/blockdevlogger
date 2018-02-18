=== BLOCK DEVICE LOGGER README ===

== ABOUT ==

Block Device Logger which writes and reads directly to or from a
block device (e.g. USB-memory) without using a filesystem. It can
also use a fixed size file. BDL tries to minimize the strain on
individual parts of a device by using the whole device in a
round-robin fashion, with metadata to aid searcing spread around
at fixed points.

A BDL structure consist of a header at the start of the device
which is only written to when first initializing the device. Fixed
size blocks of data are then written after this header. A region
consists of many blocks (usually 128MB) before ending with a hint
block which contains information about where to find the most recent
block before it. The hint blocks and other blocks are not pre-
initialized, and BDL merely considers any block with invalid checksum
to be free space.

The data blocks have a timestamp and a user defined identifier. When
writing a new entry, BDL searches the hint blocks to find unused
space. If the device was full, the oldest data block is overwritten,
and the rest of this region is also invalidated. The hint blocks are
always updated on writes.

== INITIALIZATION ==

To initialize a device, the user must first overwrite the start of it
with zeros. This is to prevent accidental overwrites of filesystems.
It is possible to choose which block size to use, and data written
has to be less than the block size minus the block header size.

Only the master header is written at initialization, the rest of the
structure is created dynamically on data writes. If an existing
structure is present on a device, it can be used if the block size
happen to match the new blocksize. To prevent this, the hint blocks
has to be overwritten.

== CHECKSUMS ==

All blocks are checksummed with CRC32. If the master header is
corrupted, no operations may be performed. If a hint block or
data block is corrupted, it is considered free space.

== COMMANDS ==

bdl init dev={DEVICE} [bs=BLOCKSIZE] [hpad=HEADER PADDING] [padchar=BYTE IN HEX]

Initializes a device by writing a new header.

	dev		Device or file to initialize
	bs		The fixed size of blocks and hint blocks,
			must be dividable by 256
	hpad	The size of the master header, can be used to change the
			position of hint blocks if desirable.
	padchar	The character to use for padding blocks, defaults to 0xff.
			Correct value relieves strain on some memory chips.

bdl write {DEVICE} {DATA}

Write a new data block.
