#ifndef DS_LIST_DOUBLE_H
#define DS_LIST_DOUBLE_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "data/data.h"

typedef struct DLIST {

	struct DLIST* prev;
	struct DLIST* next;

	DATA data;
} DLIST;

DLIST* dlist_empty();
DLIST* dlist_init(DLIST* prev, DLIST* next, void* data, unsigned long data_len);

void dlist_node_free(DLIST* node);
void dlist_free(DLIST* dlist);

DLIST* dlist_begin(DLIST* dlist);
DLIST* dlist_end(DLIST* dlist);
DLIST* dlist_search(DLIST* dlist, void* data, unsigned long data_len);

DLIST* dlist_node_link(DLIST* prev, DLIST* next);
DLIST* dlist_link(DLIST* l, void* data, unsigned long data_len);

DLIST* dlist_node_push(DLIST* dlist, DLIST* node);
DLIST* dlist_push(DLIST* dlist, void* data, unsigned long data_len);

DLIST* dlist_node_remove(DLIST* dlist, DLIST* node);
DLIST* dlist_remove(DLIST* dlist, void* data, unsigned long data_len);

#endif
