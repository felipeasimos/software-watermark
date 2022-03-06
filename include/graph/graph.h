#ifndef GRAPH_H
#define GRAPH_H

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define MAX_NODE_NAME_SIZE 4096
#define MAX_NODE_NAME_SIZE_STR STRINGIFY(MAX_NODE_NAME_SIZE)

#include <stdint.h>

struct NODE;

typedef struct GRAPH {

    struct NODE** nodes;
    unsigned long num_nodes;
    unsigned long num_connections;
} GRAPH;

#include "set/set.h"
#include "node/node.h"
#include "utils/utils.h"
#include "dijkstra/dijkstra.h"
#include "hashmap/hashmap.h"

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
void graph_write_hamiltonian_dot_generic(GRAPH*, const char* filename, const char* label, void (*print_func)(FILE*, NODE*));

// label can be NULL
void graph_write_hamiltonian_dot(GRAPH*, const char* filename, const char* label);

// label can be NULL
void graph_write_dot_generic(GRAPH*, const char* filename, const char* label, void (*print_func)(FILE*, NODE*));

// label can be NULL
void graph_write_dot(GRAPH*, const char* filename, const char* label);

// copy the graph
GRAPH* graph_copy(GRAPH*);

// copy the graph and all the data inside
GRAPH* graph_deep_copy(GRAPH*);

// serialize the graph
void* graph_serialize(GRAPH*, unsigned long* num_bytes);

// deserialize the graph
GRAPH* graph_deserialize(uint8_t*);

// create graph from dot file
// dot files should only use "a -> b" syntax to connect nodes
// only nodes with connections will be considered
GRAPH* graph_create_from_dot(FILE* f);

#endif
