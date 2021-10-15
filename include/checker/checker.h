#ifndef CHECKER_H
#define CHECKER_H

#include "decoder/decoder.h"

// return true if it matches
uint8_t watermark_check(GRAPH* graph, void* data, unsigned long num_bytes);
uint8_t watermark_rs_check(GRAPH* graph, void* data, unsigned long num_bytes, unsigned long num_parity_symbols);

// return bit array, in which the values can be '1', '0' or 'x' (for unknown)
void* watermark_check_analysis(GRAPH* graph, void* data, unsigned long* num_bytes);
void* watermark_rs_check_analysis(GRAPH* graph, void* data, unsigned long* num_bytes, unsigned long num_parity_symbols);

#endif
