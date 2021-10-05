#ifndef NODE_H
#define NODE_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

struct CONNECTION; //just so we can point to CONNECTION struct before including connection.h
struct GRAPH; //we put this here so we can point to GRAPH before including graph.h

typedef struct NODE {

	void* data;		
	unsigned long data_len; //variables that store actual information
    unsigned long num_info; // number of info structures used before the actual data

    unsigned long graph_idx; // this node's index in the graph structure
    struct GRAPH* graph;

    struct CONNECTION* in;
	struct CONNECTION* out;
    unsigned long num_connections;
    unsigned long num_out_neighbours;
    unsigned long num_in_neighbours;
} NODE;

typedef struct INFO_NODE {

    // value of data, stored previously in the node
    void* data;
    unsigned long data_len;

    // info used in a process
    void* info;
    unsigned long info_len;
} INFO_NODE;

#include "connection/connection.h" //we can't include it before 'struct CONNECTION;'
#include "graph/graph.h"

//creates empty node
NODE* node_empty();

//allocate memory for node data
void node_alloc(NODE*, unsigned long);

//overwrite existing data and add new one
NODE* node_set_data(NODE* node, void* data, unsigned long data_len);

// get data, bypassing all info structures
void* node_get_data(NODE* node);

// get data_len, bypassing all info structures
unsigned long node_get_data_len(NODE* node); 

// free node structure and its connections
void node_free(NODE*);

uint8_t node_oriented_disconnect(NODE* node_from, NODE* node_to);

void node_oriented_connect(NODE* node_from, NODE* node_to);

CONNECTION* node_get_connection(NODE* node_from, NODE* node_to);

void node_disconnect(NODE*, NODE*);

void node_connect(NODE*,NODE*);

void node_write(NODE*, FILE*, void (*)(FILE*, NODE*));

void node_print(NODE*, void (*)(FILE*, NODE*));

void node_load_info(NODE* node, void* info, unsigned long info_len);

void node_unload_info(NODE* node);

void node_unload_all_info(NODE* node);

void node_free_info(NODE* node);

void node_free_all_info(NODE* node);

void* node_get_info(NODE* node);

unsigned long node_get_info_len(NODE* node);

void node_transfer_out_connections(NODE* from, NODE* to);
void node_transfer_in_connections(NODE* from, NODE* to);

void node_expand_to_sequence(NODE*);
void node_expand_to_repeat(NODE*);
void node_expand_to_while(NODE*);
void node_expand_to_if_then(NODE*);
void node_expand_to_if_then_else(NODE*);
void node_expand_to_p_case(NODE*, unsigned long p);

#endif
