#ifndef GRAPH_H
#define GRAPH_H

#include<stdlib.h>
#include<string.h>
#include<stdio.h>

struct CONNECTION; //just so we can point to CONNECTION struct before including connection.h

//a node from the graph, with a simple linked list of its connections and with a pointer to the next node in the list
typedef struct GRAPH{

	void* data;		
	unsigned int data_len; //variables that store actual information

	struct CONNECTION* connections; //start of simple linked list of connections
	struct GRAPH* next; //next node in the node double linked list
	struct GRAPH* prev; //previous node in the node double linked list
}GRAPH;

#include "connection/connection.h" //we can't include it before 'struct CONNECTION;'

//creates empty graph
GRAPH* graph_empty();

//allocate memory for graph data
void graph_alloc(GRAPH*, unsigned int);

//create node with data
GRAPH* graph_create(void* data, unsigned int data_len);

//properly deallocate memory of pointers in the struct (except pointer to other nodes)
//and free given pointer
void graph_free_node(GRAPH*); //depends on: connection_free()

//insert graph_to_insert in graph_root list
//if graph_root is NULL, graph_to_insert->prev points to nothing
//if graph_to_insert is NULL, graph_root->next points to nothing (handling its sucessor pointer)
void graph_insert(GRAPH* graph_root, GRAPH* graph_to_insert);

//search for graph node with given data in the given GRAPH list
GRAPH* graph_search(GRAPH*, void* data, unsigned int data_len);

//return connection from graph_from node to graph_to node
//bascially a wrapper around connection_search()
//also return NULL if no connection is found
CONNECTION* graph_connection(GRAPH* graph1, GRAPH* graph2); //depends on: connection_search()

//close the connection from graph_from to graph_to if it exists
//it does nothing to graph_to connection (connection struct)
void graph_oriented_disconnect(GRAPH* graph_from, GRAPH* graph_to);//depends on: connection_delete()

//create a connection from graph_from to graph_to
//it does not create a connection from graph_to to graph_from
void graph_oriented_connect(GRAPH* graph_from, GRAPH* graph_to); //depends on:  connection_insert()

//close connection between two graph nodes,
//making so them don't point to each other anymore
//if they don't have a connection to begin with, nothing happens
void graph_disconnect(GRAPH*, GRAPH*); //depends on: graph_oriented_disconnect()

//connect two graphs together,
//turning them into connections
//of one another
void graph_connect(GRAPH*,GRAPH*); //depends on: graph_oriented_connect()

//delete all connections this node has with other nodes
//(graph_to_isolate will still be part of the simple linked list, but will have no connections and no
//other node is going to have it as connection)
void graph_isolate(GRAPH* graph_to_isolate); //depends on: graph_disconnect()

//delete graph_to_delete node from graph
//returns graph_to_delete antecessor, and if the antecessor is NULL
//return sucessor, if its also NULL, then just return NULL ¯\_(ツ)_/¯
GRAPH* graph_delete(GRAPH* graph_to_delete); //depends on: graph_isolate()

//free the whole graph, properly deallocationg everything
void graph_free(GRAPH*); //depends on: graph_delete()

//print graph node
//given function takes in 'data' and 'data_len' attributes from node
//and should print them as you like
//if no function is given, the data is printed as if it was an unsigned int
void graph_print_node(GRAPH*, void (*)(void*, unsigned int));

//print the whole graph
//basically calls graph_print_node a lot
//print_func functions as in graph_print_node
void graph_print(GRAPH*, void (*)(void*, unsigned int)); //depends on: graph_print_node()

//check if graph node is isolated, returns 1 if it is, 0 otherwise
short graph_check_isolated(GRAPH*);

//how many nodes points to graph_node
unsigned int graph_order_to(GRAPH* graph_root, GRAPH* graph_node);

//how many nodes graph_node points to
unsigned int graph_order_from(GRAPH* graph_root, GRAPH* graph_node);

//return number of neighbors of this node 
unsigned int graph_order(GRAPH* graph_root, GRAPH* graph_node);

#endif
