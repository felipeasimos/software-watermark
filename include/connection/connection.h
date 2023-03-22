#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdio.h>
#include <stdint.h>

typedef struct NODE NODE; //just so we can point to NODE struct before including node.h

//connections, pointing to the next connection on the simple linked list
typedef struct CONNECTION{

    struct NODE* parent; // node this connection struct belongs to
    struct NODE* node; // node this connection points to or comes from (depends if this is an IN or OUT connection)
    struct CONNECTION* next;
    struct CONNECTION* prev;
} CONNECTION;

//create empty connection
CONNECTION* connection_create(NODE* parent, NODE* node);

void connection_insert(CONNECTION* connection, CONNECTION* conn);
void connection_insert_neighbour(CONNECTION* connection_root, NODE* node);

CONNECTION* connection_search_neighbour(CONNECTION* connection_node, NODE* graph_node);

void connection_delete(CONNECTION* conn);
uint8_t connection_delete_neighbour(CONNECTION* connection_node, NODE* graph_node);

void connection_free(CONNECTION* connection_root);

void connection_print(CONNECTION* connection, void (*)(FILE*, NODE*));

uint8_t is_hamiltonian(CONNECTION* conn);
CONNECTION* conn_next(CONNECTION* conn);
CONNECTION* conn_next_non_hamiltonian_edge(CONNECTION* conn);

#endif
