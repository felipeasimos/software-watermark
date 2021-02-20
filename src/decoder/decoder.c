#include "decoder/decoder.h"

typedef struct WM_NODE {

	// both 0 based idx, ready to use
	unsigned long hamiltonian_idx;
	unsigned long stack_idx;
} WM_NODE;

typedef struct DECODER {

	GRAPH* current_node;

	// odd labeled stack
	GRAPH** odd;
	unsigned long n_odd;

	// even labeled stack
	GRAPH** even;
	unsigned long n_even;

	// stack size history
	unsigned long* history;
	unsigned long n_history;

	unsigned long n_bits;
} DECODER;

unsigned long get_num_bits(GRAPH* graph) {

	unsigned long i=0;
	while( graph ) {
		i++;
		graph = graph->next;
	}
	return i-1; // ignore final node (don't represent any bits)
}

void label_new_current_node(DECODER* decoder) {

	// get parity	
	uint8_t is_odd = !(decoder->n_history & 1);

	// get indexes
	unsigned long stack_idx = is_odd ? decoder->n_even : decoder->n_odd;
	WM_NODE wm_node_info = { decoder->n_history, stack_idx };

	// add node to stacks
	decoder->history[decoder->n_history++] = is_odd ? decoder->n_even : decoder->n_odd;
	if( is_odd ) {
		decoder->odd[decoder->n_odd++] = decoder->current_node;
	} else {
		decoder->even[decoder->n_even++] = decoder->current_node;
	}

	// get WM_NODE on the node
	graph_alloc(decoder->current_node, sizeof(WM_NODE));
	memcpy(decoder->current_node->data, &wm_node_info, decoder->current_node->data_len);
}

DECODER* decoder_create(GRAPH* graph, unsigned long n_bits) {

	DECODER* decoder = malloc( sizeof(DECODER) );
	decoder->current_node = graph;

	decoder->odd = malloc(sizeof(GRAPH*) * (n_bits/2+1));
	decoder->even = malloc(sizeof(GRAPH*) * (n_bits/2));
	decoder->history = malloc(sizeof(unsigned long) * n_bits);
	decoder->n_history = 0;
	decoder->n_odd = 0;
	decoder->n_even = 0;
	decoder->n_bits = n_bits;

	return decoder;
}

uint8_t* get_bit_array(DECODER* decoder) {

	uint8_t* bit_arr = malloc( sizeof(uint8_t) * decoder->n_bits );

	while( decoder->current_node->next ) {

		// check if there is backedge
		if( decoder->current_node->connections->node ) {

		} else {

			// check if this is the first node
			if( !decoder->current_node->prev ) {

				bit_arr[0]=1;
			// check if this is the final node
			} else if ( !decoder->current_node->next ) {

				return bit_arr;
			// else the node could always have connected to the first node (since it is
			// never an inner vertex). Since it didn't, it means it has the value inverse
			// to the one it would have if it connected to the first node
			} else {

				// get index and subtract by first node's idx (1)
				// + 1 - 1 + hamiltonian_idx, since the hamiltonian_idx is saved as a zero based idx
				unsigned long diff = ((WM_NODE*)decoder->current_node->data)->hamiltonian_idx;
				
				// diff is the hamiltonian_idx!
				bit_arr[diff] = !!(diff & 1);
			}
		}

		label_new_current_node(decoder);
		decoder->current_node = decoder->current_node->next;
	}
}

// basically do the same as the encoder, but with too differences:
// 1. instead to random backedges, just check the current node's
// backedge and infer bit value from it
// 2. instead of just the hamiltonian path index (which is the same as
// the history idx) and index in its parity's stack (used to check if watermark is valid)
void* watermark_decode(GRAPH* graph, unsigned long* num_bytes) {

	if( !graph ) return NULL;

	// 1. get number of bits (and label nodes)
	unsigned long n_bits = get_num_bits(graph);
	*num_bytes = n_bits/8 + !!(n_bits%8);

	// 2. create decoder struct
	DECODER* decoder = decoder_create(graph, n_bits);

	// 3. get bit array (nodes are labeled as they are evaluated)
	uint8_t* bit_arr = get_bit_array(decoder);

	// 4. turn bit array into bit sequence
	// uint8_t* data = get_bit_sequence(  );

	free(bit_arr);
	free(decoder);

	// return data;
}
