#include "encoder/encoder.h"

typedef struct ENCODER {

	// nodes have pointer as data, which point to a list
	GRAPH* graph;
	GRAPH* final_node;

	// odd labeled stack
	GRAPH** odd;
	unsigned long n_odd;

	// even labeled stack
	GRAPH** even;
	unsigned long n_even;

	// stack size history
	unsigned long* history;
	unsigned long n_history;

} ENCODER;

typedef enum {

	HAMILTONIAN,
	BACK_EDGE,
	FORWARD_EDGE
} CONN_TYPE;

#ifdef DEBUG 
void _print_stacks_encoder(ENCODER* encoder) {

	printf("e o h\n");
	for( unsigned long i=0; i < encoder->n_history; i++) {

		unsigned long even = i < encoder->n_even ? (*(unsigned long*)encoder->even[i]->data) : 0;
		unsigned long odd = i < encoder->n_odd ? (*(unsigned long*)encoder->odd[i]->data) : 0;
		printf("%lu %lu %lu %s\n", even, odd, encoder->history[i], encoder->n_history & 1 ? "(e)" : "(o)");
	}
}
#endif

uint8_t _get_bit(uint8_t* data, unsigned long bit_idx) {

	// 0x80 = 0b1000_0000
	uint8_t byte = data[bit_idx/8];
	return byte & ( 0x80 >> (bit_idx % 8));
}

uint8_t _get_trailing_zeroes(uint8_t* data, unsigned long data_len) {

	// praticamente python
	for(unsigned long i = 0; i < data_len; i++) {
		if(data[i]) {
			for(uint8_t j = 0; j < 8; j++) {
				if(_get_bit(data, i*8+j)) {
					return i*8 + j;
				}
			}
		}
	}

	return data_len*8;
}

void connect_hamiltonian_edge(GRAPH* node1, GRAPH* node2) {

	graph_oriented_connect(node1, node2);
	node1->connections->weight = HAMILTONIAN;
}

void connect_forward_edge(GRAPH* node1, GRAPH* node2) {

	graph_oriented_connect(node1, node2);
	node1->connections->weight = FORWARD_EDGE;
}

void connect_back_edge(GRAPH* node1, GRAPH* node2) {

	graph_oriented_connect(node1, node2);
	node1->connections->weight = BACK_EDGE;
}

CONNECTION* search_connection_type(CONNECTION* conn, CONN_TYPE type) {

	while(conn) {

		if( conn->weight == type ) return conn;
	}
	return NULL;
}

void add_new_node_to_hamiltonian_path(ENCODER* encoder) {

	unsigned long idx = *((unsigned long*)encoder->final_node->data)+1;

	graph_insert(encoder->final_node, graph_create(&idx, sizeof(unsigned long)));
	graph_oriented_connect(encoder->final_node, encoder->final_node->next);
	encoder->final_node = encoder->final_node->next;
}

void _add_node_to_stacks(ENCODER* encoder, unsigned long is_odd) {

	// save size of the stack with different parity
	encoder->history[encoder->n_history++] = is_odd ? encoder->n_even : encoder->n_odd;

	// save to stack with the same parity
	GRAPH** stack = is_odd ? encoder->odd : encoder->even;
	unsigned long* n = is_odd ? &encoder->n_odd : &encoder->n_even;
	stack[(*n)++] = encoder->final_node;
}

ENCODER* _encoder_create(unsigned long n_bits) {

	ENCODER* encoder = malloc( sizeof(ENCODER) );
	// nodes are determined to be odd or even based on their 1-based index
	encoder->odd = malloc(sizeof(GRAPH*) * (n_bits/2+1));
	encoder->even = malloc(sizeof(GRAPH*) * (n_bits/2));
	encoder->history = malloc(sizeof(unsigned long) * n_bits);
	encoder->n_history = 0;
	encoder->n_odd = 0;
	encoder->n_even = 0;

	// create graph
	unsigned long one = 1;
	encoder->final_node = encoder->graph = graph_create(&one, sizeof(unsigned long));
	_add_node_to_stacks(encoder, 1);

	// create graph with empty nodes (with n+1 nodes) and 1-based indices
	for(unsigned long i = 2; i < n_bits+1; i++) {

		graph_insert(encoder->final_node, graph_create(&i, sizeof(i)));
		connect_hamiltonian_edge(encoder->final_node, encoder->final_node->next);
		encoder->final_node = encoder->final_node->next;
	}

	return encoder;
}

void _encoder_free(ENCODER* encoder) {

	free(encoder->odd);
	free(encoder->even);
	free(encoder->history);
	free(encoder);
}

void _pop_all(unsigned long* n, unsigned long idx) {

	*n = idx+1;
}

void _pop_all_history(unsigned long* n, unsigned long size) {

	*n = size;
}

void add_random_backedge(ENCODER* encoder, GRAPH* node, uint8_t bit, uint8_t is_odd) {

	// ii) remove w = v - 1 from selection set
	CONNECTION* v_1_backedge = search_connection_type(node->prev->connections, BACK_EDGE);
	GRAPH* v_1_backedge_destination = v_1_backedge ? v_1_backedge->node : NULL;

	uint8_t is_dest_odd = bit ? !is_odd : is_odd;

	GRAPH** dest_stack = is_dest_odd ? encoder->odd : encoder->even;
	unsigned long* n_dest = is_dest_odd ? &encoder->n_odd : &encoder->n_even;

	unsigned long* not_n_dest = is_dest_odd ? &encoder->n_even : &encoder->n_odd;

	if( *n_dest ) {
		unsigned long dest_idx = rand() % (*n_dest - (dest_stack[ (*n_dest) -1 ] == v_1_backedge_destination));
		GRAPH* dest = dest_stack[ dest_idx ];
		graph_oriented_connect(node, dest);
		_pop_all(n_dest, dest_idx);
		_pop_all_history(not_n_dest, encoder->history[ (*(unsigned long*)dest->data) - 1 ]);
	}
}

uint8_t is_forward_edge_destination(GRAPH* node) {

	// v-2 -> v
	// weight = 1
	// don't need to check if this is the exact destination node, if node->prev->prev has a forward edge, it must connect to node
	return node->prev && node->prev->prev && search_connection_type(node->prev->prev->connections, FORWARD_EDGE);
}

uint8_t is_inner_to_forward_edge(GRAPH* node) {

	return node->next && is_forward_edge_destination(node->next);
}

uint8_t exists_possible_backedge_from(ENCODER* encoder, GRAPH* node, uint8_t bit) {

	uint8_t is_odd = *((unsigned long*)node->data) & 1;

	// i) if v - 1 -> v + 1 exists, set w = v - 1
	if( is_inner_to_forward_edge(node) ) {
		// is w = v - 1 valid?
		return !search_connection_type(node->prev->connections, BACK_EDGE);
	}

	// ii) remove w <- v - 1 from selection set
	CONNECTION* v_1_backedge = search_connection_type(node->prev->connections, BACK_EDGE);
	GRAPH* v_1_backedge_destination = v_1_backedge ? v_1_backedge->node : NULL;

	// add backedge, remove every node above w from stack

	// iii) stacks will make sure there is no w as source of a backege

	// iv) stacks will make sure no w is inner node to a forward edge

	// v) stacks will make sure no w is inner node to a backedge

	if( bit ^ is_odd ) {
		// odd
		// ii)
		return encoder->n_odd && ( encoder->odd[encoder->n_odd-1] != v_1_backedge_destination || encoder->n_odd > 1);
	} else {
		// even
		return encoder->n_even && ( encoder->even[encoder->n_even-1] != v_1_backedge_destination || encoder->n_even > 1);
	}
}

void encode2017(ENCODER* encoder, void* data, unsigned long total_bits, unsigned long trailing_zeroes) {

	GRAPH* node = encoder->graph;

	for(unsigned long i = trailing_zeroes+1; i < total_bits;) {

		// 0-based index of the node's position in the hamiltonian path
		unsigned long idx = i-trailing_zeroes;

		// if this node is the destination of a forward edge
		if( is_forward_edge_destination(node) ) {
			i--;
		} else if( exists_possible_backedge_from(encoder, node, _get_bit(data, i)) ) {

			if( is_inner_to_forward_edge(node) ) {

				if( _get_bit(data, i) ) {

					// add back edge v - 1 <- v
					// remove path edge v -> v + 1
					// while block
					connect_back_edge(node, node->prev);
					graph_oriented_disconnect(node, node->next);

				}

				// don't add back or forward edge with origin in v
				// v - 1, v, and v + 1 = if then

			} else {

				// add backedge v - k <- v randomly
				add_random_backedge(encoder, node, _get_bit(data, i), idx & 1);
			}

		} else if( _get_bit(data, i) ) {

				// add node to graph ( V(G) U {t+1} ) to the hamiltonian path
				// add forward edge v -> v + 2
				add_new_node_to_hamiltonian_path(encoder);
				connect_forward_edge(node, node->next->next);
		}
		node = node->next;
	}
}

GRAPH* watermark2017_encode(void* data, unsigned long data_len) {

	if( !data || !data_len ) return NULL;

	srand(time(0));

	unsigned long trailing_zeroes = _get_trailing_zeroes(data, data_len);

	// only zeroes
	if( trailing_zeroes == data_len * 8 ) return NULL;

	unsigned long n_bits = data_len * 8 - trailing_zeroes;

	ENCODER* encoder = _encoder_create(n_bits);

	encode2017(encoder, data, n_bits + trailing_zeroes, trailing_zeroes);

	// add final node
	graph_insert(encoder->final_node, graph_empty());
	graph_oriented_connect(encoder->final_node, encoder->final_node->next);

	GRAPH* graph = encoder->graph;

	_encoder_free(encoder);
	
	return graph;
}
