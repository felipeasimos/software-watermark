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

// stack
STACK* stack_create(unsigned long max_nodes);
void stack_push(STACK*, unsigned long);
unsigned long stack_pop(STACK*);
void stack_pop_until(STACK*, unsigned long size);
unsigned long stack_get(STACK*);
void stack_free(STACK*);
void stack_print(STACK*);

// binary sequence
uint8_t get_bit(uint8_t* data, unsigned long idx);
void set_bit(uint8_t* data, unsigned long idx, uint8_t value);
void invert_binary_sequence(uint8_t* data, unsigned long size);
unsigned long get_first_positive_bit_index(uint8_t* data, unsigned long size_in_bytes);
uint8_t* get_sequence_from_bit_arr(uint8_t* bit_arr, unsigned long n_bits, unsigned long* num_bytes);

#endif
