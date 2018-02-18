#ifndef BDL_CRYPT_H
#define BDL_CRYPT_H

#include <stdint.h>

#define BDL_HASH_ALGORITHM_CRC32 0

#define BDL_HASH_ALGORITHM_MAX 0

#define BDL_DEFAULT_HASH_ALGORITHM BDL_HASH_ALGORITHM_CRC32

#define BDL_HASH_ALGORITHM_NAMES {	\
		"CRC32",					\
		""							\
}

typedef uint8_t BDL_HASH_ALGORITHM;

int crypt_hash_data(const char *data, int length, BDL_HASH_ALGORITHM algorithm, uint32_t *dest);
int crypt_check_hash(const char *data, int length, BDL_HASH_ALGORITHM algorithm, uint32_t hash, int *result);

#endif
