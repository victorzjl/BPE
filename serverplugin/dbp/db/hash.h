#ifndef HASH_H
#define    HASH_H
#include <stdint.h>

/*typedef  unsigned int uint32_t;
typedef  unsigned short uint16_t;
typedef  */
typedef  unsigned int size_t;
uint32_t hash(const void *key, size_t length, const uint32_t initval);

#define average_hash hash

#endif    /* HASH_H */

