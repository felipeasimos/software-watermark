#include "rs_api/rs.h"

struct rs_control* get_rs_struct(int parity_len) {

	int symbol_size = 8;
	int gfpoly = 0x187;
	int fcr = 0;
	int prim = 1;

	// parity_len == nroots
	return init_rs(symbol_size, gfpoly, fcr, prim, parity_len);
}

// give data, data_len and parity_len, get parity back
void rs_encode(uint8_t* data, int data_len, uint16_t* parity, int number_of_parity_symbols) {

	struct rs_control* rs = get_rs_struct(number_of_parity_symbols);

	encode_rs8(rs, data, data_len, parity, 0);

	free_rs(rs);
}

// give data, data_len, parity and parity_len, data array is changed to correct errors
// if errors can't be corrected, -1 is returned, otherwise number of errors is returned (can be 0)
int rs_decode(uint8_t* data, int data_len, uint16_t* parity, int number_of_parity_symbols) {

	struct rs_control* rs = get_rs_struct(number_of_parity_symbols);

	int numerr = decode_rs8(rs, data, parity, data_len, NULL, 0, NULL, 0, NULL);

	free_rs(rs);

	return numerr == -74 ? -1 : numerr;
}

// 'num_parity_symbols' will hold the original data sequence size
uint8_t* remove_rs_code(uint8_t* data, unsigned long data_len, unsigned long* num_parity_symbols) {

    unsigned long original_size = data_len - sizeof(uint16_t) * (*num_parity_symbols);
    if( rs_decode(data, original_size, (uint16_t*)(data+original_size), *num_parity_symbols) != -1 ) {
        *num_parity_symbols = original_size;
        return realloc(data, original_size);
    } else {
        free(data);
        return NULL;
    }
}
