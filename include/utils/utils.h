#ifndef UTILS_H
#define UTILS_H

#include "graph/graph.h"
#include <limits.h>

GRAPH* get_backedge(GRAPH* node);

GRAPH* get_forward_edge(GRAPH* node);

GRAPH* get_next_hamiltonian_node(GRAPH* node);

#endif
