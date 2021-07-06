#ifndef DECODER_H
#define DECODER_H

#include "utils/utils.h"
#include <stdint.h>
#include "rs_api/rs.h"
#include "time.h"

void decoder_graph_to_utils_nodes(GRAPH* graph);

void decoder_print_node(void* data, unsigned long data_len);

void* watermark2014_decode(GRAPH* graph, unsigned long* num_bytes);

void* watermark2014_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_uint16s);

void* watermark2017_decode(GRAPH* graph, unsigned long* num_bytes);

void* watermark2017_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_uint16s);

void* watermark2017_decode_analysis(GRAPH* graph, unsigned long* num_bytes);

void* watermark2017_decode_analysis_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_uint16s);

#endif
