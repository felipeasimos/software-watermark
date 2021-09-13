#include "encoder/encoder.h"

GRAPH* watermark2014_encode(void* data, unsigned long data_len) {

    // get index of first positive bit
    unsigned long total_number_of_bits = data_len*8;
    unsigned long starting_idx = get_first_positive_bit_index(data, data_len);
    unsigned long n = total_number_of_bits - starting_idx;

    GRAPH* graph = graph_create(n+1);

    STACK* odd_stack = stack_create(n);
    STACK* even_stack = stack_create(n);
    unsigned long* history = calloc(n, sizeof(unsigned long));

    // iterate over the bits
    for(unsigned long i = starting_idx; i < total_number_of_bits; i++) {

        unsigned long idx = i - starting_idx;
        uint8_t bit = get_bit(data, i);
        uint8_t is_odd = idx & 1;

        // construct hamiltonian path
        graph_oriented_connect(graph->nodes[idx], graph->nodes[idx+1]);

        // add backedge
        STACK* possible_backedges = (is_odd && bit) || (!is_odd && !bit) ? even_stack : odd_stack ;
        STACK* other_stack = possible_backedges == even_stack ? odd_stack : even_stack;
        if( possible_backedges->n ) {

            unsigned long idx_of_backedge = rand() % possible_backedges->n;
            NODE* backedge_node = graph->nodes[possible_backedges->stack[idx_of_backedge]];
            graph_oriented_connect(graph->nodes[idx], backedge_node);
            stack_pop_until(possible_backedges, idx_of_backedge+1); // add +1 since we want the size, not the index
            stack_pop_until(other_stack, history[backedge_node->graph_idx]);
        }

        // save stacks
        // odd
        if(is_odd) {
            stack_push(odd_stack, idx);
            history[idx] = even_stack->n;
        // even
        } else {
            stack_push(even_stack, idx);
            history[idx] = odd_stack->n;
        }
    }

    stack_free(odd_stack);
    stack_free(even_stack);
    free(history);
    return graph;
}

GRAPH* watermark_encode(void* data, unsigned long data_len) {

    // get index of first positive bit
    unsigned long total_number_of_bits = data_len*8;
    unsigned long starting_idx = get_first_positive_bit_index(data, data_len);
    unsigned long n = data_len*8 - starting_idx;

    GRAPH* graph = graph_create(n+2);

    STACK* odd_stack = stack_create(n);
    STACK* even_stack = stack_create(n);
    unsigned long* history = calloc(n, sizeof(unsigned long));

    // iterate over the bits
    for(unsigned long i = starting_idx; i < total_number_of_bits; i++) {

        unsigned long idx = i-starting_idx;
        uint8_t bit = get_bit(data, i);
        uint8_t is_odd = idx&1;

        // construct hamiltonian path
        graph_oriented_connect(graph->nodes[idx], graph->nodes[idx+1]);

        // add backedge
        STACK* possible_backedges = (is_odd && bit) || (!is_odd && !bit) ? even_stack : odd_stack ;
        STACK* other_stack = possible_backedges == even_stack ? odd_stack : even_stack;
        if( possible_backedges->n ) {

            unsigned long idx_of_backedge = rand() % possible_backedges->n;
            graph_oriented_connect(graph->nodes[idx], graph->nodes[idx_of_backedge]);
            stack_pop_until(possible_backedges, idx_of_backedge+1); // add +1 since we want the size, not the index
            stack_pop_until(other_stack, history[idx_of_backedge]);
        }

        // save stacks
        // odd
        if(is_odd) {
            stack_push(odd_stack, idx);
            history[idx] = even_stack->n;
        // even
        } else {
            stack_push(even_stack, idx);
            history[idx] = odd_stack->n;
        }
    }

    stack_free(odd_stack);
    stack_free(even_stack);
    free(history);
    return graph;

}
