#include "connection/connection.h"
#include "node/node.h"

CONNECTION* connection_create(NODE* parent, NODE* node) {

	//allocate memory for struct
	CONNECTION* connection = malloc(sizeof(CONNECTION));

    connection->next = connection->prev = NULL;
    connection->parent = parent;
    connection->node = node;

	return connection;
}

void connection_insert(CONNECTION* root, CONNECTION* new_connection) {

    new_connection->next = root;
    root->prev = new_connection;
    new_connection->prev = NULL;
    if(root->parent->in == root) {
        root->parent->in = new_connection;
    } else if(root->parent->out == root) {
        root->parent->out = new_connection;
    }
}

void connection_insert_neighbour(CONNECTION* connection_root, NODE* node) {

    CONNECTION* conn = connection_create(connection_root->parent, node);
    connection_insert(connection_root, conn);
}

CONNECTION* connection_search_neighbour(CONNECTION* connection_node, NODE* graph_node){

	//if one of the arguments is NULL nothing happens
	if( !connection_node || !graph_node ) return NULL;

	for(;
	connection_node;	//iterates through connections until current_connection is NULL
	connection_node = connection_node->next ){

		//return current_connection if it is graph_connection
		if( connection_node->node == graph_node ) return connection_node;
	}

	//no connection equivalent to graph_connection, so return NULL
	return NULL;
}

void connection_delete(CONNECTION* conn) {

    if(!conn) return;
    if(conn->prev) conn->prev->next = conn->next;
    if(conn->next) conn->next->prev = conn->prev;

    // in case this is the root pointer
    if( conn->parent->out == conn ) {
        conn->parent->out = conn->next;
    } else if( conn->parent->in == conn ) {
        conn->parent->in = conn->next;
    }
    free(conn);
}

uint8_t connection_delete_neighbour(CONNECTION* conn_root, NODE* node) {

    CONNECTION* conn = connection_search_neighbour(conn_root, node);
    connection_delete(conn);
    return !!conn;
}

void connection_free(CONNECTION* conn){

	if( !conn ) return;

    CONNECTION* conn_next = conn;
    for(CONNECTION* conn = conn_next; conn; conn = conn_next) {

        conn_next = conn_next->next;
        free(conn);
    } 
}

void connection_print(CONNECTION* connection, void(*print_func)(FILE*, NODE*)){

	//iterate through list and print each node the connections represent
	for(; connection; connection = connection->next) {
        node_print(connection->node, print_func);
        printf(" ");
    }
}

uint8_t is_hamiltonian(CONNECTION* conn) {
  return conn->parent->graph_idx + 1 == conn->node->graph_idx;
}

CONNECTION* conn_next(CONNECTION* conn) {
  if(conn->next) return conn->next;
  // iterate over nodes
  for(NODE* next_node = graph_get(conn->parent->graph, conn->parent->graph_idx + 1); next_node; next_node = graph_get(next_node->graph, next_node->graph_idx + 1)) {
    if(next_node->out) return next_node->out;
  }
  return NULL;
}

CONNECTION* conn_next_non_hamiltonian_edge(CONNECTION* conn) {
  do {
    conn = conn_next(conn);
  } while(conn && is_hamiltonian(conn));
  return conn;
}
