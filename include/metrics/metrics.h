#ifndef METRICS_H
#define METRICS_H

#include "graph/graph.h"
#include "utils/utils.h"

unsigned long watermark_num_edges(GRAPH* graph);

unsigned long watermark_num_hamiltonian_edges2014(GRAPH* graph);

unsigned long watermark_num_hamiltonian_edges(GRAPH* graph);

unsigned long watermark_num_nodes(GRAPH* graph);

unsigned long watermark_cyclomatic_complexity(GRAPH* graph);

unsigned long watermark_num_cycles(GRAPH* graph);

#endif
