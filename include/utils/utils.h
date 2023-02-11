#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

#include "rs_api/rs.h"

typedef struct STACK {

    unsigned long* stack;
    unsigned long n;
} STACK;

typedef struct QUEUE {
    unsigned long* queue;
    unsigned long n;
} QUEUE;

typedef enum STATUS_BIT {
    BIT_0='0',
    BIT_1='1',
    BIT_MUTE='m',
    BIT_0_BACKEDGE='b',
    BIT_1_BACKEDGE='B',
    BIT_1_FORWARD_EDGE_AND_BIT_0='f',
    BIT_1_FORWARD_EDGE_AND_BIT_1='F',
    BIT_UNKNOWN='x'
} STATUS_BIT;

typedef struct UTILS_NODE {
    unsigned long backedge_idx;
    STATUS_BIT checker_bit;
    unsigned long bit_idx;
} UTILS_NODE;

#include "graph/graph.h"

// system
uint8_t is_little_endian_machine();

// math
unsigned long ceil_power_of_2(unsigned long n);

// stack
STACK* stack_create(unsigned long max_nodes);
void stack_push(STACK*, unsigned long);
unsigned long stack_pop(STACK*);
void stack_pop_until(STACK*, unsigned long size);
unsigned long stack_get(STACK*);
void stack_free(STACK*);
void stack_print(STACK*);

// queue
QUEUE* queue_create(unsigned long max_nodes);
void queue_push(QUEUE*, unsigned long);
unsigned long queue_pop(QUEUE*);
unsigned long queue_get(QUEUE*);
void queue_free(QUEUE*);

// binary sequence
uint8_t get_bit(uint8_t* data, unsigned long idx);
void set_bit(uint8_t* data, unsigned long idx, uint8_t value);
uint8_t* invert_binary_sequence(uint8_t* data, unsigned long size);
uint8_t* invert_byte_sequence(uint8_t* data, unsigned long size);
unsigned long get_first_positive_bit_index(uint8_t* data, unsigned long size_in_bytes);
uint8_t* get_sequence_from_bit_arr(uint8_t* bit_arr, unsigned long n_bits, unsigned long* num_bytes);
uint8_t binary_sequence_equal(uint8_t* data1, uint8_t* data2, unsigned long num_bytes1, unsigned long num_bytes2);
void* encode_numeric_string(char* string, unsigned long* data_len);
void* decode_numeric_string(void* data, unsigned long* data_len);
unsigned long get_number_of_left_zeros(uint8_t* data, unsigned long data_len);
unsigned long get_number_of_right_zeros(uint8_t* data, unsigned long data_len);
void remove_left_zeros(uint8_t* data, unsigned long* data_len);
void merge_arr(void* data, unsigned long* data_len, unsigned long element_size, unsigned long symbol_size);
void unmerge_arr(void* data, unsigned long num_symbols, unsigned long element_size, unsigned long symbol_size, void* res);

// 2017 codec-specific
uint8_t has_possible_backedge(STACK* possible_backedges, GRAPH* graph, unsigned long current_idx);
unsigned long get_backedge_index(STACK* possible_backedges, GRAPH* graph, unsigned long current_idx);

// rs
void* append_rs_code8(void* data, unsigned long* data_len, unsigned long num_parity_symbols);
void* append_rs_code(void* data, unsigned long* data_len, unsigned long num_parity_symbols, unsigned long symsize);

#endif
