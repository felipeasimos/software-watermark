#ifndef ENCODER_H
#define ENCODER_H

#include "graph/graph.h"

GRAPH* watermark2014_encode(void* data, unsigned long data_len);
GRAPH* watermark_encode8(void* data, unsigned long data_len);
GRAPH* watermark_encode(void* data, unsigned long n_bits);

GRAPH* watermark2014_rs_encode(void* data, unsigned long data_len, unsigned long num_parity_symbols);
GRAPH* watermark_rs_encode8(void* data, unsigned long data_len, unsigned long num_parity_symbols);
GRAPH* watermark_rs_encode(void* data, unsigned long num_data_symbols, unsigned long num_parity_symbols, unsigned long symsize);

#endif
