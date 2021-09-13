#ifndef DECODER_H
#define DECODER_H

#include "graph/graph.h"

uint8_t* watermark2014_decode(GRAPH* graph, unsigned long* num_bytes);
uint8_t* watermark_decode(GRAPH*, unsigned long* num_bytes);

#endif
