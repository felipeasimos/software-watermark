#ifndef DECODER_H
#define DECODER_H

#include "utils/utils.h"
#include <stdint.h>
#include "rs_api/rs.h"

void* watermark2014_decode(GRAPH* graph, unsigned long* num_bytes);

void* watermark2014_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_bytes);

unsigned long watermark_num_edges(GRAPH* graph);

unsigned long watermark_num_hamiltonian_edges2014(GRAPH* graph);

unsigned long watermark_num_hamiltonian_edges(GRAPH* graph);

unsigned long watermark_num_nodes(GRAPH* graph);

unsigned long watermark_cyclomatic_complexity(GRAPH* graph);

#endif
