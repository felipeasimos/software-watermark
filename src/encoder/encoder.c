#include "encoder/encoder.h"

typedef struct ENCODER {

	// nodes have pointer as data, which point to a list
	GRAPH* graph;
	GRAPH* final_node;

	STACKS stacks;
} ENCODER;

#ifdef DEBUG 
void print_stacks_encoder(ENCODER* encoder) {

	printf("e o h\n");
	for( unsigned long i=0; i < encoder->stacks.history.n; i++) {

		unsigned long even = i < encoder->stacks.even.n ? (*(unsigned long*)encoder->stacks.even.stack[i]->data) : 0;
		unsigned long odd = i < encoder->stacks.odd.n ? (*(unsigned long*)encoder->stacks.odd.stack[i]->data) : 0;
		printf("%lu %lu %lu %s\n", even, odd, encoder->stacks.history.stack[i], encoder->stacks.history.n & 1 ? "(e)" : "(o)");
	}
}
#endif

uint8_t get_bit(uint8_t* data, unsigned long bit_idx) {

	// 0x80 = 0b1000_0000
	uint8_t byte = data[bit_idx/8];
	return byte & ( 0x80 >> (bit_idx % 8));
}

uint8_t get_trailing_zeroes(uint8_t* data, unsigned long data_len) {

	// praticamente python
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

void add_node_to_graph(ENCODER* encoder) {

	GRAPH* new_node = graph_empty();

	// save index of the node in the hamiltonian path
	graph_insert(encoder->final_node, new_node);
	graph_oriented_connect(encoder->final_node, new_node);
	encoder->final_node = new_node;
}

void add_idx(GRAPH* node, unsigned long idx) {

	idx++;
	graph_alloc(node, sizeof(unsigned long));
	*((unsigned long*)node->data) = idx;
}

ENCODER* encoder_create(unsigned long n_bits) {

	ENCODER* encoder = malloc( sizeof(ENCODER) );
	create_stacks(&encoder->stacks, n_bits);
	// create graph (all nodes are created without an index, except for the first one)
	// indices are added as we go through the graph
	unsigned long one = 1;
	encoder->final_node = encoder->graph = graph_create(&one, sizeof(unsigned long));
	add_node_to_stacks(&encoder->stacks, encoder->final_node, 1);

	for(unsigned long i = 1; i < n_bits+1; i++) {
		add_node_to_graph(encoder);
	}

	return encoder;
}

void encoder_free(ENCODER* encoder) {

	free_stacks(&encoder->stacks);
	free(encoder);
}

void encode(ENCODER* encoder, void* data, unsigned long total_bits, unsigned long trailing_zeroes) {

	//start from second node
	GRAPH* node = get_next_hamiltonian_node(encoder->graph);
	for(unsigned long i = trailing_zeroes+1; i < total_bits; i++) {

		// 0-based index of the node's position in the hamiltonian path
		unsigned long idx = i-trailing_zeroes;
		uint8_t is_odd = !((i-trailing_zeroes) & 1);
		uint8_t bit = get_bit(data, i);

		// 1. add index to node
		add_idx(node, idx);

		// 2. if bit is 1, connect to a different parity bit, otherwise connect to same parity
		add_backedge( &encoder->stacks, node, bit, is_odd );

		// 3. add node to proper parity stack, and to the history stack
		add_node_to_stacks( &encoder->stacks, node, is_odd );
		node = get_next_hamiltonian_node(node);
	}
}

GRAPH* watermark2014_encode(void* data, unsigned long data_len) {

	if( !data || !data_len ) return NULL;

	srand(time(0));

	unsigned long trailing_zeroes = get_trailing_zeroes(data, data_len);

	// only zeroes
	if( trailing_zeroes == data_len * 8 ) return NULL;

	unsigned long n_bits = data_len * 8 - trailing_zeroes;

	ENCODER* encoder = encoder_create(n_bits);

	encode(encoder, data, n_bits + trailing_zeroes, trailing_zeroes);

	GRAPH* graph = encoder->graph;

	encoder_free(encoder);
	
	return graph;
}

GRAPH* watermark2014_encode_with_rs(void* data, unsigned long data_len, unsigned long num_rs_bytes) {

	uint16_t par[num_rs_bytes];
	memset(par, 0x00, num_rs_bytes);

	// get parity data
	rs_encode(data, data_len, par, num_rs_bytes);

	// copy data + parity data
	uint8_t final_data[data_len + num_rs_bytes];
	memcpy(final_data, data, data_len);
	memcpy(final_data + data_len, par, num_rs_bytes * 2);

	return watermark2014_encode(final_data, data_len);
}
