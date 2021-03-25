#include "decoder/decoder.h"

typedef struct WM_NODE {

	// both 0 based idx, ready to use
	unsigned long hamiltonian_idx;
	unsigned long stack_idx;
} WM_NODE;

typedef struct DECODER {

	#ifdef DEBUG
		GRAPH* graph;
	#endif

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
	unsigned long stack_idx = is_odd ? decoder->n_odd : decoder->n_even;
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

	#ifdef DEBUG
		decoder->graph = graph;
	#endif

	return decoder;
}

uint8_t is_inner_node(DECODER* decoder, GRAPH* node) {

	WM_NODE* node_info = node->data;
	uint8_t is_odd = !(node_info->hamiltonian_idx & 1);

	GRAPH** stack = is_odd ? decoder->odd : decoder->even;
	unsigned long n = is_odd ? decoder->n_odd : decoder->n_even;

	return !( node_info->stack_idx < n && stack[node_info->stack_idx] == node );
}

void update_stacks_to_new_backedge(DECODER* decoder, WM_NODE* dest) {

	if( !( dest->hamiltonian_idx & 1 ) ) {
		decoder->n_odd = dest->stack_idx + 1;
		decoder->n_even = decoder->history[dest->hamiltonian_idx];
	} else {
		decoder->n_odd = decoder->history[dest->hamiltonian_idx];
		decoder->n_even = dest->stack_idx + 1;
	}
}

#ifdef DEBUG
void print_node(void* data, unsigned int data_len) {

	if( data ) {
		printf("\x1b[33m %lu %s\x1b[0m",
				data_len == sizeof(unsigned long) ? *(unsigned long*)data : ((WM_NODE*)data)->hamiltonian_idx + 1,
				data_len == sizeof(unsigned long) ? "(untouched)" : "(WM_NODE)");
	} else {	
		printf("\x1b[33m null \x1b[0m");
	}
}

void print_stacks(DECODER* decoder) {

	printf("e o h\n");
	for( unsigned long i=0; i < decoder->n_history; i++) {

		unsigned long even = i < decoder->n_even ? ((WM_NODE*)decoder->even[i]->data)->hamiltonian_idx+1 : 0;
		unsigned long odd = i < decoder->n_odd ? ((WM_NODE*)decoder->odd[i]->data)->hamiltonian_idx+1 : 0;
		printf("%lu %lu %lu %s\n", even, odd, decoder->history[i], decoder->n_history & 1 ? "(e)" : "(o)");
	}
}

void print_inner_node_log(DECODER* decoder, WM_NODE* dest, unsigned long i, uint8_t* bit_arr) {

	printf("ERRO (idx %lu): dest node %lu é inner node\n", i+1, dest->hamiltonian_idx+1);
	printf("dest node info:\n");
	printf("\tis_odd: %d\n", !(dest->hamiltonian_idx & 1));
	printf("\tstack_idx (0-based): %lu\n", dest->stack_idx);
	printf("\thamiltonian_idx (1-based): %lu\n", dest->hamiltonian_idx);
	printf("current bit arr: ");
	for( unsigned long j=0; j < i; j++ ) printf("%hhu ", bit_arr[j]);
	printf("\n");
	print_stacks(decoder);
	graph_print(decoder->graph, print_node);
}

void print_bit_arr(uint8_t* bit_arr, unsigned long n) {

	for(unsigned long i = 0; i < n; i++) printf("%hhu ", bit_arr[i]);
	printf("\n");
}
#endif

uint8_t* get_bit_array(DECODER* decoder) {

	uint8_t* bit_arr = malloc( sizeof(uint8_t) * decoder->n_bits );

	// iterate over every node but the last
	for(unsigned long i=0; i < decoder->n_bits; i++ ) {

		// check if there is backedge (must have two connections, the first one is the backedge)
		if( decoder->current_node->connections && decoder->current_node->connections->next ) {

			GRAPH* dest_node = decoder->current_node->connections->node;
			WM_NODE* dest = ((WM_NODE*)dest_node->data);

			// check if it is inner node
			if( is_inner_node(decoder, dest_node) ) {
				#ifdef DEBUG
					print_inner_node_log(decoder, dest, i, bit_arr);
				#endif
				free(bit_arr);
				return NULL;
			}

			update_stacks_to_new_backedge(decoder, dest);
			// 1 if diff is odd, 0 otherwise
			bit_arr[i] = (i - dest->hamiltonian_idx) & 1;

		// check if this is the first node
		} else if( !decoder->current_node->prev ) {

			bit_arr[0]=1;

		// else the node could always have connected to the first node (since it is
		// never an inner vertex). Since it didn't, it means it has the value inverse
		// to the one it would have if it connected to the first node
		} else {
			// if a bit is 1, the backedge should go to a node with different parity
			// if a bit is 0, the backedge should go to a node with the same parity
			// since there is no backedge here, and we can always connect to the first
			// node, we just need to see what is the current node's parity, to know
			// the bit it encodes (it the value which can't be represented by an
			// backedge to the first node)

			// 1 - current_node = i. If i is odd, the bit is 0, if i is even the bit is 1.
			bit_arr[i] = !(i & 1);
		}

		label_new_current_node(decoder);
		decoder->current_node = decoder->current_node->next;
	}

	return bit_arr;
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

uint8_t* get_bit_sequence(uint8_t* bit_arr, unsigned long n_bits) {

	unsigned long num_bytes = n_bits / 8 + !!( n_bits % 8 ); 
	uint8_t* data = malloc( sizeof(uint8_t) * num_bytes );
	memset(data, 0x00, num_bytes);

	// get trailing_zeros in first byte
	uint8_t offset = n_bits % 8 ? 8 - (n_bits % 8) : 0;

	for( unsigned long i = 0; i < n_bits; i++ ) set_bit( data, i+offset, bit_arr[i] );

	return data;
}

void decoder_free(DECODER* decoder) {

	free(decoder->even);
	free(decoder->odd);
	free(decoder->history);
	free(decoder);
} 

// basically do the same as the encoder, but with too differences:
// 1. instead to random backedges, just check the current node's
// backedge and infer bit value from it
// 2. instead of just the hamiltonian path index (which is the same as
// the history idx) and index in its parity's stack (used to check if watermark is valid)
void* watermark_decode(GRAPH* graph, unsigned long* num_bytes) {

	if( !graph ) return NULL;

	// 1. get number of bits
	unsigned long n_bits = get_num_bits(graph);
	*num_bytes = n_bits/8 + !!(n_bits%8);

	// 2. create decoder struct
	DECODER* decoder = decoder_create(graph, n_bits);

	// 3. get bit array (nodes are labeled as they are evaluated)
	uint8_t* bit_arr = get_bit_array(decoder);

	if( !bit_arr ) return NULL;

	// 4. turn bit array into bit sequence
	uint8_t* data = get_bit_sequence( bit_arr, n_bits);

	free(bit_arr);
	decoder_free(decoder);

	return data;
}

void* watermark_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_bytes) {

	if( num_rs_bytes >= *num_bytes ) return NULL;

	uint8_t* data = watermark_decode(graph, num_bytes);

	int result = rs_decode(data, *num_bytes, (uint16_t*)( data + (*num_bytes - num_rs_bytes) ), num_rs_bytes);

	// if there were no errors or they were corrected
	if( result >= 0 ) {

		return data;

	// if there were errors and they could not be corrected
	}
	free(data);
	return NULL;
}
