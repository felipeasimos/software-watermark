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
        uint8_t is_odd = !(idx & 1);

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
    unsigned long n = total_number_of_bits - starting_idx;

    GRAPH* graph = graph_create(n+2);

    // construct hamiltonian path
    for(unsigned long i = 1; i < graph->num_nodes; i++) graph_oriented_connect(graph->nodes[i-1], graph->nodes[i]);

    STACK* odd_stack = stack_create(n);
    STACK* even_stack = stack_create(n);
    stack_push(odd_stack, 0);
    unsigned long* history = calloc(n*2, sizeof(unsigned long));

    // iterate over the bits
    unsigned long idx = 1;
    for(unsigned long i = starting_idx+1; i < total_number_of_bits; i++, idx++) {

        uint8_t bit = get_bit(data, i);
        uint8_t is_odd = !(idx&1);

        // backedge management
        STACK* possible_backedges = (is_odd && bit) || (!is_odd && !bit) ? even_stack : odd_stack ;
        STACK* other_stack = possible_backedges == even_stack ? odd_stack : even_stack;
        // check if there is a forward edge that point to the current node and ignore it if so
        if( idx > 1 && graph_get_connection(graph->nodes[idx-2], graph->nodes[idx]) ) {
            i--;
        // if there are available backedges in the backedge stack, taking into account that we
        // can't connect a backedge to a node that the last node also does
        } else if( has_possible_backedge(possible_backedges, graph, idx) ) {

            // if v-1 has forward edge
            if( graph_get_connection(graph->nodes[idx-1], graph->nodes[idx+1]) ) {

                if(bit) {

                    // backedge to v-1
                    graph_oriented_connect(graph->nodes[idx], graph->nodes[idx-1]);
                    // remove hamiltonian edge
                    graph_oriented_disconnect(graph->nodes[idx], graph->nodes[idx+1]);
                    // while block is built!
                } else {
                    // nothing needs to be done, and we have an if block built!
                }
                // node inside forward edge can't be backedge destination
                continue;
            } else {

                // connect to backedge
                unsigned long backedge_idx = get_backedge_index(possible_backedges, graph, idx);
                NODE* backedge_node = graph->nodes[possible_backedges->stack[backedge_idx]];
                graph_oriented_connect(graph->nodes[idx], backedge_node);
                // pop stacks
                stack_pop_until(possible_backedges, backedge_idx); // pop backedge and all nodes on top of it
                stack_pop_until(other_stack, history[backedge_node->graph_idx]);
                // repeat block is built!
                // this node is now the origin of a backedge, so it isn't a valid
                // backedge destination
                continue;
            }
        } else {
            if(bit) {

                // add new node to graph and create forward edge
                graph_add(graph);
                graph_oriented_connect(graph->nodes[graph->num_nodes-2], graph->nodes[graph->num_nodes-1]);
                graph_oriented_connect(graph->nodes[idx], graph->nodes[idx+2]);
            }
        }

        // if this is not a inner forward node
        if(!graph_get_forward(graph->nodes[idx-1])) {

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
    }

    stack_free(odd_stack);
    stack_free(even_stack);
    free(history);
    return graph;

}

GRAPH* watermark2014_rs_encode(void* data, unsigned long data_len, unsigned long num_parity_symbols) {

    uint8_t* data_with_parity = append_rs_code8(data, &data_len, num_parity_symbols);
    GRAPH* graph = watermark2014_encode(data_with_parity, data_len);
    free(data_with_parity);
    return graph;
}

GRAPH* watermark_rs_encode(void* data, unsigned long data_len, unsigned long num_parity_symbols) {

    uint8_t* data_with_parity = append_rs_code8(data, &data_len, num_parity_symbols);
    GRAPH* graph = watermark_encode(data_with_parity, data_len);
    free(data_with_parity);
    return graph;
}
