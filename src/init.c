#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "init.h"
#include "defaults.h"
#include "blocks.h"
#include "crypt.h"
#include "io.h"

int check_blank_device (const char *device_path) {
	FILE *device = fopen(device_path, "r");

	if (device == NULL) {
		fprintf (stderr, "Could not open device %s for reading: %s\n", device_path, strerror(errno));
		return 1;
	}

	int item_count = BDL_NEW_DEVICE_BLANK_START_SIZE / sizeof(int);
	int buf[item_count];
	size_t items = fread (buf, sizeof(buf[0]), item_count, device);

	if (items < item_count) {
		if (feof(device)) {
			fprintf (stderr, "Device %s was too small\n", device_path);
			goto error_close;
		}
		int error = ferror(device);
		fprintf (stderr, "Error while reading from device %s: %s\n", device_path, strerror(error));
		goto error_close;
	}

	for (int i = 0; i < item_count; i++) {
		if (buf[i] != 0) {
			fprintf (stderr,
					"Device needs to be pre-initialized with %i bytes of zeros. Try running 'dd if=/dev/zero of=%s count=%i'\n",
					BDL_NEW_DEVICE_BLANK_START_SIZE, device_path, BDL_NEW_DEVICE_BLANK_START_SIZE
			);
			goto error_close;
		}
	}

	success:
	fclose (device);
	return 0;

	error_close:
	fclose(device);
	return 1;
}

int init_dev(const char *device_path, long int blocksize, long int header_pad, char padchar) {
	if (check_blank_device(device_path)) {
		return 1;
	}

	struct bdl_header header;
	memset (&header, '\0', sizeof(header));

	int pad_size = header_pad - sizeof(header);
	char header_pad_string[pad_size];
	memset (header_pad_string, padchar, pad_size);

	strncpy(header.header_begin_message, BDL_CONFIG_HEADER_START, 32);
	header.version_major = BDL_CONFIG_VERSION_MAJOR;
	header.version_minor = BDL_CONFIG_VERSION_MINOR;
	header.block_size = blocksize;
	header.pad_character = padchar;
	header.hash = 0;
	header.default_hash_algorithm = BDL_DEFAULT_HASH_ALGORITHM;
	header.total_size = 0;

	struct io_file file;
	if (io_open(device_path, "r+", &file) != 0) {
		fprintf (stderr, "Failed to open device %s\n", device_path);
		return 1;
	}

	if (file.size < (header_pad + blocksize * 2)) {
		fprintf(stderr, "The total size will be too small, minimum size is %ld\n", (header_pad + blocksize * 2));
		io_close(&file);
		return 1;
	}

	header.total_size = (file.size - header_pad - ((file.size - header_pad) % blocksize));

	uint32_t hash;
	if (crypt_hash_data((const char *) &header, sizeof(header), header.default_hash_algorithm, &hash)) {
		fprintf (stderr, "Hashing of header failed\n");
		io_close(&file);
		return 1;
	}

	header.hash = hash;

	int write_result = io_write_block(&file, 0, (const char *) &header, sizeof(header), header_pad_string, pad_size, 1);

	io_close(&file);

	if (write_result != 0) {
		fprintf (stderr, "Failed to write header to device %s\n", device_path);
		return 1;
	}

	return 0;
}
