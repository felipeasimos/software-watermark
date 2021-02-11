#include "connection/connection.h"

CONNECTION* connection_create(){

	//allocate memory for struct
	CONNECTION* connection = malloc(sizeof(CONNECTION));

	//set all values inside struct to zero
	memset( connection, 0x00, sizeof(CONNECTION) );

	//set weight to one by default
	connection->weight = 1;

	return connection;
}

void connection_insert(CONNECTION* connection_root, GRAPH* new_connection){

	//if one of the arguments is NULL nothing happens	
	if( !connection_root || !new_connection ) return;

	//create new connection struct
	CONNECTION* new_connection_node = connection_create();

	//make it point to new_connection
	new_connection_node->node = new_connection;

	//set new connection as connection_root's sucessor and
	//connection_root->next's antecessor
	new_connection_node->next = connection_root->next;
	connection_root->next = new_connection_node;
}

CONNECTION* connection_search(CONNECTION* connection_node, GRAPH* graph_node){

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

void connection_delete(CONNECTION* connection_node, GRAPH* graph_node){

	//if one of the given arguments is NULL, nothing happens
	if( !connection_node || !graph_node ) return;

	for(;
	connection_node->next;	//iterates through connections until current_connection
	connection_node = connection_node->next ){

		if( connection_node->next->node == graph_node ){

			CONNECTION* tmp = connection_node->next; //save found node
			connection_node->next = tmp->next; //points to found node sucessor
			free(tmp); //deallocate found node's memory
			return; //end function here
		}
	}
}

void connection_free(CONNECTION* connection_root){

	//if connection_root is NULL, nothing happens
	if( !connection_root ) return;

	//delete all nodes after connection_root until there is none left
	while( connection_root->next ) connection_delete(connection_root, connection_root->next->node);

	//free connection_root
	free( connection_root );
}

void connection_print(CONNECTION* connection, void(*print_func)(void*,unsigned int)){

	//iterate through list and print each node the connections represent
	for(; connection; connection = connection->next) graph_print_node(connection->node, print_func);
}
