#ifndef DECODER_H
#define DECODER_H

#include "graph/graph.h"
#include "rs_api/rs.h"

void* watermark2014_decode(GRAPH* graph, unsigned long* num_bytes);
void* watermark_decode(GRAPH*, unsigned long* num_bytes);
void* watermark_decode_improved(GRAPH*, uint8_t* key, unsigned long* num_bytes);

// 'num_parity_symbols' will hold the size of the binary sequence
// afterwards
void* watermark2014_rs_decode(GRAPH*, unsigned long* num_parity_symbols);
void* watermark_rs_decode(GRAPH*, unsigned long* num_parity_symbols);
void* watermark_rs_decode_improved(GRAPH*, void* key, unsigned long* num_bytes, unsigned long num_parity_symbols);

void* watermark_decode_analysis(GRAPH*, unsigned long* num_bytes);
void* watermark_rs_decode_analysis(GRAPH*, unsigned long* num_parity_symbols);

#endif
