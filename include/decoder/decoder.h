#ifndef DECODER_H
#define DECODER_H

#include "graph/graph.h"
#include <stdint.h>

void* watermark_decode(GRAPH* graph, unsigned long* num_bytes);

#endif
