#include "rs_api/rs.h"

struct rs_control* get_rs_struct(int symbol_size, int parity_len) {

	int gfpoly = 0x187;
	int fcr = 0;
	int prim = 1;

	// parity_len == nroots
	return init_rs(symbol_size, gfpoly, fcr, prim, parity_len);
}

// give data, data_len and parity_len, get parity back
void rs_encode8(uint8_t* data, int data_len, uint8_t* parity, int number_of_parity_symbols) {

	struct rs_control* rs = get_rs_struct(8, number_of_parity_symbols);

  uint16_t parity16[number_of_parity_symbols];
  memset(parity16, 0x00, sizeof(uint16_t) * number_of_parity_symbols);
	encode_rs8(rs, data, data_len, parity16, 0);
  for(int i = 0; i < number_of_parity_symbols; i++) parity[i] = parity16[i];

	free_rs(rs);
}

// give data, data_len, parity and parity_len, data array is changed to correct errors
// if errors can't be corrected, -1 is returned, otherwise number of errors is returned (can be 0)
int rs_decode8(uint8_t* data, int data_len, uint8_t* parity, int number_of_parity_symbols) {

	struct rs_control* rs = get_rs_struct(8, number_of_parity_symbols);

  uint16_t parity16[number_of_parity_symbols];
  for(int i = 0; i < number_of_parity_symbols; i++) parity16[i] = parity[i];
	int numerr = decode_rs8(rs, data, parity16, data_len, NULL, 0, NULL, 0, NULL);

	free_rs(rs);

	return numerr == -74 ? -1 : numerr;
}

// 'num_parity_symbols' will hold the original data sequence size
uint8_t* remove_rs_code8(uint8_t* data, unsigned long data_len, unsigned long* num_parity_symbols) {

    unsigned long original_size = data_len - (*num_parity_symbols);
    if( rs_decode8(data, original_size, (uint8_t*)(data+original_size), *num_parity_symbols) != -1 ) {
        *num_parity_symbols = original_size;
        return realloc(data, original_size);
    } else {
        free(data);
        return NULL;
    }
}
