#include "hashmap/hashmap.h"

unsigned long djb2(uint8_t* key, unsigned long key_len) {

    unsigned long hash = 5381;
    for(unsigned long i = 0; i < key_len; i++) {
        hash = ((hash << 5) + hash) + key[i];
    }
    return hash;
}

unsigned long hashmap_get_idx(HASHMAP* hashmap, void* key, unsigned long key_len) {

    // get fractional part of hashed key and multiply by the hash size
    double n;
    return HASHMAP_SIZE * (modf(hashmap->hash_function(key, key_len) * HASHMAP_MULTIPLICATION_CONSTANT, &n));
}

HASHMAP* hashmap_create(uint8_t copy_key, uint8_t copy_data, unsigned long (*hash)(void*, unsigned long)) {

    HASHMAP* hashmap = malloc(sizeof(HASHMAP));
    // find a power of 2 to use as the hash table size
    hashmap->copy_key = copy_key;
    hashmap->copy_data = copy_data;
    hashmap->hash_function = hash ? hash : (unsigned long(*)(void*, unsigned long))djb2;
    hashmap->hashmap = calloc(HASHMAP_SIZE, sizeof(HASHMAP_NODE*));
    for(unsigned long i = 0; i < HASHMAP_SIZE; i++) hashmap->hashmap[i] = NULL;
    return hashmap;
}

HASHMAP_NODE* hashmap_find(HASHMAP* hashmap, void* key, unsigned long key_len) {

    unsigned long idx = hashmap_get_idx(hashmap, key, key_len);
    HASHMAP_NODE* node = hashmap->hashmap[idx];
    while(node && (key_len != node->key_len || memcmp(node->key, key, key_len))) {
        node = node->next;
    }
    return node;
}

void* hashmap_get(HASHMAP* hashmap, void* key, unsigned long* len) {

    HASHMAP_NODE* node = hashmap_find(hashmap, key, *len);
    if(node) {
        *len = node->data_len;
        return node->data;
    }
    return NULL;
}

HASHMAP_NODE* hashmap_create_node(HASHMAP* hashmap, void* key, unsigned long key_len, void* data, unsigned long data_len) {

    HASHMAP_NODE* node = malloc(sizeof(HASHMAP_NODE));
    if(hashmap->copy_data) {
        void* copy_data = malloc(data_len);
        memcpy(copy_data, data, data_len);
        data = copy_data;
    }
    if(hashmap->copy_key) {
        void* copy_key = malloc(key_len);
        memcpy(copy_key, key, key_len);
        key = copy_key;
    }
    node->data = data;
    node->data_len = data_len;
    node->key = key;
    node->key_len = key_len;
    node->prev = node->next = NULL;
    return node;
}

void hashmap_set(HASHMAP* hashmap, void* key, unsigned long key_len, void* data, unsigned long data_len) {

    unsigned long idx = hashmap_get_idx(hashmap, key, key_len);
    HASHMAP_NODE* node = hashmap->hashmap[idx];
    // if there isn't root node
    if(!node) {
        // create root node
        hashmap->hashmap[idx] = hashmap_create_node(hashmap, key, key_len, data, data_len);
    // if there is a root node
    } else {
        // find node before the one that matches, or the last node
        while(node->next && (key_len != node->next->key_len || memcmp(node->next->key, key, key_len))) {
            node = node->next;
        }
        // if there is a node that matches (this isn't the last node)
        if( node->next ) {
            node->data = data;
            node->data_len = data_len;
        // if this is the last node (we don't have a match), create new one
        } else {
            node->next = hashmap_create_node(hashmap, key, key_len, data, data_len);
        }
    }
}

void hashmap_destroy(HASHMAP* hashmap, void* key, unsigned long key_len) {

    HASHMAP_NODE* node = hashmap_find(hashmap, key, key_len);
    if(node) {
        // if this is a root node
        if(!node->prev && !node->next) {
            unsigned long idx = hashmap_get_idx(hashmap, key, key_len);
            hashmap->hashmap[idx] = node->next;
        // if this isn't a root node
        } else {
            node->prev->next = node->next;
        }
        if(node->next) node->next->prev = node->prev;
        free(node);
    }
}

void hashmap_free(HASHMAP* hashmap) {
    for(unsigned long i = 0; i < HASHMAP_SIZE; i++) {
        for(HASHMAP_NODE* node = hashmap->hashmap[i]; node; node = hashmap->hashmap[i]) {

            hashmap->hashmap[i] = node->next;
            if(hashmap->copy_data) free(node->data);
            if(hashmap->copy_key) free(node->key);
            free(node);
        };
    }
    free(hashmap->hashmap);
    free(hashmap);
}
