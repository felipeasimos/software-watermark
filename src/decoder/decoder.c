#include "decoder/decoder.h"

typedef struct WM_NODE {

	uint8_t is_odd;
	unsigned long idx;
} WM_NODE;

typedef struct DECODER {

	// odd labeled stack
	GRAPH** odd;
	unsigned long n_odd;

	// even labeled stack
	GRAPH** even;
	unsigned long n_even;

} DECODER;

void add_WM_NODE(GRAPH* node, uint8_t is_odd, unsigned long idx) {

	if( node->data ) free(node->data);

	node->data = malloc(sizeof(WM_NODE));
	((WM_NODE*)node->data)->is_odd = is_odd;
	((WM_NODE*)node->data)->idx = idx;
}

uint8_t* get_bit_arr(GRAPH* graph, unsigned long n_bits) {

	uint8_t* bits = malloc(sizeof(uint8_t) * n_bits);
	bits[0] = 1;

	// init decoder
	DECODER decoder;
	decoder.n_odd=1;
	decoder.n_even=0;
	decoder.odd = malloc(sizeof(GRAPH*) * ( n_bits/2 + 1) );
	decoder.even = malloc(sizeof(GRAPH*) * (n_bits/2));

	// add WM_NODE to first node
	decoder.odd[0] = graph;
	add_WM_NODE(graph, 1,0);
	graph = graph->next;

	// 'graph' will never be the first of the last node here
	for(unsigned long i=1; i < n_bits; i++) {

		// i is +1 the node's index (which is 1-based)
		uint8_t is_odd = !(i & 1);

		// 1. check for backedge (backedge exists if first connection is not the next node)
		if( graph->connections[0].node != graph->next ){

			WM_NODE* dest = graph->connections[0].node->data;

			// check if destination is in fact valid (not an inner node)
			if( ( dest->is_odd && dest->idx >= decoder.n_odd ) || ( !dest->is_odd && dest->idx >= decoder.n_even ) ) {

				free(decoder.even);
				free(decoder.odd);
				free(bits);
				return NULL;
			}

			// 2. if destination has same parity, the bit is 0, it is 1 otherwise
			bits[i] = ( dest->is_odd != is_odd );

			// 2.5 remove nodes from stack that are on top of the destination
			if( dest->is_odd ) {
				decoder.n_odd = dest->idx+1;
				decoder.n_even = dest->idx;
			} else {
				decoder.n_even = dest->idx+1;
				decoder.n_odd = dest->idx+1;
			}

		} else {
			// 3. if there is no backedge, check what stack is empty
			bits[i] = ( (!!decoder.n_odd) != (!!is_odd) );
		}

		// add WM_NODE data (useful when future nodes need to inspect this one)
		add_WM_NODE(graph, is_odd, is_odd ? decoder.n_odd : decoder.n_even);

		// add as valid node
		if(is_odd) {
			decoder.odd[decoder.n_odd++] = graph;
		} else {
			decoder.even[decoder.n_even++] = graph;
		}

		graph = graph->next;
	}

	free(decoder.even);
	free(decoder.odd);
	free(bits);

	return bits;
}

unsigned long num_nodes(GRAPH* graph) {

	unsigned long n = 0;
	for(; graph; graph = graph->next ) n++;
	return n;
}

void* get_bit_sequence(uint8_t* bits, unsigned long n_bits, unsigned long num_bytes) {

	uint8_t* data = malloc( sizeof(uint8_t) * num_bytes);
	memset(data, 0x00, num_bytes);

	// zeroes before first one
	int8_t offset = (8 - n_bits % 8);

	for(unsigned long i=0; i < num_bytes; i++) {

		for(uint8_t j=0; j < 8; j++){

			if(offset--) continue;
			data[i] |= (!!bits[i*8+j]) >> j;
		}
	}

	return data;
}

void* watermark_decode(GRAPH* graph, unsigned long* num_bytes) {

	if( !graph ) return NULL;

	// 0. get number of bits
	unsigned long n_bits = num_nodes(graph)-1;
	*num_bytes = (n_bits/8) + !!(n_bits%8);

	// 1. get bit array
	uint8_t* bit_arr = get_bit_arr(graph, n_bits);

	// get_bit_arr returns NULL if the watermark is wrong (backedge to inner node)
	if(!bit_arr) return NULL;

	for(unsigned int i=0; i < n_bits; i++) {
		printf("%hhu", bit_arr[i]);
	}

	// 2. turn bit_array into sequence of bits
	void* data = get_bit_sequence(bit_arr, n_bits, *num_bytes);

	return data;
}
