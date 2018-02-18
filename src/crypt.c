#include <stdio.h>
#include <stdint.h>

#include "crypt.h"
#include "crc32.h"

#define BDL_DEBUG_HASHING

const char *algorithm_names[] = BDL_HASH_ALGORITHM_NAMES;

const char *crypt_get_algorithm_name (BDL_HASH_ALGORITHM algorithm) {
	if (algorithm > BDL_HASH_ALGORITHM_MAX || *algorithm_names[algorithm] == '\0') {
		fprintf (stderr, "Unknown algorithm number %u while getting it's name\n", algorithm);
		return NULL;
	}
	return algorithm_names[algorithm];
}

int crypt_hash_data(const char *data, int length, BDL_HASH_ALGORITHM algorithm, uint32_t *dest) {
	const char *algorithm_name = crypt_get_algorithm_name (algorithm);

	if (algorithm_name == NULL) {
		return 1;
	}

	if (algorithm == BDL_HASH_ALGORITHM_CRC32) {
		*dest = crc32buf (data, length);
	}
	else {
		fprintf (stderr, "Bug: Unknown algorithm\n");
		return 1;
	}

#ifdef BDL_DEBUG_HASHING
	printf ("Hashed %i bytes of data, result was: %u", length, *dest);
	for (int i = 0; i < length; i++) {
		printf ("%s%02x-", (i % 32 == 0 ? "\n" : ""), (unsigned char) data[i]);
	}
	printf ("\n");
#endif

	return 0;
}

int crypt_check_hash(const char *data, int length, BDL_HASH_ALGORITHM algorithm, uint32_t hash, int *result) {
	uint32_t test;

	if (crypt_hash_data(data, length, algorithm, &test) != 0) {
		fprintf (stderr, "Hashing of data failed while testing checksum");
		return 1;
	}

	*result = hash - test;

	return 0;
}
