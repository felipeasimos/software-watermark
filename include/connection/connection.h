#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdio.h>
#include <string.h>

struct GRAPH; //just so we can point to GRAPH struct before including graph.h

//connections, pointing to the next connection on the simple linked list
typedef struct CONNECTION{

	double weight;
	struct CONNECTION* next; //next connection in the original node's simple linked list of connections
	struct GRAPH* node; //node this connection represents
    struct GRAPH* parent; // node this connection comes from
} CONNECTION;

#include "graph/graph.h" //we can't include it before 'struct GRAPH;'

//create empty connection
CONNECTION* connection_create(GRAPH* graph);

//insert new node in list
//making it the connection_root's sucessor
//(sets connection_root's previous sucessor properly)
void connection_insert_node(CONNECTION* connection_root, GRAPH* new_connection);
void connection_insert(CONNECTION* connection, CONNECTION* conn);

//search for node pointing to graph_node, starting from connection_node
//if none is found, return NULL
CONNECTION* connection_search_node(CONNECTION* connection_node, GRAPH* graph_node);

//search for and delete node pointing to graph_node, starting from connection_node
//if connection_node points to graph_node nothing happens
void connection_delete_node(CONNECTION* connection_node, GRAPH* graph_node);

void connection_delete(CONNECTION* connection_node, CONNECTION* conn);

    //free list (don't free GRAPH and the structs point to)
void connection_free(CONNECTION* connection_root); //depends on: connection_delete_node()

//print list
//given print function can be NULL to print unsigned long by default
void connection_print(CONNECTION* connection, void (*)(void*, unsigned long)); //depends on: graph_print_node()

// return number of connections in this list of connections
unsigned long connection_num(CONNECTION* connection);

#endif
