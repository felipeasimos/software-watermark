#include "utils/utils.h"
#include <fec.h>

#define XOR_MASK 0x00

#define show_bits(bits,len) fprintf(stderr, "%s:%d:" #bits ":", __FILE__, __LINE__);\
  for(unsigned long i = 0; i < len; i++) {\
    fprintf(stderr, "%hhu", get_bit(bits, i));\
  }\
  fprintf(stderr, "\n");

struct rs_control* get_rs_struct(int symsize, int num_parity, int data_len) {

  int n = (1 << symsize) - 1;
  int pad = n - num_parity - data_len;
  int gfpoly, fcr, prim;
  switch(symsize) {
    case 2:
    case 3:
    case 4:
    case 6:
    case 7:
      gfpoly = 0x3;
      fcr = 1;
      prim = 1;
      break;
    case 5:
      gfpoly = 0x5;
      fcr = 1;
      prim = 1;
      break;
    default:
    case 8:
      gfpoly = 0x187;
      fcr = 0;
      prim = 1;
      break;
  }

	// parity_len == nroots
  #if defined(_OPENMP)
    #pragma omp critical
    {
  #endif
  void* rs = init_rs_char(symsize, gfpoly, fcr, prim, num_parity, pad);
  if(!rs) {
    fprintf(stderr, "'init_rs' returned %p for symbol_size: %d and parity_len: %d\n", rs, symsize, num_parity);
    if(num_parity + data_len > n) {
      fprintf(stderr, "cause: num_parity + data_len > n. (%d + %d > %d)\n", num_parity, data_len, n);
    }
    exit(EXIT_FAILURE);
  }
  return rs;
  # if defined(_OPENMP)
    }
  #endif
}

void rs_encode(uint8_t* data, int data_len, uint8_t* parity, int num_parity, int symsize) {
  void* rs = get_rs_struct(symsize, num_parity, data_len);

  #if defined(_OPENMP)
    #pragma omp critical
  #endif

  memset(parity, 0x00, num_parity * sizeof(uint8_t));
  encode_rs_char(rs, data, parity);

  #if defined(_OPENMP)
    #pragma omp critical
  #endif
  free_rs_char(rs);
}

#define return_defer(value) do { numerr = value; goto defer; } while(0);

#define decode_erasure(num_erasures) do { \
      memcpy(decoded_data, result, num_symbols); \
      numerr = decode_rs_char(rs, decoded_data, erasures, num_erasures); \
      if(numerr == -1) return_defer(numerr);\
      if(numerr < best_num_errs || best_num_errs == -1) {\
        best_num_errs = numerr;\
        memcpy(best_decoded_data, decoded_data, num_data); \
      }\
      if(!numerr) return_defer(numerr);\
} while(0);

int decode_with_erasure(void* rs, int num_data, int num_parity, uint8_t* data, uint8_t* decoded_data_with_erasure){

  unsigned long num_symbols = num_data + num_parity;
  uint8_t data_copy[num_symbols];
  int erasures[num_symbols];
  int best_num_errs = -1;
  uint8_t best_decoded_data[num_symbols];
  for(unsigned long i = 0; i < num_symbols; i++) {
    memcpy(data_copy, data, num_symbols);
    erasures[0] = i;
    int numerr = decode_rs_char(rs, data_copy, erasures, 1);
    if(numerr < best_num_errs || best_num_errs == -1) {
      best_num_errs = numerr;
      memcpy(best_decoded_data, data_copy, num_symbols);
    }
    if(!best_num_errs) {
      goto defer;
    }
  }
defer:
  memcpy(decoded_data_with_erasure, best_decoded_data, num_symbols);
  return best_num_errs;
}

int rs_decode(uint8_t* result, int num_data, int num_parity, int symsize) {

  void* rs = get_rs_struct(symsize, num_parity, num_data);

  int numerr;
  #if defined(_OPENMP)
    #pragma omp critical
  #endif

  // 1. make copy of data
  unsigned long num_symbols = num_data + num_parity;
  uint8_t best_decoded_data[num_symbols];
  memcpy(best_decoded_data, result, num_symbols);

  // 2. try decoding with no erasures
  numerr = decode_rs_char(rs, best_decoded_data, NULL, 0);

  // 3. unless it is just a matter of using erasure decoding, return
  if(numerr != -2) return_defer(numerr);

defer:
  #if defined(_OPENMP)
    #pragma omp critical
  #endif
	  free_rs_char(rs);

  memcpy(result, best_decoded_data, num_data);

  return numerr;
}

void* append_rs_code8(void* data, unsigned long* data_len, unsigned long num_parity_symbols) {

    uint8_t parity[num_parity_symbols];
    rs_encode(data, *data_len, parity, num_parity_symbols, 8);

    unsigned long new_len = (*data_len) + num_parity_symbols;
    uint8_t* data_with_parity = malloc(new_len);
    memcpy(data_with_parity, data, *data_len);
    memcpy(data_with_parity+(*data_len), parity, num_parity_symbols);
    *data_len = new_len;
    return data_with_parity;
}

void copy_unmerged_arr_to_merged_arr(void* from, void* to, unsigned long num_symbols, unsigned long symbol_size, unsigned long num_from_element_bytes, unsigned long* next_bit) {
  unsigned long offset = num_from_element_bytes * 8 - symbol_size;
  for(unsigned long i = 0; i < num_symbols; i++) {
    void* from_element = ((uint8_t*)from) + ( num_from_element_bytes * i );
    for(unsigned long j = 0; j < symbol_size; j++) {
      set_bit(to, *next_bit, get_bit(from_element, offset + j));
      (*next_bit)++;
    }
  }
}

void copy_merged_arr_to_unmerged_arr(void* from, void* to, unsigned long num_symbols, unsigned long symbol_size, unsigned long num_to_element_bytes, unsigned long* next_bit) {

    unsigned long offset = num_to_element_bytes * 8 - symbol_size;
    for(unsigned long i = 0; i < num_symbols; i++) {
      void* to_element = ((uint8_t*)to) + ( num_to_element_bytes * i );
      for(unsigned long j = 0; j < symbol_size; j++) {
        set_bit(to_element, offset + j, get_bit(from, *next_bit));
        (*next_bit)++;
      }
    }
}

void* append_rs_code(void* data, unsigned long* num_data_symbols, unsigned long num_parity_symbols, unsigned long symsize) {

  // 0. initialize variables
  unsigned long data_bits = (*num_data_symbols) * symsize;
  unsigned long parity_bits = num_parity_symbols * symsize;
  unsigned long total_bits = data_bits + parity_bits;
  unsigned long total_bytes = total_bits / 8 + !!(total_bits % 8);
  unsigned long symbol_bytes = symsize / 8 + !!(symsize % 8);

  uint8_t parity[num_parity_symbols];
  uint8_t* res = NULL;
  // 1. unmerge data symbols and encode
  unmerge_arr(data, *num_data_symbols, symbol_bytes, symsize, (void**)&res);
  rs_encode(res, *num_data_symbols, parity, num_parity_symbols, symsize);
  // 2. alloc memory for data + parity sequence
  uint8_t* data_with_parity = malloc(total_bytes);
  memset(data_with_parity, 0x00, total_bytes);
  // 4. copy data bits
  unsigned long next_bit = 0;
  copy_unmerged_arr_to_merged_arr(res, data_with_parity, *num_data_symbols, symsize, symbol_bytes, &next_bit);
  // 5. copy parity bits
  copy_unmerged_arr_to_merged_arr(parity, data_with_parity, num_parity_symbols, symsize, sizeof(uint8_t), &next_bit);
  // 6. set 'num_data_symbols' to total number of bits of the final sequence
  *num_data_symbols = total_bits;
  free(res);

  return data_with_parity; 
}

// 'num_parity_symbols' will hold the original data sequence size
uint8_t* remove_rs_code8(uint8_t* data, unsigned long data_len, unsigned long* num_parity_symbols) {

    unsigned long original_size = data_len - (*num_parity_symbols);

    if( rs_decode(data, original_size, *num_parity_symbols, 8) != -1 ) {
        *num_parity_symbols = original_size;
        return realloc(data, original_size);
    } else {
        free(data);
        return NULL;
    }
}

uint8_t* remove_rs_code(uint8_t* data, unsigned long num_data_symbols, unsigned long num_parity_symbols, int symsize) {

    // 0. initialize variables
    unsigned long num_data_bits = num_data_symbols * symsize;
    unsigned long num_data_bytes = num_data_bits / 8 + !!(num_data_bits % 8);
    unsigned long symbol_bytes = symsize / 8 + !!(symsize % 8);

    // 1. unmerge all symbols
    uint8_t* res = NULL;
    unmerge_arr(data, num_data_symbols + num_parity_symbols, symbol_bytes, symsize, (void**)&res);

    // 2. decode
    int decode_status = rs_decode(res, num_data_symbols, num_parity_symbols, symsize);
    if( decode_status != -1 ) {
      // 3. merge only the data symbols back
      unsigned long n_bytes = num_data_symbols;
      merge_arr(res, &n_bytes, sizeof(uint8_t), symsize); 
      return realloc(res, num_data_bytes);
    } else {
      free(res);
      return NULL;
    }
}
