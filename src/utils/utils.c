#include "utils/utils.h"

uint8_t is_backedge(GRAPH* node) {
	return node->data_len;
}

GRAPH* search_for_equal_node(CONNECTION* conn1, CONNECTION* conn2) {

	for(; conn1; conn1 = conn1->next) {
		for(; conn2; conn2 = conn2->next) {
			if( conn1->node == conn2->node ) {
				return conn1->node;
			}
		}
	}
	return NULL;
}

void pop_all(PSTACK* stack, unsigned long idx) {

	stack->n = idx+1;
}

void pop_all_history(PSTACK* stack, unsigned long size) {

	stack->n = size;
}

void create_stacks(STACKS* stacks, unsigned long n_bits) {

	stacks->odd.stack = malloc(sizeof(GRAPH*) * (n_bits/2+1));
	stacks->even.stack = malloc(sizeof(GRAPH*) * (n_bits/2));
	stacks->history.stack = malloc(sizeof(unsigned long) * n_bits);
	stacks->history.n = 0;
	stacks->odd.n = 0;
	stacks->even.n = 0;
}

void free_stacks(STACKS* stacks) {

	free(stacks->odd.stack);
	free(stacks->even.stack);
	free(stacks->history.stack);

	stacks->odd.stack = NULL;
	stacks->even.stack = NULL;
	stacks->history.stack = NULL;
}


// the nodes we passed througth must be marked with an data_len != 0
GRAPH* get_backedge(GRAPH* node) {

	if( !node ) return NULL;

	for(CONNECTION* conn = node->connections; conn; conn = conn->next) {

		if( is_backedge(conn->node) ) return conn->node;
	}
	return NULL;
}

GRAPH* get_forward_edge(GRAPH* node) {

	if( !node ) return NULL;

	// the complexity of the algorithm itself is like O(n³), which is pretty bad,
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

	if( !node || !node->connections ) return NULL;

	GRAPH* forward_edge = get_forward_edge(node);

	for(CONNECTION* conn = node->connections; conn; conn = conn->next) {

		if( !is_backedge(conn->node) &&
				( !forward_edge || conn->node != forward_edge ) ) return conn->node;
	}
	return NULL;
}

PSTACK* get_parity_stack(STACKS* stacks, uint8_t is_odd) {
	return is_odd ? &stacks->odd : &stacks->even;
}

void add_node_to_stacks(STACKS* stacks, GRAPH* node, unsigned long is_odd) {

	// save size of the stack with different parity
	stacks->history.stack[stacks->history.n++] = is_odd ? stacks->even.n : stacks->odd.n;

	// save to stack with the same parity
	PSTACK* stack = get_parity_stack(stacks, is_odd);
	stack->stack[stack->n++] = node;
}

// 0 based idx
void pop_stacks(STACKS* stacks, unsigned long backedge_p_idx, unsigned long backedge_h_idx) {

	// get parity from hamiltonian idx	
	uint8_t is_odd = !( backedge_h_idx & 1 );
	PSTACK* dest_stack = get_parity_stack(stacks, is_odd);
	PSTACK* not_dest_stack = get_parity_stack(stacks, !is_odd);

	pop_all(dest_stack, backedge_p_idx);
	pop_all_history(not_dest_stack, stacks->history.stack[ backedge_h_idx ]);
}

void add_backedge(STACKS* stacks, GRAPH* source_node, uint8_t bit, uint8_t is_odd) {

	uint8_t is_dest_odd = bit ? !is_odd : is_odd;

	PSTACK* dest_stack = get_parity_stack(stacks, is_dest_odd);

	if( dest_stack->n ) {
		unsigned long dest_idx = rand() % dest_stack->n;	
		GRAPH* dest_node = dest_stack->stack[ dest_idx ];	
		graph_oriented_connect(source_node, dest_node);
		pop_stacks(stacks, dest_idx, (*(unsigned long*)dest_node->data) - 1);
	}
}
