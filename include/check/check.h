#ifndef CHECK_H
#define CHECK_H

#include "graph/graph.h"
#include "utils/utils.h"
#include "rs_api/rs.h"
#include <stdint.h>

void checker_graph_to_utils_nodes(GRAPH* graph);

uint8_t watermark_check(GRAPH* graph, void* data, unsigned long num_bytes);

// num_bytes is used to give size and receive result size
uint8_t* watermark_check_analysis(GRAPH* graph, void* data, unsigned long* num_bytes);

// num_bytes is used to give size and receive result size
uint8_t* watermark_check_analysis_with_rs(GRAPH* graph, void* data, unsigned long* num_bytes, unsigned long num_rs_parity_symbols);

#endif
