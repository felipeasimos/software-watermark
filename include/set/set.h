#ifndef SET_H
#define SET_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct SET_DATA {

    void* data;
    unsigned long data_len;
    struct SET_DATA* next;
} SET_DATA;

typedef struct SET {
    SET_DATA* set;
    unsigned long n;
    uint8_t copy;
} SET;

SET* set_create(uint8_t copy);
void set_add(SET*, void* data, unsigned long data_len);
void set_free(SET* set);
uint8_t set_contains(SET*, void* data, unsigned long data_len);
void* set_pop(SET*, unsigned long* data_len);
void set_remove(SET*, void* data, unsigned long data_len);

#endif
