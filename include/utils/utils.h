#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <limits.h>

struct NODE;

typedef struct PSTACK {

    struct NODE** stack;
    unsigned long n;
} PSTACK;

typedef struct HSTACK {

    unsigned long* stack;
    unsigned long n;
} HSTACK;

PSTACK* pstack_create(unsigned long max_nodes);
void pstack_push(PSTACK*, struct NODE*);
struct NODE* pstack_pop(PSTACK*);
struct NODE* pstack_get(PSTACK*);
void pstack_free(PSTACK*);

HSTACK* hstack_create(unsigned long max_nodes);
void hstack_push(HSTACK*, unsigned long);
unsigned long hstack_pop(HSTACK*);
unsigned long hstack_get(HSTACK*);
void hstack_free(HSTACK*);

#endif
