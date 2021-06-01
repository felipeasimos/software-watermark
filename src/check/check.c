#include "check/check.h"

typedef struct CHECKER {
    uint8_t* bit_arr;
    unsigned long n_bits;

    STACKS stacks;

    GRAPH* graph;
    GRAPH* node;
    GRAPH* final_node;
} CHECKER;

typedef struct CHECKER_NODE {

    unsigned long hamiltonian_idx;
    unsigned long stack_idx;
    unsigned long backedge_to_idx;
    uint8_t has_backedge;

} CHECKER_NODE;

CHECKER* _checker_create(GRAPH* graph, void* data, unsigned long data_len) {

    CHECKER* checker = malloc(sizeof(CHECKER));

    checker->graph = checker->node = graph;

    unsigned long trailing_zeros = get_trailing_zeroes(data, data_len);

    checker->n_bits = data_len * 8 - trailing_zeros;

    checker->bit_arr = malloc(checker->n_bits);
	for(unsigned long i = trailing_zeros+1; i < data_len * 8; i++) {

        unsigned long idx = i - trailing_zeros;
        checker->bit_arr[idx] = get_bit(data, i);
    }

    for(GRAPH* node=graph; node; node = node->next) checker->final_node = node;

    create_stacks(&checker->stacks, checker->n_bits);
    return checker;
}

void _checker_register_node(CHECKER* checker, unsigned long h_idx) {

	// get parity	
	uint8_t is_odd = !(h_idx & 1);

	// get indexes
	unsigned long stack_idx = get_parity_stack(&checker->stacks, is_odd)->n;

    GRAPH* backedge = get_backedge_with_info(checker->node);

    CHECKER_NODE checker_node = {
        .hamiltonian_idx = h_idx,
        .stack_idx = stack_idx,
        .backedge_to_idx = backedge ? ((CHECKER_NODE*)graph_get_info(backedge))->hamiltonian_idx : 0,
        .has_backedge = !!backedge
    };
    CHECKER_NODE* checker_node_cpy = malloc(sizeof(CHECKER_NODE));
    memcpy(checker_node_cpy, &checker_node, sizeof(CHECKER_NODE));
	add_node_to_stacks(&checker->stacks, checker->node, h_idx, is_odd);

    graph_load_info(checker->node, checker_node_cpy, sizeof(CHECKER_NODE));
    checker->node = checker->node->next;
}

void _checker_free(CHECKER* checker) {

    for(GRAPH* node = checker->graph; node; node = node->next) {
        if( node->data_len == sizeof(INFO_NODE) && node->data ) {
            free(graph_get_info(node));
        }
    }

    graph_unload_all_info(checker->graph);

    free(checker->bit_arr);
    free_stacks(&checker->stacks);
    free(checker);
}

unsigned long _checker_get_node_idx(GRAPH* node) {

    return ((CHECKER_NODE*)graph_get_info(node))->hamiltonian_idx;
} 

uint8_t _checker_is_inner_node(CHECKER* decoder, GRAPH* node) {

	CHECKER_NODE* node_info = graph_get_info(node);
	uint8_t is_odd = !(node_info->hamiltonian_idx & 1);

	PSTACK* stack = get_parity_stack(&decoder->stacks, is_odd);

	return !( node_info->stack_idx < stack->n && stack->stack[node_info->stack_idx] == node );
}

uint8_t _is_mute_until_end(GRAPH* node) {

    for(; node; node = node->next) {

        // check if it has a connection that isn't the hamiltonian edge
        for(CONNECTION* conn = node->connections; conn; conn = conn->next) {
            if( conn->node != node->next ) {
                return 0;
            }
        }
    }
    return 1;
}

GRAPH* _get_fourth_last(GRAPH* last) {

    for(uint8_t i=0; i < 3; i++) {

        if(!last->prev) return NULL;
        last = last->prev;
    }

    return _is_mute_until_end(last) ? last : NULL;
}

uint8_t _watermark_check(CHECKER* checker) {

	// if positive, decrement it, if it is zero the current node is a forward destination
	int forward_flag = -1;

	unsigned long h_idx=1;

    unsigned long max_n_forward_edges = get_num_bits(checker->graph) - checker->n_bits;
    unsigned long current_n_forward_edges = 0;

    _checker_register_node(checker, 0);

    GRAPH* fourth_last = _get_fourth_last(checker->final_node);

	// iterate over every node but the last
	for(unsigned long i=1; i < checker->n_bits; i++ ) {
        if(!checker->node) {
            #ifdef DEBUG
                graph_print(checker->graph, NULL);
                fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: end of graph reached too early\n", i);
            #endif
            return 0;
        }
		if( forward_flag != 0 ) {

			if( get_forward_edge(checker->node) ) {
                current_n_forward_edges++;
				if(!checker->bit_arr[i]) {
                    #ifdef DEBUG
                        graph_print(checker->graph, NULL);
                        fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0 for node with outgoing forward edge\n", i);
                    #endif
                    return 0;
                }
				forward_flag = 2;
			} else if( get_backedge_with_info(checker->node) && !( (h_idx - _checker_get_node_idx(get_backedge_with_info(checker->node))-1) & 1 ) ) {
				if(!checker->bit_arr[i]) {
                    #ifdef DEBUG
                        graph_print(checker->graph, NULL);
                        fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0 for node with backedge with odd index difference\n", i);
                    #endif
                    return 0;
                }
				GRAPH* dest_node = get_backedge_with_info(checker->node);

				// check if it is inner node
				if( _checker_is_inner_node(checker, dest_node) ) {
                    #ifdef DEBUG
                        graph_print(checker->graph, NULL);
                        fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: backedge goes to an inner node\n", i);
                    #endif
					return 0;
				}

				CHECKER_NODE* dest = graph_get_info(dest_node);
				pop_stacks(&checker->stacks, dest->stack_idx, dest->hamiltonian_idx);
			} else {
				GRAPH* dest_node = get_backedge_with_info(checker->node);

				if(dest_node) {

					// check if it is inner node
					if( _checker_is_inner_node(checker, dest_node) ) {
                        #ifdef DEBUG
                            graph_print(checker->graph, NULL);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit 0 backedge goes to an inner node\n", i);
                        #endif
						return 0;
					}

                    if(checker->bit_arr[i]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, NULL);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 1 for even backedge\n", i);
                        #endif
                        return 0;
                    }
                    CHECKER_NODE* dest = graph_get_info(dest_node);
					pop_stacks(&checker->stacks, dest->stack_idx, dest->hamiltonian_idx);
                // using removed hamiltonian edge, we can identify this node as a 1 even if its backedge was removed
				} else if ( !connection_search_node(checker->node->connections, checker->node->next) ) {
                    
                    if(!checker->bit_arr[i]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, NULL);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0, but there is a removed hamiltonian path\n", i);
                        #endif
                        return 0;
                    }
                // using removed hamiltonian edge, we can identify this node as a 1 even if its forward edge was removed
                } else if ( checker->node->next && checker->node->next->next &&
                        !connection_search_node(checker->node->next->connections, checker->node->next->next) ) {

                    current_n_forward_edges++;
                    if(!checker->bit_arr[i]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, NULL);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0, but there is a removed hamiltonian path in the next node\n", i);
                        #endif
                        return 0;
                    }
                    forward_flag=2;
                // if the fourth last nodes are mute, there is a forward edge that was removed, so the bit must be 1
                } else if( fourth_last == checker->node && current_n_forward_edges < max_n_forward_edges ) {

                    if(!checker->bit_arr[i]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, NULL);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: bit is 0, but there is a removed forward edge here\n", i);
                        #endif
                        return 0;
                    }

                    if( ++current_n_forward_edges != max_n_forward_edges ) {
                        #ifdef DEBUG
                            graph_print(checker->graph, NULL);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: not enough forward edges were detected, something went wrong\n", i);
                        #endif
                        return 0;
                    }
                } else {
                    if(checker->bit_arr[i]) {
                        #ifdef DEBUG
                            graph_print(checker->graph, NULL);
                            fprintf(stderr, "check failed:\n\tidx: %lu\n\treason: mute node should 0\n", i);
                        #endif
                        return 0;
                    }
                }
			}
		} else {
			i--;
		}

		if( forward_flag >= 0 ) forward_flag--;

        _checker_register_node(checker, h_idx);
		h_idx++;
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
