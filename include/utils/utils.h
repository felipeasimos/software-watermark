#ifndef UTILS_H
#define UTILS_H

#include "graph/graph.h"
#include <limits.h>

typedef struct PSTACK {

	GRAPH** stack;
	unsigned long n;
} PSTACK;

typedef struct HSTACK {

	unsigned long* stack;
	unsigned long n;
} HSTACK;

typedef struct STACKS {

	PSTACK odd;
	PSTACK even;
	HSTACK history;
} STACKS;

void create_stacks(STACKS* stacks, unsigned long n_bits);

void free_stacks(STACKS* stacks);

GRAPH* get_backedge(GRAPH* node);

GRAPH* get_forward_edge(GRAPH* node);

GRAPH* get_next_hamiltonian_node(GRAPH* node);

PSTACK* get_parity_stack(STACKS* stacks, uint8_t is_odd);

void add_node_to_stacks(STACKS* stacks, GRAPH* node, unsigned long is_odd);

void add_backedge(STACKS* stacks, GRAPH* source_node, uint8_t bit, uint8_t is_odd);

#endif
