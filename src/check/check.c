#include "check/check.h"

typedef struct GRAPH_ARR {

    GRAPH** nodes;
    unsigned long n;
} GRAPH_ARR;

typedef struct CHECKER {
    uint8_t* bit_arr;
    unsigned long n_bits;

    GRAPH_ARR possible_removed_forward_edges;

    STACKS stacks;

    GRAPH* graph;
    GRAPH* node;
    GRAPH* final_node;
} CHECKER;

typedef struct CHECKER_NODE {

    // 0-based
    unsigned long hamiltonian_idx;

    // bit arr idx 0-based
    unsigned long bit_idx;

    // '1'/'0' or 'm' if mute or 'x' if unknown
    char bit;

    // all of them are 0-based
    unsigned long stack_idx;
    unsigned long backedge_to_idx;
    uint8_t has_backedge;
 
} CHECKER_NODE;

void checker_graph_to_utils_nodes(GRAPH* graph) {

    for(GRAPH* node = graph; node; node = node->next) {

        if(node->data && node->data_len) {

            CHECKER_NODE* decoder_node = node->data;
            UTILS_NODE utils_node = {
                .h_idx = decoder_node->hamiltonian_idx+1,
                .bit_idx = decoder_node->bit_idx,
                .bit = decoder_node->bit
            };
            graph_alloc(node, sizeof(utils_node));
            memcpy(node->data, &utils_node, node->data_len);
        }
    }
}

void checker_print_node(void* data, unsigned long data_len) {

    if(data && data_len == sizeof(CHECKER_NODE)) {
        utils_print_node(data, sizeof(UTILS_NODE));
    } else {
        printf("\x1b[33m null \x1b[0m");
    }
}

uint8_t _is_mute(GRAPH* node) {

    // check if it has a connection that isn't the hamiltonian edge
    for(CONNECTION* conn = node->connections; conn; conn = conn->next) {
        if( conn->node != node->next ) {
            return 0;
        }
    }
    return 1;
}

void _get_possibly_removed_forward_edges(CHECKER* checker) {

    unsigned long i=0;
    for(GRAPH* node = checker->graph; node && node->next && node->next->next && node->next->next->next; node = node->next) {
        i++;
        if( _is_mute(node) &&
            _is_mute(node->next) &&
            _is_mute(node->next->next) ) {

            checker->possible_removed_forward_edges.nodes = realloc(checker->possible_removed_forward_edges.nodes,
                    (++checker->possible_removed_forward_edges.n) * sizeof(GRAPH*));
            checker->possible_removed_forward_edges.nodes[checker->possible_removed_forward_edges.n-1] = node;
            printf("%lu ", i);
        }
    }
    printf("\n");
}

CHECKER* _checker_create(GRAPH* graph, void* data, unsigned long data_len) {

    CHECKER* checker = malloc(sizeof(CHECKER));

    checker->possible_removed_forward_edges.nodes = NULL;
    checker->possible_removed_forward_edges.n = 0;

    checker->graph = checker->node = graph;

    unsigned long trailing_zeros = get_trailing_zeroes(data, data_len);
    checker->n_bits = data_len * 8 - trailing_zeros;

    checker->bit_arr = malloc(checker->n_bits);
	for(unsigned long i = trailing_zeros+1; i < data_len * 8; i++) {

        unsigned long idx = i - trailing_zeros;
        checker->bit_arr[idx] = get_bit(data, i);
    }

    for(GRAPH* node=graph; node; node = node->next) checker->final_node = node;

    _get_possibly_removed_forward_edges(checker);

    create_stacks(&checker->stacks, checker->n_bits);
    return checker;
}

void _checker_register_node(CHECKER* checker, unsigned long h_idx, unsigned long bit_idx, char bit, int forward_flag) {

	// get parity	
	uint8_t is_odd = !(h_idx & 1);

	// get indexes
	unsigned long stack_idx = get_parity_stack(&checker->stacks, is_odd)->n;

    GRAPH* backedge = get_backedge(checker->node);

    CHECKER_NODE checker_node = {
        .hamiltonian_idx = h_idx,
        .stack_idx = stack_idx,
        .backedge_to_idx = backedge ? ((CHECKER_NODE*)backedge->data)->hamiltonian_idx : 0,
        .has_backedge = !!backedge,
        .bit_idx = bit_idx,
        .bit = bit
    };
    graph_alloc(checker->node, sizeof(CHECKER_NODE));
    memcpy(checker->node->data, &checker_node, sizeof(CHECKER_NODE));

    // backedge origin can't be used as backedge destination
	if(!backedge && forward_flag != 1) add_node_to_stacks(&checker->stacks, checker->node, h_idx, is_odd);

    checker->node = checker->node->next;
}

void _checker_free(CHECKER* checker) {
    free(checker->possible_removed_forward_edges.nodes);
    free(checker->bit_arr);
    free_stacks(&checker->stacks);
    free(checker);
}

unsigned long _checker_get_node_idx(GRAPH* node) {

    return ((CHECKER_NODE*)node->data)->hamiltonian_idx;
} 

uint8_t _checker_is_inner_node(CHECKER* decoder, GRAPH* node) {

	CHECKER_NODE* node_data = node->data;
	uint8_t is_odd = !(node_data->hamiltonian_idx & 1);

	PSTACK* stack = get_parity_stack(&decoder->stacks, is_odd);

	return !( node_data->stack_idx < stack->n && stack->stack[node_data->stack_idx] == node );
}

uint8_t _is_mute_until_end(GRAPH* node) {

    for(; node; node = node->next) {

        if( !_is_mute(node) ) return 0;
    }
    return 1;
}

GRAPH* _get_fourth_last(GRAPH* last) {

    last = last->prev;
    for(uint8_t i=0; i < 3; i++) {

        if(!last->prev) return NULL;
        last = last->prev;
    }

    return _is_mute_until_end(last) ? last : NULL;
}

void remove_first_node(CHECKER* checker) {

    for(unsigned long i = 1; i < checker->possible_removed_forward_edges.n; i++) {
        checker->possible_removed_forward_edges.nodes[i-1] = checker->possible_removed_forward_edges.nodes[i];
    }
    checker->possible_removed_forward_edges.nodes = realloc(checker->possible_removed_forward_edges.nodes,
            --checker->possible_removed_forward_edges.n * sizeof(GRAPH*));
}

int possible_forward_edge_check(CHECKER* checker) {
    uint8_t res = checker->possible_removed_forward_edges.n && checker->possible_removed_forward_edges.nodes[0] == checker->node;

    // every node is only evaluated once
    if(res) remove_first_node(checker);
    return res;
}

uint8_t _watermark_check(CHECKER* checker) {

	// if positive, decrement it, if it is zero the current node is a forward destination
	int forward_flag = -1;

	unsigned long h_idx=1;

    unsigned long max_n_forward_edges = get_num_bits(checker->graph) - checker->n_bits;
    unsigned long current_n_forward_edges = 0;

    _checker_register_node(checker, 0, 0, '1', 0);

    GRAPH* fourth_last = _get_fourth_last(checker->final_node);

    char bit = 'x';

    // iterate over every node but the last
	for(unsigned long i=1; i < checker->n_bits; i++ ) {
        if(!checker->node) {
            #ifdef DEBUG
                graph_print(checker->graph, checker_print_node);
                fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: end of graph reached too early\n", i);
            #endif
            return 0;
        }
		if( forward_flag != 0 ) {

			if( get_forward_edge(checker->node) ) {
                current_n_forward_edges++;
				if(!checker->bit_arr[i]) {
                    #ifdef DEBUG
                        graph_print(checker->graph, checker_print_node);
                        fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0 for node with outgoing forward edge\n", i);
                    #endif
                    return 0;
                }
                bit = '1';
				forward_flag = 2;
			} else if( get_backedge(checker->node) && !( (h_idx - _checker_get_node_idx(get_backedge(checker->node))-1) & 1 ) ) {
				if(!checker->bit_arr[i]) {
                    #ifdef DEBUG
                        graph_print(checker->graph, checker_print_node);
                        fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0 for node with backedge with odd index difference\n", i);
                    #endif
                    return 0;
                }
				GRAPH* dest_node = get_backedge(checker->node);

				// check if it is inner node
				if( _checker_is_inner_node(checker, dest_node) ) {
                    #ifdef DEBUG
                        graph_print(checker->graph, checker_print_node);
                        fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: backedge goes to an inner node\n", i);
                    #endif
					return 0;
				}

                bit = '1';
				CHECKER_NODE* dest = dest_node->data;
				pop_stacks(&checker->stacks, dest->stack_idx, dest->hamiltonian_idx);
			} else {
				GRAPH* dest_node = get_backedge(checker->node);

				if(dest_node) {

					// check if it is inner node
					if( _checker_is_inner_node(checker, dest_node) ) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit 0 backedge goes to an inner node\n", i);
                        #endif
						return 0;
					}

                    if(checker->bit_arr[i]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 1 for even backedge\n", i);
                        #endif
                        return 0;
                    }
                    bit = '0';
                    CHECKER_NODE* dest = dest_node->data;
					pop_stacks(&checker->stacks, dest->stack_idx, dest->hamiltonian_idx);
                // using removed hamiltonian edge, we can identify this node as a 1 even if its backedge was removed
				} else if ( !connection_search_node(checker->node->connections, checker->node->next) ) {
                    
                    if(!checker->bit_arr[i]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0, but there is a removed hamiltonian path\n", i);
                        #endif
                        return 0;
                    }
                    bit = '1';
                // using removed hamiltonian edge, we can identify this node as a 1 even if its forward edge was removed
                } else if ( checker->node->next && checker->node->next->next &&
                        !connection_search_node(checker->node->next->connections, checker->node->next->next) ) {

                    current_n_forward_edges++;
                    if(!checker->bit_arr[i]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0, but there is a removed hamiltonian path in the next node\n", i);
                        #endif
                        return 0;
                    }
                    bit = '1';
                    forward_flag=2;
                // if the fourth last nodes are mute, there is a forward edge that was removed, so the bit must be 1
                } else if( fourth_last == checker->node && current_n_forward_edges < max_n_forward_edges ) {

                    if(!checker->bit_arr[i]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0, but there is a removed forward edge here\n", i);
                        #endif
                        return 0;
                    }
                    if(checker->bit_arr[i+1]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 1, but the second mute node in a tail sequence must be 0\n", i+1);
                        #endif
                        return 0;
                    }

                    if( ++current_n_forward_edges != max_n_forward_edges ) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: not enough forward edges were detected, something went wrong\n", i);
                        #endif
                        return 0;
                    }
                    _checker_register_node(checker, h_idx, i, '1', 0);
                    bit = '0';
                    return 1;
                // if this node is the beginning of a three node mute group, and it is supposed to be 1 and there is no possible backedge from it
                } else if(
                        possible_forward_edge_check(checker) &&
                        checker->bit_arr[i] && (
                            // check if the node has an even hamiltonian idx and can't connect to anyone, not even 1
                            (
                                (h_idx & 1) && 
                                get_parity_stack(&checker->stacks, 1)->n == 1 &&
                                connection_search_node(checker->node->prev->connections, checker->graph)
                            // check if the node has an odd hamiltonian idx and can't connect to anyone
                            ) || (
                                !(h_idx & 1) &&
                                !get_parity_stack(&checker->stacks, 0)->n
                            ) 
                        )
                        ) {

                    current_n_forward_edges++;

                    if(checker->bit_arr[i+1]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "checker failed:\n\tidx: %lu\n\treason: ", i+1);
                        #endif
                    }
                    _checker_register_node(checker, h_idx, i, '0', 2);
                    i++;
                    h_idx++;
                    forward_flag=1;
                    bit = '0';

                } else {
                    if(checker->bit_arr[i]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: mute node should 0\n", i);
                        #endif
                        return 0;
                    }
                    bit = '0';
                }
			}
		} else {
			i--;
		}


        _checker_register_node(checker, h_idx, i, !forward_flag ? UTILS_MUTE_NODE : bit, forward_flag);
		h_idx++;

		if( forward_flag >= 0 ) forward_flag--;
	}
    return 1;
}

uint8_t watermark_check(GRAPH* graph, void* data, unsigned long data_len) {

    if(!is_graph_structure_valid(graph)) {
        #ifdef DEBUG
            fprintf(stderr, "check failed:\n\treason: invalid graph structure\n");
        #endif
        return 0;
    }

    CHECKER* checker = _checker_create(graph, data, data_len);

    uint8_t result = _watermark_check(checker);

    _checker_free(checker);
    return result;
}

uint8_t* watermark_check_analysis(GRAPH* graph, void* data, unsigned long* num_bytes) {

    CHECKER* checker = _checker_create(graph, data, *num_bytes);

    uint8_t* bit_arr = malloc( sizeof(char) * checker->n_bits );
    memset(bit_arr, 'x', checker->n_bits * sizeof(char));

    *num_bytes = checker->n_bits;

	// if positive, decrement it, if it is zero the current node is a forward destination
	int forward_flag = -1;

	unsigned long h_idx=1;

    unsigned long max_n_forward_edges = get_num_bits(checker->graph) - checker->n_bits;
    unsigned long current_n_forward_edges = 0;

    _checker_register_node(checker, 0, 0, '1', 0);
    bit_arr[0]='1';

    GRAPH* fourth_last = _get_fourth_last(checker->final_node);

    // iterate over every node but the last
	for(unsigned long i=1; i < checker->n_bits; i++ ) {
        if(!checker->node) {
            #ifdef DEBUG
                graph_print(checker->graph, checker_print_node);
                fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: end of graph reached too early\n", i);
            #endif
            break;
        }

        printf("h_idx: %lu\n", h_idx);
        printf("has backedge: %d\n", !!get_backedge(checker->node));
        utils_print_stacks(&checker->stacks, checker_print_node);
		if( forward_flag != 0 ) {

			if( get_forward_edge(checker->node) ) {
                current_n_forward_edges++;
				if(!checker->bit_arr[i]) {
                    #ifdef DEBUG
                        graph_print(checker->graph, checker_print_node);
                        fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0 for node with outgoing forward edge\n", i);
                    #endif
                } else {
				    forward_flag = 2;
                    bit_arr[i] = '1';
                }
			} else if( get_backedge(checker->node) && ( (h_idx - _checker_get_node_idx(get_backedge(checker->node))) & 1 ) ) {
				if(!checker->bit_arr[i]) {
                    #ifdef DEBUG
                        graph_print(checker->graph, checker_print_node);
                        fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0 for node with backedge with odd index difference\n", i);
                    #endif
                } else {
                    GRAPH* dest_node = get_backedge(checker->node);

                    // check if it is inner node
                    if( _checker_is_inner_node(checker, dest_node) ) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: backedge goes to an inner node\n", i);
                        #endif
                    } else {
                        bit_arr[i] = '1';
                    }
                    CHECKER_NODE* dest = dest_node->data;
                    pop_stacks(&checker->stacks, dest->stack_idx, dest->hamiltonian_idx);
                }
			} else {
				GRAPH* dest_node = get_backedge(checker->node);

				if(dest_node) {

					// check if it is inner node
					if( _checker_is_inner_node(checker, dest_node) ) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit 0 backedge goes to an inner node\n", i);
                        #endif
						bit_arr[i] = 'x';
					} else {

                        if(checker->bit_arr[i]) {
                            #ifdef DEBUG
                                graph_print(checker->graph, checker_print_node);
                                fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 1 for even backedge\n", i);
                            #endif
                        } else {
                            bit_arr[i] = '0';
                        }
                        CHECKER_NODE* dest = dest_node->data;
                        pop_stacks(&checker->stacks, dest->stack_idx, dest->hamiltonian_idx);
                    }
                // using removed hamiltonian edge, we can identify this node as a 1 even if its backedge was removed
				} else if ( !connection_search_node(checker->node->connections, checker->node->next) ) {
                    
                    if(!checker->bit_arr[i]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0, but there is a removed hamiltonian path\n", i);
                        #endif
                    } else {
                        bit_arr[i] = '1';
                    }
                // using removed hamiltonian edge, we can identify this node as a 1 even if its forward edge was removed
                } else if ( checker->node->next && checker->node->next->next &&
                        !connection_search_node(checker->node->next->connections, checker->node->next->next) ) {

                    current_n_forward_edges++;
                    if(!checker->bit_arr[i]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0, but there is a removed hamiltonian path in the next node\n", i);
                        #endif
                    } else {
                        bit_arr[i] = '1';
                        forward_flag=2;
                    }
                // if the fourth last nodes are mute, there is a forward edge that was removed, so the bit must be 1
                } else if( fourth_last == checker->node && current_n_forward_edges < max_n_forward_edges ) {

                    if(!checker->bit_arr[i]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0, but there is a removed forward edge here\n", i);
                        #endif
                    } else {
                        bit_arr[i] = '1';
                    }
                    if(checker->bit_arr[i+1]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 1, but the second mute node in a tail sequence must be 0\n", i+1);
                        #endif
                    } else {
                        bit_arr[i+1] = '0';
                    }

                    if( ++current_n_forward_edges != max_n_forward_edges ) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: not enough forward edges were detected, something went wrong\n", i);
                        #endif
                        break;
                    }
                    break;
                // if this node is the beginning of a three node mute group, and it is supposed to be 1 and there is no possible backedge from it
                } else if(
                        possible_forward_edge_check(checker) &&
                        checker->bit_arr[i] && (
                            // check if the node has an even hamiltonian idx and can't connect to anyone, not even 1
                            (
                                (h_idx & 1) && 
                                get_parity_stack(&checker->stacks, 1)->n == 1 &&
                                connection_search_node(checker->node->prev->connections, checker->graph)
                            // check if the node has an odd hamiltonian idx and can't connect to anyone
                            ) || (
                                !(h_idx & 1) &&
                                // can't connect if the previous node connects to it
                                (
                                  !get_parity_stack(&checker->stacks, 0)->n ||
                                  (
                                    get_parity_stack(&checker->stacks, 0)->n == 1 &&
                                    connection_search_node(checker->node->prev->connections, checker->graph) &&
                                    connection_search_node(checker->node->prev->connections, checker->graph)->node ==
                                    get_parity_stack(&checker->stacks, 0)->stack[0]
                                  )
                                )
                            ) 
                        )
                        ) {
                    current_n_forward_edges++;
                    bit_arr[i] = '1';
                    forward_flag = 2;
                
                } else {
                    if(checker->bit_arr[i]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, checker_print_node);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: mute node should be 0\n", i);
                        #endif
                    } else {
                        bit_arr[i] = '0';
                    }
                }
			}
		} else {
			i--;
		}

        _checker_register_node(checker, h_idx, i, !forward_flag ? UTILS_MUTE_NODE : bit_arr[i], forward_flag);
		h_idx++;

        if( forward_flag >= 0 ) forward_flag--;
	}

    _checker_free(checker);

    return bit_arr;
}

uint8_t* watermark_check_analysis_with_rs(GRAPH* graph, void* data, unsigned long* num_bytes, unsigned long num_rs_parity_symbols) {

    //unsigned long payload_total_n_bits = (*num_bytes) * 8;
    unsigned long data_total_n_bytes = (*num_bytes) + num_rs_parity_symbols * sizeof(uint16_t);
    unsigned long data_total_n_bits = data_total_n_bytes*8;

    // generate parity bytes again and add them to 'data', resulting in 'final_data'
    uint8_t final_data[data_total_n_bytes];
    memset(final_data, 0x00, data_total_n_bytes);
    memcpy(final_data, data, *num_bytes);

    rs_encode(final_data, *num_bytes, (uint16_t*)(&final_data[*num_bytes]), num_rs_parity_symbols);

    uint8_t* result = watermark_check_analysis(graph, final_data, &data_total_n_bytes);
    unsigned long result_n_bits = data_total_n_bytes;
    unsigned long result_num_bytes = result_n_bits / 8 + !!(result_n_bits % 8);
    unsigned long result_total_n_bits = result_num_bytes * 8;
    uint8_t* final_result = malloc(sizeof(uint8_t) * result_num_bytes);
    memset(final_result, 0x00, result_num_bytes);

    // turn 'result' into an actual bit sequence, turn 'x' into the wrong bit of the sequence
    for(unsigned long i = 0; i < result_n_bits; i++) {

        // if this is true, the result is too small
        if( data_total_n_bits == i ) {

            free(result);
            return NULL;
        }

        if( result[ result_n_bits - i - 1 ] == 'x' ) {

            // set to wrong bit
            set_bit(final_result, result_total_n_bits - i - 1, !get_bit(final_data, data_total_n_bits - i -1));
        } else {
            // set to result bit
            set_bit(final_result, result_total_n_bits - i - 1, result[ result_n_bits - i - 1 ] - '0');
        }
    }

    // since we know the true parity bits, we can use them to correct the payload

    // use reed solomon on 'final_result'
    unsigned long payload_num_bytes = result_num_bytes - ( num_rs_parity_symbols * sizeof(uint16_t) );

	int res = rs_decode(final_result, payload_num_bytes, (uint16_t*)( (uint8_t*)&final_data[*num_bytes] ), num_rs_parity_symbols);

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
