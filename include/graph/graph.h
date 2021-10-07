#ifndef GRAPH_H
#define GRAPH_H

#include <stdint.h>

struct NODE;

typedef struct GRAPH {

    struct NODE** nodes;
    unsigned long num_nodes;
    unsigned long num_connections;
} GRAPH;

#include "node/node.h"
#include "utils/utils.h"
#include "dijkstra/dijkstra.h"

// create graph with N empty nodes;
GRAPH* graph_create(unsigned long num_nodes);

// get node, returns NULL if out of bounds
NODE* graph_get(GRAPH* graph, unsigned long i);

// free graph and all structures in it
void graph_free(GRAPH* graph);

// print graph
void graph_print(GRAPH* graph, void (*print_func)(FILE*, NODE*));

// add node to graph
void graph_add(GRAPH* graph);

// insert node at index
void graph_insert(GRAPH* graph, unsigned long idx);

// swap the index of two nodes
void graph_swap(NODE*, NODE*);

// remove all connections that this node is a part of
void graph_isolate(NODE* node);

// delete node
void graph_delete(NODE* node);

// connect node to another
void graph_oriented_connect(NODE* from, NODE* to);

// check if nodes are connected
CONNECTION* graph_get_connection(NODE* from, NODE* to);

// return backedge connection (that goes to a node with lower index)
// if it exists
CONNECTION* graph_get_backedge(NODE* node);

// return forward edge connection (that goes to a node with greater index)
// if it exists
CONNECTION* graph_get_forward(NODE* node);

// disconnect node from another
// returns true if connection existed
uint8_t graph_oriented_disconnect(NODE* from, NODE* to);

// sort topologically, ignoring back edges
void graph_topological_sort(GRAPH*);

// unload info from all nodes
void graph_unload_info(GRAPH*);

// unload all info from all nodes
void graph_unload_all_info(GRAPH*);

// free info from all nodes
void graph_free_info(GRAPH*);

// free all info from all nodes
void graph_free_all_info(GRAPH*);

// label can be NULL
void graph_write_hamiltonian_dot(GRAPH*, const char* filename, const char* label);

// label can be NULL
void graph_write_dot(GRAPH*, const char* filename, const char* label);

// copy the graph
GRAPH* graph_copy(GRAPH*);

// serialize the graph
void* graph_serialize(GRAPH*, unsigned long* num_bytes);

// deserialize the graph
GRAPH* graph_deserialize(uint8_t*);

#endif
