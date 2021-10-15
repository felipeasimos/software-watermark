#include "connection/connection.h"

CONNECTION* connection_create(NODE* parent, NODE* from, NODE* to) {

	//allocate memory for struct
	CONNECTION* connection = malloc(sizeof(CONNECTION));
	memset( connection, 0x00, sizeof(CONNECTION) );

	//set weight to one by default
    connection->parent = parent;
    connection->from = from;
    connection->to = to;

	return connection;
}

void connection_insert(CONNECTION* root, CONNECTION* new_connection) {

    if(root->next) root->next->prev = new_connection;
    new_connection->next = root->next;
    root->next = new_connection;
    new_connection->prev = root;
}

void connection_insert_in_neighbour(CONNECTION* connection_root, NODE* node) {

    CONNECTION* conn = connection_create(connection_root->parent, node, connection_root->parent);
    connection_insert(connection_root, conn);
}

void connection_insert_out_neighbour(CONNECTION* connection_root, NODE* node) {

    CONNECTION* conn = connection_create(connection_root->parent, connection_root->parent, node);
    connection_insert(connection_root, conn);
}

CONNECTION* connection_search_in_neighbour(CONNECTION* connection_node, NODE* graph_node){

	//if one of the arguments is NULL nothing happens
	if( !connection_node || !graph_node ) return NULL;

	for(;
	connection_node;	//iterates through connections until current_connection is NULL
	connection_node = connection_node->next ){

		//return current_connection if it is graph_connection
		if( connection_node->from == graph_node ) return connection_node;
	}

	//no connection equivalent to graph_connection, so return NULL
	return NULL;
}

CONNECTION* connection_search_out_neighbour(CONNECTION* connection_node, NODE* graph_node){

	//if one of the arguments is NULL nothing happens
	if( !connection_node || !graph_node ) return NULL;

	for(;
	connection_node;	//iterates through connections until current_connection is NULL
	connection_node = connection_node->next ){

		//return current_connection if it is graph_connection
		if( connection_node->to == graph_node ) return connection_node;
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

uint8_t connection_delete_in_neighbour(CONNECTION* conn_root, NODE* node) {

    CONNECTION* conn = connection_search_in_neighbour(conn_root, node);
    connection_delete(conn);
    return !!conn;
}

uint8_t connection_delete_out_neighbour(CONNECTION* conn_root, NODE* node) {

    CONNECTION* conn = connection_search_out_neighbour(conn_root, node);
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
        node_print(connection->to, print_func);
        printf(" ");
    }
}
