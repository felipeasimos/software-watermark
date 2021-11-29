#ifndef SET_H
#define SET_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct HASHMAP;

typedef struct SET {
    struct HASHMAP* hashmap;
} SET;

#include "hashmap/hashmap.h"

SET* set_create(uint8_t copy, unsigned long (*hash)(void* key, unsigned long key_len));
void set_add(SET*, void* data, unsigned long data_len);
void set_free(SET* set);
uint8_t set_contains(SET*, void* data, unsigned long data_len);
void set_remove(SET*, void* data, unsigned long data_len);

#endif
