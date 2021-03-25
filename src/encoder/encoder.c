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

#ifdef DEBUG 
void print_stacks_encoder(ENCODER* encoder) {

	printf("e o h\n");
	for( unsigned long i=0; i < encoder->n_history; i++) {

		unsigned long even = i < encoder->n_even ? (*(unsigned long*)encoder->even[i]->data) : 0;
		unsigned long odd = i < encoder->n_odd ? (*(unsigned long*)encoder->odd[i]->data) : 0;
		printf("%lu %lu %lu %s\n", even, odd, encoder->history[i], encoder->n_history & 1 ? "(e)" : "(o)");
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

void add_node_to_stacks(ENCODER* encoder, unsigned long is_odd) {

	// save size of the stack with different parity
	encoder->history[encoder->n_history++] = is_odd ? encoder->n_even : encoder->n_odd;

	// save to stack with the same parity
	GRAPH** stack = is_odd ? encoder->odd : encoder->even;
	unsigned long* n = is_odd ? &encoder->n_odd : &encoder->n_even;
	stack[(*n)++] = encoder->final_node;
}

ENCODER* encoder_create(unsigned long n_bits) {

	ENCODER* encoder = malloc( sizeof(ENCODER) );
	encoder->odd = malloc(sizeof(GRAPH*) * (n_bits/2+1));
	encoder->even = malloc(sizeof(GRAPH*) * (n_bits/2));
	encoder->history = malloc(sizeof(unsigned long) * n_bits);
	encoder->n_history = 0;
	encoder->n_odd = 0;
	encoder->n_even = 0;

	// create graph
	unsigned long one = 1;
	encoder->final_node = encoder->graph = graph_create(&one, sizeof(unsigned long));
	add_node_to_stacks(encoder, 1);

	return encoder;
}

void encoder_free(ENCODER* encoder) {

	free(encoder->odd);
	free(encoder->even);
	free(encoder->history);
	free(encoder);
}

void add_node_to_graph(ENCODER* encoder, unsigned long idx) {

	idx++;

	GRAPH* new_node = graph_create(&idx, sizeof(idx));

	// save index of the node in the hamiltonian path
	graph_insert(encoder->final_node, new_node);
	graph_oriented_connect(encoder->final_node, new_node);
	encoder->final_node = new_node;
}

void pop_all(unsigned long* n, unsigned long idx) {

	*n = idx+1;
}

void pop_all_history(unsigned long* n, unsigned long size) {

	*n = size;
}

void add_backedge(ENCODER* encoder, uint8_t bit, uint8_t is_odd) {

	uint8_t is_dest_odd = bit ? !is_odd : is_odd;

	GRAPH** dest_stack = is_dest_odd ? encoder->odd : encoder->even;
	unsigned long* n_dest = is_dest_odd ? &encoder->n_odd : &encoder->n_even;

	unsigned long* not_n_dest = is_dest_odd ? &encoder->n_even : &encoder->n_odd;

	if( *n_dest ) {
		unsigned long dest_idx = rand() % *n_dest;
		GRAPH* dest = dest_stack[ dest_idx ];
		graph_oriented_connect(encoder->final_node, dest);
		pop_all(n_dest, dest_idx);
		pop_all_history(not_n_dest, encoder->history[ (*(unsigned long*)dest->data) - 1 ]);
	}
}

void encode_bit(ENCODER* encoder, uint8_t bit, uint8_t is_odd, unsigned long idx) {

	// 1. add node to graph
	add_node_to_graph(
			encoder,
			idx
		);

	// 2. if bit is 1, connect to a different parity bit, otherwise connect to same parity
	add_backedge(
			encoder,
			bit,
			is_odd
		);

	// 3. add node to proper parity stack, and to the history stack
	add_node_to_stacks(
			encoder,
			is_odd
		);
}

void encode(ENCODER* encoder, void* data, unsigned long total_bits, unsigned long trailing_zeroes) {

	for(unsigned long i = trailing_zeroes+1; i < total_bits; i++) {

		// 0-based index of the node's position in the hamiltonian path
		unsigned long idx = i-trailing_zeroes;

		encode_bit(
				encoder,
				get_bit(data, i),
				(i-trailing_zeroes-1) & 1,
				idx
			);
	}
}

GRAPH* watermark_encode(void* data, unsigned long data_len) {

	if( !data || !data_len ) return NULL;

	srand(time(0));

	unsigned long trailing_zeroes = get_trailing_zeroes(data, data_len);

	// only zeroes
	if( trailing_zeroes == data_len * 8 ) return NULL;

	unsigned long n_bits = data_len * 8 - trailing_zeroes;

	ENCODER* encoder = encoder_create(n_bits);

	encode(encoder, data, n_bits + trailing_zeroes, trailing_zeroes);

	// add final node
	graph_insert(encoder->final_node, graph_empty());
	graph_oriented_connect(encoder->final_node, encoder->final_node->next);

	GRAPH* graph = encoder->graph;

	encoder_free(encoder);
	
	return graph;
}

GRAPH* watermark_encode_with_rs(void* data, unsigned long data_len, unsigned long num_rs_bytes) {

	uint16_t par[num_rs_bytes];
	memset(par, 0x00, num_rs_bytes);

	// get parity data
	rs_encode(data, data_len, par, num_rs_bytes);

	// copy data + parity data
	uint8_t final_data[data_len + num_rs_bytes];
	memcpy(final_data, data, data_len);
	memcpy(final_data + data_len, par, num_rs_bytes * 2);

	return watermark_encode(final_data, data_len);
}
