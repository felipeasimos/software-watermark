#ifndef DECODER2017_H
#define DECODER2017_H

#include "graph/graph.h"
#include <stdint.h>
#include "rs_api/rs.h"

void* watermark2017_decode(GRAPH* graph, unsigned long* num_bytes);

void* watermark2017_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_bytes);

#endif
