#ifndef BDL_CRC32_H
#define BDL_CRC32_H

#include <stdint.h>

uint32_t crc32buf (const char *buf, int len);
int crc32cmp (const char *buf, int len, uint32_t crc32);

#endif
