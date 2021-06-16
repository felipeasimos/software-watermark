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

	STACKS stacks;

	unsigned long n_bits;
} DECODER;

void label_new_current_node(DECODER* decoder, GRAPH* node, unsigned long h_idx) {

	// get parity	
	uint8_t is_odd = !(h_idx & 1);

	// get indexes
	unsigned long stack_idx = get_parity_stack(&decoder->stacks, is_odd)->n;
	WM_NODE wm_node_info = { h_idx, stack_idx };

	add_node_to_stacks(&decoder->stacks, node, h_idx, is_odd);

	// get WM_NODE on the node
	graph_alloc(node, sizeof(WM_NODE));
	memcpy(node->data, &wm_node_info, node->data_len);
}

DECODER* decoder_create(GRAPH* graph, unsigned long n_bits) {

	DECODER* decoder = malloc( sizeof(DECODER) );
	decoder->current_node = graph;

	create_stacks(&decoder->stacks, n_bits);

	decoder->n_bits = n_bits;

	#ifdef DEBUG
		decoder->graph = graph;
	#endif

	return decoder;
}

uint8_t is_inner_node(DECODER* decoder, GRAPH* node) {

	WM_NODE* node_info = node->data;
	uint8_t is_odd = !(node_info->hamiltonian_idx & 1);

	PSTACK* stack = get_parity_stack(&decoder->stacks, is_odd);

	return !( node_info->stack_idx < stack->n && stack->stack[node_info->stack_idx] == node );
}

uint8_t* get_bit_array2014(DECODER* decoder) {

	uint8_t* bit_arr = malloc( sizeof(uint8_t) * decoder->n_bits );

	// iterate over every node but the last
	for(unsigned long i=0; i < decoder->n_bits; i++ ) {

		// check if there is a backedge ( connection with non-zero data_len is a valid backedge)
		GRAPH* dest_node = get_backedge(decoder->current_node);
		if( dest_node ) {

			WM_NODE* dest = ((WM_NODE*)dest_node->data);

			// check if it is inner node
			if( is_inner_node(decoder, dest_node) ) {
				#ifdef DEBUG
					fprintf(stderr, "Invalid backedge detected!\n");
				#endif
				free(bit_arr);
				return NULL;
			}

			pop_stacks(&decoder->stacks, dest->stack_idx, dest->hamiltonian_idx);
			// 1 if diff is odd, 0 otherwise
			bit_arr[i] = (i - dest->hamiltonian_idx) & 1;

		// check if this is the first node (the input graph must have the first
		// hamiltonian node as the first node in the list)
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
			// the bit it encodes (it is the value which can't be represented by an
			// backedge to the first node)

			// 1 - current_node = i. If i is odd, the bit is 0, if i is even the bit is 1.
			bit_arr[i] = !(i & 1);

			// however, if there was a possible backedge for the even stack, this only means
			// that the graph is invalid
			if( get_parity_stack(&decoder->stacks, 0)->n ) {
				free(bit_arr);
				return NULL;
			}
		}

		label_new_current_node(decoder, decoder->current_node, i);
		decoder->current_node = get_next_hamiltonian_node2014(decoder->current_node);
	}

	return bit_arr;
}

unsigned long get_node_idx(GRAPH* node) {

	return ((WM_NODE*)node->data)->hamiltonian_idx;
}

uint8_t* get_bit_array2017(DECODER* decoder) {

	uint8_t* bit_arr = malloc( sizeof(uint8_t) * decoder->n_bits );
	memset(bit_arr, 0x00, sizeof(uint8_t) * decoder->n_bits);

	// if positive, decrement it, if it is zero the current node is a forward destination
	int forward_flag = -1;

	bit_arr[0]=1;

	unsigned long h_idx=1;

	GRAPH* next = decoder->current_node->next;
	label_new_current_node(decoder, decoder->current_node, 0);
	decoder->current_node = next;

	// iterate over every node but the last
	for(unsigned long i=1; i < decoder->n_bits; i++ ) {

        if(!decoder->current_node) {
            free(bit_arr);
            return NULL;
        }

		if( forward_flag != 0 ) {

			if( get_forward_edge(decoder->current_node) ) {
				bit_arr[i] = 1;
				forward_flag = 2;
			} else if( get_backedge(decoder->current_node) && !( (h_idx - get_node_idx(get_backedge(decoder->current_node))-1) & 1 ) ) {
				bit_arr[i] = 1;
				GRAPH* dest_node = get_backedge(decoder->current_node);
				WM_NODE* dest = dest_node->data;

				// check if it is inner node
				if( is_inner_node(decoder, dest_node) ) {
					#ifdef DEBUG
						fprintf(stderr, "Invalid backedge detected!\n");
					#endif
					free(bit_arr);
					return NULL;
				}

				pop_stacks(&decoder->stacks, dest->stack_idx, dest->hamiltonian_idx);
			} else {
				bit_arr[i] = 0;
				GRAPH* dest_node = get_backedge(decoder->current_node);
				if(dest_node) {

					// check if it is inner node
					if( is_inner_node(decoder, dest_node) ) {
						#ifdef DEBUG
							fprintf(stderr, "Invalid backedge detected!\n");
						#endif
						free(bit_arr);
						return NULL;
					}
					WM_NODE* dest = dest_node->data;
					pop_stacks(&decoder->stacks, dest->stack_idx, dest->hamiltonian_idx);
				}
			}
		} else {
			i--;
		}

		if( forward_flag >= 0 ) forward_flag--;

		GRAPH* next = decoder->current_node->next;	
		label_new_current_node(decoder, decoder->current_node, h_idx);
		decoder->current_node = next;
		h_idx++;
	}

	return bit_arr;

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

	free_stacks(&decoder->stacks);
	free(decoder);
} 

void* _watermark_decode(GRAPH* graph, unsigned long* num_bytes, unsigned long n_bits, uint8_t* (*bit_array_func)(DECODER* decoder)) {

	if( !is_graph_structure_valid(graph) ) return NULL;

	// 1. get number of bits and set nodes to null
	*num_bytes = n_bits/8 + !!(n_bits%8);

	// 2. create decoder struct
	DECODER* decoder = decoder_create(graph, n_bits);

	// 3. get bit array (nodes are labeled as they are evaluated)
	uint8_t* bit_arr = bit_array_func(decoder);

	if( !bit_arr ) return NULL;

	// 4. turn bit array into bit sequence
	uint8_t* data = get_bit_sequence( bit_arr, n_bits);

	free(bit_arr);
	decoder_free(decoder);

	return data;
}

void* _watermark_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_bytes, void* (*decode_func)(GRAPH*, unsigned long*)) {

	uint8_t* data = decode_func(graph, num_bytes);

	// make num_bytes equal to the actual data payload size
	*num_bytes -= num_rs_bytes * sizeof(uint16_t);
	
	int result = rs_decode(data, *num_bytes, (uint16_t*)( data + *num_bytes ), num_rs_bytes);

	// if there were no errors or they were corrected
	if( result >= 0 ) return data;

	// if there were errors and they could not be corrected
	free(data);
	return NULL;
}

void* watermark2014_decode(GRAPH* graph, unsigned long* num_bytes) {

	return _watermark_decode(graph, num_bytes, get_num_bits2014(graph), get_bit_array2014);
}

void* watermark2014_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_bytes) {

	return _watermark_decode_with_rs(graph, num_bytes, num_rs_bytes, watermark2014_decode);
}

void* watermark2017_decode(GRAPH* graph, unsigned long* num_bytes) {

	return _watermark_decode(graph, num_bytes, get_num_bits(graph), get_bit_array2017);
}

void* watermark2017_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_bytes) {

	return _watermark_decode_with_rs(graph, num_bytes, num_rs_bytes, watermark2017_decode);
}

void* watermark2017_decode_analysis(GRAPH* graph, unsigned long* num_bytes) {

	if( !is_graph_structure_valid(graph) || !num_bytes ) return NULL;

    unsigned long n_bits = get_num_bits(graph);

	// 1. get size of bit_arr array and set nodes to null
	*num_bytes = n_bits;

	// 2. create decoder struct
	DECODER* decoder = decoder_create(graph, n_bits);

	uint8_t* bit_arr = malloc( sizeof(char) * decoder->n_bits );
	memset(bit_arr, 0x00, sizeof(char) * decoder->n_bits);

	// if positive, decrement it, if it is zero the current node is a forward destination
	int forward_flag = -1;

	bit_arr[0]='1';

	unsigned long h_idx=1;

	GRAPH* next = decoder->current_node->next;
	label_new_current_node(decoder, decoder->current_node, 0);
	decoder->current_node = next;

	// iterate over every node but the last
	for(unsigned long i=1; i < decoder->n_bits; i++ ) {

        if(!decoder->current_node) {
            free(bit_arr);
            return NULL;
        }

		if( forward_flag != 0 ) {

			if( get_forward_edge(decoder->current_node) ) {
				bit_arr[i] = '1';
				forward_flag = 2;
			} else if( get_backedge(decoder->current_node) && !( (h_idx - get_node_idx(get_backedge(decoder->current_node))-1) & 1 ) ) {
				bit_arr[i] = '1';
				GRAPH* dest_node = get_backedge(decoder->current_node);
				WM_NODE* dest = dest_node->data;

				// check if it is inner node
				if( is_inner_node(decoder, dest_node) ) {
					#ifdef DEBUG
						fprintf(stderr, "Invalid backedge detected!\n");
					#endif
					free(bit_arr);
					bit_arr[i]='x';
				} else {
				    pop_stacks(&decoder->stacks, dest->stack_idx, dest->hamiltonian_idx);
                }

			} else {
				bit_arr[i] = '0';
				GRAPH* dest_node = get_backedge(decoder->current_node);
				if(dest_node) {

					// check if it is inner node
					if( is_inner_node(decoder, dest_node) ) {
						#ifdef DEBUG
							fprintf(stderr, "Invalid backedge detected!\n");
						#endif
						free(bit_arr);
						bit_arr[i]='x';
					}
					WM_NODE* dest = dest_node->data;
					pop_stacks(&decoder->stacks, dest->stack_idx, dest->hamiltonian_idx);
				}
			}
		} else {
			i--;
		}

		if( forward_flag >= 0 ) forward_flag--;

		GRAPH* next = decoder->current_node->next;	
		label_new_current_node(decoder, decoder->current_node, h_idx);
		decoder->current_node = next;
		h_idx++;
	}
    decoder_free(decoder);

	return bit_arr;
}

void* watermark2017_decode_analysis_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_bytes) {

    uint8_t* result = watermark2017_decode_analysis(graph, num_bytes);

    unsigned long result_n_bits = *num_bytes;
    unsigned long result_num_bytes = result_n_bits / 8 + !!(result_n_bits % 8);
    unsigned long result_total_n_bits = result_num_bytes * 8;
    uint8_t* final_result = malloc(sizeof(uint8_t) * result_num_bytes);
    memset(final_result, 0x00, result_num_bytes);

    // turn 'result' into an actual bit sequence, turn 'x' into a random bit value
    srand(time(NULL));
    for(unsigned long i = 0; i < result_n_bits; i++) {

        if( result[ result_n_bits - i - 1 ] == 'x' ) {

            // set to wrong bit
            set_bit(final_result, result_total_n_bits - i - 1, rand() & 1);
        } else {
            // set to result bit
            set_bit(final_result, result_total_n_bits - i - 1, result[ result_n_bits - i - 1 ] - '0');
        }
    }

    // use reed solomon on 'final_result'
    unsigned long payload_num_bytes = result_num_bytes - num_rs_bytes * sizeof(uint16_t);

	int res = rs_decode(final_result, payload_num_bytes, (uint16_t*)( (uint8_t*)final_result + payload_num_bytes ), num_rs_bytes);

	// if there were no errors or they were corrected
	if( res >= 0 ) {

        // convert 'final_result' values into array of bits
        free(result);
        unsigned long payload_total_n_bits = payload_num_bytes * 8;
        unsigned long payload_n_bits = payload_total_n_bits - get_trailing_zeroes(final_result, payload_total_n_bits/8);
        *num_bytes = payload_n_bits;
        result = malloc( sizeof(uint8_t) * payload_n_bits );
        for(unsigned long i = 0; i < payload_n_bits; i++) result[ payload_n_bits - i - 1 ] = '0' + !!get_bit(final_result, payload_total_n_bits - i - 1);
        free(final_result);
    } else {
        // an error happened
        free(final_result);
    }
    return result;   
}
