#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

typedef struct STACK {

    unsigned long* stack;
    unsigned long n;
} STACK;

typedef struct QUEUE {
    unsigned long* queue;
    unsigned long n;
} QUEUE;

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

// 2017 codec-specific
uint8_t has_possible_backedge(STACK* possible_backedges, GRAPH* graph, unsigned long current_idx);
unsigned long get_backedge_index(STACK* possible_backedges, GRAPH* graph, unsigned long current_idx);

#endif
