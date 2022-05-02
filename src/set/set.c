#include "set/set.h"

SET* set_create(uint8_t copy, unsigned long (*hash)(void* key, unsigned long key_len)) {

    SET* set = malloc(sizeof(SET));
    set->hashmap = hashmap_create(copy, 0, hash );
    return set;
}

void set_add(SET* set, void* data, unsigned long data_len) {

    hashmap_set(set->hashmap, data, data_len, NULL, 0);
}

void set_free(SET* set) {

    hashmap_free(set->hashmap);
    free(set);
}

uint8_t set_contains(SET* set, void* data, unsigned long data_len) {

    return !!hashmap_get(set->hashmap, data, &data_len);
}

void set_remove(SET* set, void* data, unsigned long data_len) {

    hashmap_destroy(set->hashmap, data, data_len);
}
