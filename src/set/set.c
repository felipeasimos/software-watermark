#include "set/set.h"

SET* set_create(uint8_t copy) {

    SET* set = malloc(sizeof(SET));
    set->set = NULL;
    set->n = 0;
    set->copy = copy;
    return set;
}

void set_add(SET* set, void* data, unsigned long data_len) {

    if(set_contains(set, data, data_len)) return;
    SET_DATA* node = malloc(sizeof(SET_DATA));
    if(set->copy) {
        void* copy_data = malloc(data_len);
        memcpy(copy_data, data, data_len);
        data = copy_data;
    }
    node->data = data;
    node->data_len = data_len;
    node->next = set->set;

    set->set = node;
    set->n++;
}

void set_free(SET* set) {

    SET_DATA* node_next = NULL;
    for(SET_DATA* node = set->set; node; node = node_next) {
        node_next = node->next;
        if(set->copy) free(node->data);
        free(node);
    }
    free(set);
}

uint8_t set_contains(SET* set, void* data, unsigned long data_len) {

    for(SET_DATA* node = set->set; node; node = node->next) {

        if(data_len == node->data_len && !memcmp(node->data, data, data_len)) return 1;
    }
    return 0;
}

void* set_pop(SET* set, unsigned long* data_len) {

    if(!set->n) return NULL;
    SET_DATA* node = set->set;
    set->set = node->next;

    void* data = node->data;
    *data_len = node->data_len;
    free(node);
    set->n--;
    return data;
}

void set_remove(SET* set, void* data, unsigned long data_len) {

    if(!set->n) return;
    // iterate until final node or the node before the node we want to remove
    SET_DATA* node = set->set;
    for(; node->next && data_len != node->next->data_len && memcmp(node->next->data, data, data_len); node = node->next);
    
    if(!node->next) return;

    SET_DATA* to_remove = node->next;
    node->next = to_remove->next;
    free(to_remove);
    set->n--;
}
