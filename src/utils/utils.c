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
	if( ( node1->connections && is_backedge(node1->connections->node) ) || ( node2->connections && is_backedge(node2->connections->node) ) ) {
		return ( node1->connections && is_backedge(node1->connections->node) ) ? node2 : node1;
	} else {
		// 2. we have a forward edge and no missing hamiltonian edges, so this means that
		// one node (the forward edge one) is the next hamiltonian node of the other
		return connection_search_node(node1->connections, node2) ? node2 : node1;
	}
}

void utils_print_node(void* data, unsigned long data_len) {

    if(!data) {
		printf("\x1b[33m null \x1b[0m");
    } else if( data_len == sizeof(UTILS_NODE) ) {

        char bit = ((UTILS_NODE*)data)->bit;
        if( bit == UTILS_MUTE_NODE ) {

            printf("\x1b[33m %lu[%c] \x1b[0m", ((UTILS_NODE*)data)->h_idx, bit);
        } else { 
            printf("\x1b[33m %lu(bit_arr[%lu] = '%c']) \x1b[0m", ((UTILS_NODE*)data)->h_idx, ((UTILS_NODE*)data)->bit_idx, bit);
        }
    }
}

uint8_t is_graph_structure_valid(GRAPH* graph) {

	if( !graph ) return 0;
	
	// 1. first node has only a hamiltonian edge, and no less
	if( !graph->connections || graph->connections->next ) return 0;

	for(; graph; graph = graph->next) {
	
		// 3. every node has less than three outgoing connections
		if( graph->connections && graph->connections->next && graph->connections->next->next ) return 0;

		// 4. no connections to the same node
		if( connection_search_node(graph->connections, graph) ) return 0;
	}

	return 1;
}

void create_stacks(STACKS* stacks, unsigned long n_bits) {

	// allocate extra space in case of mute nodes in 2017
	stacks->odd.stack = malloc(sizeof(GRAPH*) * (n_bits));
	stacks->even.stack = malloc(sizeof(GRAPH*) * (n_bits));
	stacks->history.stack = malloc(sizeof(unsigned long) * n_bits * 2);

	stacks->odd.n = 0;
	stacks->even.n = 0;
}

void free_stacks(STACKS* stacks) {

	free(stacks->odd.stack);
	free(stacks->even.stack);
	free(stacks->history.stack);
}

uint8_t get_bit(uint8_t* data, unsigned long bit_idx) {

	// 0x80 = 0b1000_0000
	uint8_t byte = data[bit_idx/8];
	return !!(byte & ( 0x80 >> (bit_idx % 8)));
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

unsigned long get_num_bits2014(GRAPH* graph) {

	unsigned long i = 0;

	for(GRAPH* node = graph; node; node = node->next) {

		i++;
		free(node->data);
		node->data = NULL;
		node->data_len = 0;
	}

	return i-1;
}

unsigned long get_num_bits(GRAPH* graph) {

	unsigned long i=0;

	get_num_bits2014(graph);

	// count nodes, decrementing count for when we see a forward edge
	for(GRAPH* node = graph; node; node = node->next) {

		if( !get_forward_edge(node) ) i++;
		node->data_len = UINT_MAX;
	}

	// make data_len = 0 again
	for(GRAPH* node = graph; node; node = node->next) node->data_len = 0;

	return i-2; // ignore final 2 nodes (don't represent any bits)
}

// the nodes we passed througth must be marked with an data_len != 0
GRAPH* get_backedge(GRAPH* node) {

    if(!node) return NULL;

	for(CONNECTION* conn = node->connections; conn; conn = conn->next) {

		if( is_backedge(conn->node) ) return conn->node;
	}
	return NULL;
}

// the nodes we passed througth must be marked with info
GRAPH* get_backedge_with_info(GRAPH* node) {

    if(!node) return NULL;

	for(CONNECTION* conn = node->connections; conn; conn = conn->next) {

		if( conn->node->data_len == sizeof(INFO_NODE) && conn->node->data && graph_get_info(conn->node) ) return conn->node;
	}
	return NULL;
}


GRAPH* get_forward_edge(GRAPH* node) {

	if( !node ) return NULL;

	// if we have only one connection or less, there is no forward edge
	if( !node->connections || !node->connections->next ) {
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

void add_node_to_stacks(STACKS* stacks, GRAPH* node, unsigned long h_idx, unsigned long is_odd) {

	// save size of the stack with different parity
	stacks->history.stack[h_idx] = is_odd ? stacks->even.n : stacks->odd.n;

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

void add_backedge2014(STACKS* stacks, GRAPH* source_node, uint8_t bit, uint8_t is_odd) {

	add_backedge(stacks, source_node, 0, bit, is_odd);
}

void add_backedge(STACKS* stacks, GRAPH* source_node, uint8_t prev_has_backedge_in_this_stack, uint8_t bit, uint8_t is_odd) {

	uint8_t is_dest_odd = bit ? !is_odd : is_odd;

	PSTACK* dest_stack = get_parity_stack(stacks, is_dest_odd);

	if( dest_stack->n && ( dest_stack->n - prev_has_backedge_in_this_stack ) ) {
		unsigned long dest_idx = rand() % (dest_stack->n - prev_has_backedge_in_this_stack);	
		GRAPH* dest_node = dest_stack->stack[ dest_idx ];	
		graph_oriented_connect(source_node, dest_node);
		pop_stacks(stacks, dest_idx, ((UTILS_NODE*)dest_node->data)->h_idx - 1);
	}
}

unsigned long get_trailing_zeroes(uint8_t* data, unsigned long data_len) {

	for(unsigned long i = 0; i < data_len; i++) {
		if(data[i]) {
			for(uint8_t j = 0; j < 8; j++) {
				if(get_bit(data, i*8+j)) {
					return i*8 + j;
				}
			}
		}
	}

	return data_len*8;
}

void* encode_numeric_string(char* string, unsigned long* data_len) {

    if( !string ) return NULL;

    unsigned long size = strlen(string);

    // if any of these isn't a number, return NULL;
    for(unsigned long i = 0; i < size; i++) if( string[i] < '0' || string[i] > '9' ) return NULL;

    unsigned long offset = size % 4;
    *data_len = 3 * (size/4) + ( offset ? offset: 1);
    uint8_t* data = malloc(*data_len);
    memset(data, 0x00, *data_len);

    unsigned long data_idx = offset == 1 ? 2 : 4;

    for(unsigned long i = 0; i < size; i++) {

        if( string[i] ) {
            for(unsigned long j = 2; j < 8; j++) {

                set_bit(data, data_idx++, get_bit((uint8_t*)string, i*8+j));
            }
        }
    }
    return data;
}

char* decode_numeric_string(void* data, unsigned long data_len) {

    unsigned long num_bits = data_len * 8;
    uint8_t offset = get_trailing_zeroes(data, data_len);
    unsigned long str_size = 4 * (data_len/3) + (data_len % 3) + 1;
    char* str = malloc(str_size);
    memset(str, 0x00, str_size);

    unsigned long str_idx = 0;

    for(unsigned long i = offset; i < num_bits; i++) {

        if( !(str_idx % 8) ) str_idx+=2;

        set_bit((uint8_t*)str, str_idx++, get_bit(data, i));
    }
    return str;
}

void add_idx(GRAPH* node, unsigned long h_idx, unsigned long bit_idx, char bit) {

	h_idx++;
	graph_alloc(node, sizeof(UTILS_NODE));
	((UTILS_NODE*)node->data)->h_idx = h_idx;
    ((UTILS_NODE*)node->data)->bit_idx = bit_idx;
	((UTILS_NODE*)node->data)->bit = bit;
}

