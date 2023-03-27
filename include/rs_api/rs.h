#ifndef RS_H
#define RS_H

#include <stdint.h>
#include <stdlib.h>

// give data, data_len and parity_len, get parity back
void rs_encode(uint8_t* data, int data_len, uint8_t* parity, int number_of_parity_symbols, int symsize);

int rs_decode(uint8_t* result, int data_len, int number_of_parity_symbols, int symsize);

void* append_rs_code8(void* data, unsigned long* data_len, unsigned long num_parity_symbols); // return n_bytes
void* append_rs_code(void* data, unsigned long* num_data_symbols, unsigned long num_parity_symbols, unsigned long symsize); // returns n_bits

// 'num_parity_symbols' will hold the original data sequence size
uint8_t* remove_rs_code8(uint8_t* data, unsigned long data_len, unsigned long* num_parity_symbols);
uint8_t* remove_rs_code(uint8_t* data, unsigned long num_data_symbols, unsigned long num_parity_symbols, int symsize);

#endif
