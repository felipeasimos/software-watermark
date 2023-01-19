#ifndef ENCODER_H
#define ENCODER_H

#include "graph/graph.h"
#include "utils/utils.h"

GRAPH* watermark2014_encode(void* data, unsigned long data_len);
GRAPH* watermark_encode(void* data, unsigned long data_len);

GRAPH* watermark2014_rs_encode(void* data, unsigned long data_len, unsigned long num_parity_symbols);
GRAPH* watermark_rs_encode(void* data, unsigned long data_len, unsigned long num_parity_symbols);

#endif
