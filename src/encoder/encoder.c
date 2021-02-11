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

} ENCODER;

void add_to_odd(ENCODER* encoder, GRAPH* node) {
	encoder->odd[encoder->n_odd++] = node;
}

void add_to_even(ENCODER* encoder, GRAPH* node) {
	encoder->even[encoder->n_even++] = node;
}

void connect_to_backedge(ENCODER* encoder, uint8_t connect_to_odd) {

	// 1. get random node to connect to
	if( connect_to_odd ) {

		unsigned long idx = rand() % encoder->n_odd;
		GRAPH* node = encoder->odd[idx];
		graph_oriented_connect(encoder->final_node, node);
		// remove new inner nodes
		encoder->n_odd = idx+1;
	} else {
		unsigned long idx = rand() % encoder->n_even;
		GRAPH* node = encoder->even[idx];
		graph_oriented_connect(encoder->final_node, node);
		// remove new inner nodes
		encoder->n_even = idx+1;
	}
}

void encode_bit(ENCODER* encoder, uint8_t bit) {

	// 1. create node and insert it in the node
	graph_insert(encoder->final_node, graph_create(&bit, sizeof(uint8_t)));

	graph_oriented_connect(encoder->final_node, encoder->final_node->next);
	encoder->final_node = encoder->final_node->next;

	// 2. connect to backedge to odd or even node
	uint8_t is_odd = !( encoder->final_node->prev == encoder->odd[encoder->n_odd-1]);

	uint8_t connect_to_odd = bit ? !is_odd : is_odd ;

	connect_to_backedge(encoder, connect_to_odd);

	// 3. add new node as a new valid node
	if(is_odd) {
		add_to_odd(encoder, encoder->final_node);
	} else {
		add_to_even(encoder, encoder->final_node);
	}
}

uint8_t get_bit(uint8_t byte, uint8_t bit_idx) {

	return byte & ( 0b10000000 >> bit_idx );
}

void encode_byte(ENCODER* encoder, uint8_t byte, uint8_t* bit_1_found) {

	uint8_t one=1;

	// 1. iterate over bits
	for(uint8_t bit_idx=0; bit_idx < 8; bit_idx++) {

		// 2. get current bit
		uint8_t bit = get_bit(byte, bit_idx);

		// 3. if we just found the first positive bit, create first node
		if( !*bit_1_found ) {

			if( *bit_1_found = bit ) {

				encoder->graph = graph_create(&one, sizeof(uint8_t));
				encoder->final_node = encoder->graph;
				add_to_odd(encoder, encoder->graph);
			}
		// 4. if first positive bit was already found, just encode whatever we find normally
		} else {
			encode_bit(encoder, bit);	
		}
	}
}

GRAPH* watermark_encode(void* data, unsigned long data_len) {

	if( !data || !data_len ) return NULL;

	// initialize RNG
	srand(time(0));

	// initialize encoder
	ENCODER* encoder = malloc(sizeof(ENCODER));
	encoder->graph = NULL;
	encoder->final_node = NULL;
	encoder->even = malloc(sizeof(GRAPH*) * data_len * 4);
	encoder->odd = malloc(sizeof(GRAPH*) * data_len * 4);
	encoder->n_even = encoder->n_odd = 0;

	uint8_t bit_1_found = 0;

	uint8_t* bytes = data;

	for(unsigned long i=0; i < data_len; i++) {
		encode_byte(encoder, bytes[i], &bit_1_found);
	}

	// add an empty node after the ones with encoded bits
	uint8_t full = 0xff;
	GRAPH* final_node = graph_create(&full, sizeof(full));
	graph_insert(encoder->final_node, final_node);
	graph_oriented_connect(encoder->final_node, final_node);

	GRAPH* final_graph = encoder->graph;

	free(encoder->even);
	free(encoder->odd);
	free(encoder);

	return final_graph;
}
