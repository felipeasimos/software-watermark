#ifndef GRAPH_H
#define GRAPH_H

#include <stdint.h>

#include "utils/utils.h"

struct NODE;

typedef struct GRAPH {

    struct NODE** nodes;
    unsigned long num_nodes;
    unsigned long num_connections;
} GRAPH;

#include "node/node.h"

// create graph with N empty nodes;
GRAPH* graph_create(unsigned long num_nodes);

// free graph and all structures in it
void graph_free(GRAPH* graph);

// print graph
void graph_print(GRAPH* graph, void (*print_func)(NODE*, void*, unsigned long));

// add node to graph
void graph_add(GRAPH* graph);

// delete node
void graph_delete(NODE* node);

// connect node to another
void graph_oriented_connect(NODE* from, NODE* to);

// sort topologically
void graph_topological_sort(GRAPH*);

// generate png image with the graph
// uses 'system' syscall
void graph_write_dot(GRAPH*, const char* filename);

// copy the graph
GRAPH* graph_copy(GRAPH*);

// serialize the graph
void* graph_serialize(GRAPH*, unsigned long* num_bytes);

// deserialize the graph
GRAPH* graph_deserialize(void*, unsigned long num_bytes);

#endif
