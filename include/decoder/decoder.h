#ifndef DECODER_H
#define DECODER_H

#include "graph/graph.h"
#include <stdint.h>
#include "rs_api/rs.h"

void* watermark_decode(GRAPH* graph, unsigned long* num_bytes);

void* watermark_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_bytes);

#endif
