#ifndef RS_H
#define RS_H

#include "rs_api/rslib.h"

// give data, data_len and parity_len, get parity back
void rs_encode(uint8_t* data, int data_len, uint16_t* parity, int number_of_parity_symbols);

// give data, data_len, parity and parity_len, data array is changed to correct errors
// if errors can't be corrected, -1 is returned, otherwise number of errors is returned (can be 0)
int rs_decode(uint8_t* data, int data_len, uint16_t* parity, int number_of_parity_symbols);

#endif
