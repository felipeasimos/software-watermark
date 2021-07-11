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
} HSTACK;

typedef struct STACKS {

	PSTACK odd;
	PSTACK even;
	HSTACK history;
} STACKS;

// keep some information about the node that can
// be used by tests
typedef struct UTILS_NODE {

    // hamiltonian idx 1-based
    unsigned long h_idx;

    // bit arr idx 0-based
    unsigned long bit_idx;

    // '1'/'0' or 'm' if mute or 'x' if unknown
    char bit;
} UTILS_NODE;
#define UTILS_MUTE_NODE 'm'
#define UTILS_UNKNOWN_NODE 'x'

void utils_print_node(void* data, unsigned long data_len);

void utils_print_stacks(STACKS* stacks, void (*print_func)(void* data, unsigned long data_len));

unsigned long invert_unsigned_long(unsigned long i);

// 1. first node has only a hamiltonian edge, and no less
// 2. only final node has no outgoing connections (so there can only be one node without outgoing connections)
// 3. every node has less than three outgoing connections
// 4. no connections to the same node
uint8_t is_graph_structure_valid(GRAPH* graph);

void create_stacks(STACKS* stacks, unsigned long n_bits);

void free_stacks(STACKS* stacks);

uint8_t get_bit(uint8_t* data, unsigned long bit_idx);

void set_bit( uint8_t* data, unsigned long i, uint8_t value );

unsigned long get_num_bits2014(GRAPH* graph);

// also free the data of each node
unsigned long get_num_bits(GRAPH* graph);

GRAPH* get_backedge(GRAPH* node);

GRAPH* get_backedge_with_info(GRAPH* node);


GRAPH* get_forward_edge(GRAPH* node);

GRAPH* get_forward_edge_with_info(GRAPH* node);

CONNECTION* get_hamiltonian_connection(GRAPH* node);

GRAPH* get_next_hamiltonian_node(GRAPH* node);

GRAPH* get_next_hamiltonian_node2014(GRAPH* node);

PSTACK* get_parity_stack(STACKS* stacks, uint8_t is_odd);

void add_node_to_stacks(STACKS* stacks, GRAPH* node, unsigned long h_idx, unsigned long is_odd);

// 0 based idx
void pop_stacks(STACKS* stacks, unsigned long backedge_p_idx, unsigned long backedge_h_idx);

void add_backedge2014(STACKS* stacks, GRAPH* source_node, uint8_t bit, uint8_t is_odd);

void add_backedge(STACKS* stacks, GRAPH* source_node, uint8_t prev_has_backedge_in_this_stack, uint8_t bit, uint8_t is_odd);

unsigned long get_trailing_zeroes(uint8_t* data, unsigned long data_len);

void* encode_numeric_string(char* string, unsigned long* data_len);

uint8_t* decode_numeric_string(void* data, unsigned long* data_len);

void add_idx(GRAPH* node, unsigned long h_idx, unsigned long bit_idx, char bit);

int has_possible_backedge2017(STACKS* stacks, GRAPH* node, uint8_t is_odd, uint8_t bit);

uint8_t* bin_to_bit_arr(uint8_t* bin, unsigned long* bin_len);

uint8_t* bit_arr_to_bin(uint8_t* bit_arr, unsigned long* bit_arr_len);

#endif
