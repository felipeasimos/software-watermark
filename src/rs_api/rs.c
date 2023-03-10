#include "rs_api/rs.h"
#include "utils/utils.h"
#include "rs_api/rslib.h"

struct rs_control* get_rs_struct(int symbol_size, int parity_len) {

  int gfpoly, fcr, prim;
  switch(symbol_size) {
    case 3:
      gfpoly = 0x3;
      fcr = 1;
      prim = 1;
      break;
    default:
    case 8:
      gfpoly = 0x187;
      fcr = 0;
      prim = 1;
  }

	// parity_len == nroots
  #if defined(_OPENMP)
    #pragma omp critical
    {
  #endif
  struct rs_control* rs = init_rs(symbol_size, gfpoly, fcr, prim, parity_len);

  if(!rs) {
    fprintf(stderr, "'init_rs' returned %p\n", (void*)rs);
    exit(EXIT_FAILURE);
  }
  return rs;
  # if defined(_OPENMP)
    }
  #endif
}

// give data, data_len and parity_len, get parity back
void rs_encode8(uint8_t* data, int data_len, uint8_t* parity, int number_of_parity_symbols) {

	struct rs_control* rs = get_rs_struct(8, number_of_parity_symbols);

  uint16_t parity16[number_of_parity_symbols];
  memset(parity16, 0x00, sizeof(uint16_t) * number_of_parity_symbols);

  #if defined(_OPENMP)
    #pragma omp critical
  #endif
	encode_rs8(rs, data, data_len, parity16, 0);
  for(int i = 0; i < number_of_parity_symbols; i++) parity[i] = parity16[i];

  #if defined(_OPENMP)
    #pragma omp critical
  #endif
	free_rs(rs);
}

void rs_encode(uint8_t* data, int data_len, uint16_t* parity, int number_of_parity_symbols, int symsize) {
  struct rs_control* rs = get_rs_struct(symsize, number_of_parity_symbols);

  #if defined(_OPENMP)
    #pragma omp critical
  #endif
	encode_rs8(rs, data, data_len, parity, 0);

  #if defined(_OPENMP)
    #pragma omp critical
  #endif
  free_rs(rs);
}

// give data, data_len, parity and parity_len, data array is changed to correct errors
// if errors can't be corrected, -1 is returned, otherwise number of errors is returned (can be 0)
int rs_decode8(uint8_t* data, int data_len, uint8_t* parity, int number_of_parity_symbols) {

	struct rs_control* rs = get_rs_struct(8, number_of_parity_symbols);

  uint16_t parity16[number_of_parity_symbols];
  for(int i = 0; i < number_of_parity_symbols; i++) parity16[i] = parity[i];

  int numerr;
  #if defined(_OPENMP)
    #pragma omp critical
  #endif
	numerr = decode_rs8(rs, data, parity16, data_len, NULL, 0, NULL, 0, NULL);

  #if defined(_OPENMP)
    #pragma omp critical
  #endif
	free_rs(rs);

	return numerr == -74 ? -1 : numerr;
}

int rs_decode(uint8_t* data, int data_len, uint16_t* parity, int number_of_parity_symbols, int symsize) {

	struct rs_control* rs = get_rs_struct(symsize, number_of_parity_symbols);

  int numerr;
  #if defined(_OPENMP)
    #pragma omp critical
  #endif
	numerr = decode_rs8(rs, data, parity, data_len, NULL, 0, NULL, 0, NULL);

  #if defined(_OPENMP)
    #pragma omp critical
  #endif
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

uint8_t* remove_rs_code(uint8_t* data, unsigned long data_len, unsigned long* num_parity_symbols, int symsize) {

    unsigned long num_parity_bits = (*num_parity_symbols) * symsize;
    unsigned long num_parity_bytes = num_parity_bits / 8 + !!(num_parity_bits % 8);
    unsigned long original_size = data_len - num_parity_bytes;

    // put every parity symbol in their separate uint16_t element
    uint16_t parity[*num_parity_symbols];
    unsigned long num_bytes = *num_parity_symbols;
    unmerge_arr(data + original_size, &num_bytes, sizeof(uint16_t), symsize, (void**)&parity);

    if( rs_decode(data, original_size, parity, *num_parity_symbols, symsize) != -1 ) {
        *num_parity_symbols = original_size;
        return realloc(data, original_size);
    } else {
        free(data);
        return NULL;
    }
}
