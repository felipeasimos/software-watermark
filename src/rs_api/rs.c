#include "rs_api/rs.h"

// give data, data_len and parity_len, get parity back
void rs_encode(uint8_t* data, unsigned long data_len, uint16_t* parity, unsigned long parity_len) {

	int symbol_size = 8;
	int gfpoly = 0x187;
	int fcr = 0;
	int prim = 1;

	// get GF(2) primitive polynomial for this 
}

// give data, data_len, parity and parity_len, data array is changed to correct errors
// if errors can't be corrected, -1 is returned, otherwise number of errors is returned (can be 0)
int rs_decode(uint8_t* data, unsigned long data_len, uint16_t* parity, unsigned long parity_len) {

}
