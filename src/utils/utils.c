#include "utils/utils.h"

uint8_t is_backedge(GRAPH* node) {
	return node->data_len;
}

GRAPH* search_for_equal_node(CONNECTION* conn1, CONNECTION* conn2) {

	for(; conn1; conn1 = conn1->next)
		for(; conn2; conn2 = conn2->next)
			if( conn1->node == conn2->node )
				return conn2->node;
	return NULL;
}

// the nodes we passed througth must be marked with an data_len != 0
GRAPH* get_backedge(GRAPH* node) {
	for(CONNECTION* conn = node->connections; conn; conn = conn->next) {

		if( is_backedge(conn->node) ) return conn->node;
	}
	return NULL;
}

GRAPH* get_forward_edge(GRAPH* node) {

	// the complexity of the algorithm itself is like n³, which is pretty bad,
	// but since we can assure that a node will have at most 3 connections
	// coming out of it, its okay.

	// follow each connection and get the nodes that are 1 nodes apart from
	// 'node'. If one of them match one that is directly connected to 'node',
	// that one is the forward edge destination
	for(CONNECTION* conn = node->connections; conn; conn = conn->next) {

		GRAPH* forward_edge_destination = search_for_equal_node(conn, conn->node->connections);
		if( forward_edge_destination ) return forward_edge_destination;
	}

	return NULL;
}

// the nodes we haven't passed througth must be marked with a data_len == 0
GRAPH* get_next_hamiltonian_node(GRAPH* node) {

	GRAPH* forward_edge = get_forward_edge(node);

	for(CONNECTION* conn = node->connections; conn; conn = conn->next) {

		if( !is_backedge(conn->node) && conn->node != forward_edge ) return conn->node;
	}
	return NULL;
}
