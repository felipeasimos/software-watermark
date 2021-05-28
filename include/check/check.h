#ifndef CHECK_H
#define CHECK_H

#include "graph/graph.h"
#include "utils/utils.h"
#include <stdint.h>

uint8_t watermark_check(GRAPH* graph, void* data, unsigned long num_bytes);

#endif
