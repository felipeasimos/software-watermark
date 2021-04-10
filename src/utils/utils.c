#include "utils/utils.h"

uint8_t is_backedge(GRAPH* node) {
	return node->data_len;
}

void pop_all(PSTACK* stack, unsigned long idx) {

	stack->n = idx+1;
}

void pop_all_history(PSTACK* stack, unsigned long size) {

	stack->n = size;
}

GRAPH* find_guaranteed_forward_edge(GRAPH* node) {
	// if we have a forward edge, we have two nodes connected:
	GRAPH* node1 = node->connections->node;
	GRAPH* node2 = node->connections->next->node;

	// in this case we have, for sure, a forward edge among us, and two possibilities:
	// 1. we have a removed hamiltonian edge afterwards, which means that both nodes
	// have only one connection: one has a backedge and the other has a hamiltonian edge.
	if( is_backedge(node1->connections->node) || is_backedge(node2->connections->node) ) {
		return is_backedge(node1->connections->node) ? node2 : node1;
	} else {
		// 2. we have a forward edge and no missing hamiltonian edges, so this means that
		// one node (the forward edge one) is the next hamiltonian node of the other
		return connection_search(node1->connections, node2) ? node2 : node1;
	}
}

uint8_t is_graph_structure_valid(GRAPH* graph) {

	if( !graph ) return 0;
	
	// 1. first node has only a hamiltonian edge, and no less
	if( !graph->connections || graph->connections->next ) return 0;

	uint8_t node_without_connections_found=0;

	for(; graph; graph = graph->next) {
	
		// 2. only final node has no outgoing connections (so there can only be one node without outgoing connections)
		if( !graph->connections ) {
			if( node_without_connections_found ) return 0;
			else node_without_connections_found++;
		}

		// 3. every node has less than three outgoing connections
		if( graph->connections && graph->connections->next && graph->connections->next->next ) return 0;

		// 4. no connections to the same node
		if( connection_search(graph->connections, graph) ) return 0;
	}

	return 1;
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

uint8_t get_bit(uint8_t* data, unsigned long bit_idx) {

	// 0x80 = 0b1000_0000
	uint8_t byte = data[bit_idx/8];
	return byte & ( 0x80 >> (bit_idx % 8));
}

void set_bit( uint8_t* data, unsigned long i, uint8_t value ) {

	uint8_t* byte = &data[i/8];

	// 0x80 = 0b1000_0000
	if( value ) {
		*byte |= 0x80 >> (i%8);
	} else {
		*byte &= ~(0x80 >> (i%8));
	}
}

// the nodes we passed througth must be marked with an data_len != 0
GRAPH* get_backedge(GRAPH* node) {

	for(CONNECTION* conn = node->connections; conn; conn = conn->next) {

		if( is_backedge(conn->node) ) return conn->node;
	}
	return NULL;
}

GRAPH* get_forward_edge(GRAPH* node) {

	if( !node || !node->connections ) return node;

	// if we have only one connection, there is no forward edge
	if( !node->connections->next ) {
		return NULL;
	} else {
		// if we have 2 connections, one is hamiltonian path and the other
		// is a forward edge of backedge
		CONNECTION* conn1 = node->connections;
		CONNECTION* conn2 = node->connections->next;

		if( is_backedge(conn1->node) || is_backedge(conn2->node) ) {
			return NULL;
		} else {
			return find_guaranteed_forward_edge(node);
		}	
	}
}

CONNECTION* get_hamiltonian_connection(GRAPH* node) {

	if( !node || !node->connections ) return NULL;
 
	// if there is only one connection, it is either a backedge or a hamiltonian
	if( !node->connections->next ) {
		return is_backedge(node->connections->node) ? NULL : node->connections;
	} else {
		// if there is two, one is a backedge or a forward edge, while the other
		// is the hamiltonian
		CONNECTION* conn1 = node->connections;
		CONNECTION* conn2 = node->connections->next;

		if( is_backedge(conn1->node) || is_backedge(conn2->node) ) {
			return is_backedge(conn1->node) ? conn2 : conn1;
		} else {
			return find_guaranteed_forward_edge(node)->connections == conn1 ? conn2 : conn1;
		}
	}
}

// the nodes we haven't passed througth must be marked with a data_len == 0
GRAPH* get_next_hamiltonian_node(GRAPH* node) {

	if( !node ) return NULL;

	CONNECTION* conn = get_hamiltonian_connection(node);

	// we might have a removed hamiltonian edge, which means we only have a backedge
	return conn ? conn->node : get_forward_edge(node->connections->node);
}

GRAPH* get_next_hamiltonian_node2014(GRAPH* node) {

	if( !node ) return NULL;

	for(CONNECTION* conn = node->connections; conn; conn = conn->next) {

		if( !is_backedge(conn->node) ) return conn->node;
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
