#include "decoder/decoder.h"

void* watermark2014_decode(GRAPH* graph, unsigned long* num_bytes) {

    unsigned long n_bits = graph->num_nodes-1;
    uint8_t bits[n_bits];
    bits[0] = 1;
    for(unsigned long i = 1; i < n_bits; i++) {

        // if this node has two out connections
        if(graph->nodes[i]->num_out_neighbours == 2) {

            // get backedge
            CONNECTION* conn = graph_get_backedge(graph->nodes[i]);
            bits[i] = ( graph->nodes[i]->graph_idx - conn->node->graph_idx ) & 1;
        } else {
            bits[i] = 0;
        }
    }
    void* data = get_sequence_from_bit_arr(bits, n_bits, num_bytes);
    return data;
}

int watermark_decode_improved_four_last_are_mute(GRAPH* graph) {
  return graph->num_nodes > 3 &&
    graph->nodes[graph->num_nodes-1]->num_out_neighbours == 0 && graph->nodes[graph->num_nodes-1]->num_in_neighbours == 1 &&
    graph->nodes[graph->num_nodes-2]->num_out_neighbours == 1 && graph->nodes[graph->num_nodes-2]->num_in_neighbours == 1 && graph_get_connection(graph->nodes[graph->num_nodes-2], graph->nodes[graph->num_nodes-1]) &&
    graph->nodes[graph->num_nodes-3]->num_out_neighbours == 1 && graph->nodes[graph->num_nodes-3]->num_in_neighbours == 1 && graph_get_connection(graph->nodes[graph->num_nodes-3], graph->nodes[graph->num_nodes-2]) &&
    graph->nodes[graph->num_nodes-4]->num_out_neighbours == 1 && graph->nodes[graph->num_nodes-4]->num_in_neighbours == 1 && graph_get_connection(graph->nodes[graph->num_nodes-4], graph->nodes[graph->num_nodes-3]);
}

int watermark_decode_improved_sequence_of_three(GRAPH* graph, unsigned long graph_idx) {
  return graph->num_nodes - graph_idx - 1 > 3 &&
  graph->nodes[graph_idx]->num_out_neighbours == 1 && graph_get_connection(graph->nodes[graph_idx], graph->nodes[graph_idx+1]) &&
  graph->nodes[graph_idx+1]->num_out_neighbours == 1 && graph_get_connection(graph->nodes[graph_idx+1], graph->nodes[graph_idx+2]) &&
  graph->nodes[graph_idx+2]->num_out_neighbours == 1 && graph_get_connection(graph->nodes[graph_idx+2], graph->nodes[graph_idx+3]);
}

void* watermark_decode_improved(GRAPH* graph, uint8_t* data, unsigned long* num_bytes) {

    unsigned long data_num_bits = *num_bytes * 8;
    unsigned long data_begin = get_first_positive_bit_index(data, *num_bytes);

    unsigned long n_bits = graph->num_nodes-2;
    uint8_t bits[n_bits];
    bits[0] = 1;
    unsigned long i = 1;

    STACK* odd_stack = stack_create(n_bits);
    STACK* even_stack = stack_create(n_bits);
    stack_push(odd_stack, 0);
    unsigned long* history = calloc(n_bits*2, sizeof(unsigned long));

    uint8_t four_last_are_mute = watermark_decode_improved_four_last_are_mute(graph);

    uint8_t node_29_was_the_last = 0;
    uint8_t node_27_was_the_last = 0;
    unsigned long forward_edges_left = graph->num_nodes - 2 - (data_num_bits - data_begin);
    uint8_t forward_destination = 0;
    for(unsigned long graph_idx = 1; graph_idx < n_bits; graph_idx++, i++) {

        if(data_begin + i >= data_num_bits) break;
        uint8_t bit = get_bit(data, data_begin + i);
        uint8_t is_odd = !(graph_idx&1);
        STACK* possible_backedges = (is_odd && bit) || (!is_odd && !bit) ? even_stack : odd_stack ;
        STACK* other_stack = possible_backedges == even_stack ? odd_stack : even_stack;

        if(forward_destination) forward_destination--;
        // if it isn't a mute node
        if(!( graph_idx > 1 && graph_get_connection(graph->nodes[graph_idx-2], graph->nodes[graph_idx])) && forward_destination != 1 ) {

            // 2.2 if it has a forward edge, it encodes a bit 1
            if( graph_idx < n_bits && graph_get_connection(graph->nodes[graph_idx], graph->nodes[graph_idx+2]) ) {
                forward_edges_left--;
                bits[i]=1;
            // 2.1/2.4 encode bit according to backedge
            } else if( graph_get_backedge(graph->nodes[graph_idx]) ){
                CONNECTION* backedge = graph_get_backedge(graph->nodes[graph_idx]);
                bits[i] = backedge && (( graph_idx - backedge->node->graph_idx ) & 1);
                // pop stacks
                if(backedge) {
                  NODE* backedge_node = backedge->node;
                  // look for backedge node index in the stack
                  unsigned long backedge_index = 0;
                  for(unsigned long i = 0; i < possible_backedges->n; i++) {
                    if(possible_backedges->stack[i] == backedge_node->graph_idx) {
                      backedge_index = i;
                      break;
                    }
                  }
                  stack_pop_until(possible_backedges, backedge_index); // pop backedge and all nodes on top of it
                  stack_pop_until(other_stack, history[backedge_index]);
                }
                node_29_was_the_last = 0;
                node_27_was_the_last = 0;
                continue;
            // 2.5 if hamiltonian edge [v -> v+1] doesn't exist, v encodes 1
            } else if( !graph_get_connection(graph->nodes[graph_idx], graph->nodes[graph_idx+1]) ) {
              bits[i] = 1;
            // 2.6 if hamiltonian edge [v+1 -> v+2] doesn't exist, v encodes 1
            } else if( !graph_get_connection(graph->nodes[graph_idx+1], graph->nodes[graph_idx+2]) ) {
              bits[i] = 1;
              forward_destination = 3;
              forward_edges_left--;
            // 2.7 if node is fourth to last and four last nodes are mute, v encodes 1
            } else if(four_last_are_mute && graph_idx == graph->num_nodes-4 && forward_edges_left && bit) {
              bits[i] = 1;
              node_27_was_the_last = 1;
              forward_destination = 3;
              forward_edges_left--;
              continue;
            // 2.8 if node is third to last and four last nodes are mute, v encodes 0
            } else if(four_last_are_mute && graph_idx == graph->num_nodes-3 && node_27_was_the_last) {
              bits[i] = 0;
            // 2.9 v is the first in a sequence of three nodes without back or forward edges, v should encode 1 and
            // it isn't possible to create a backedge in v, v encodes 0
            } else if( watermark_decode_improved_sequence_of_three(graph, graph_idx) && 
                !has_possible_backedge( possible_backedges, graph, graph_idx) && bit) {
              bits[i] = 1;
              node_29_was_the_last = 1;
              forward_destination = 3;
              forward_edges_left--;
              continue;
            // 2.10 if v - 1 is the node above, this one encodes 0
            } else if( node_29_was_the_last ) {
              bits[i] = 0;
            // 2.11 if everything above is false, this node encodes 0
            } else {
              bits[i] = 0;
            }
        } else {
            i--;
        }
        node_27_was_the_last = 0;
        node_29_was_the_last = 0;

        // if this is not a inner forward node
        if(!graph_get_forward(graph->nodes[graph_idx-1])) {

            // save stacks
            // odd
            if(is_odd) {
                stack_push(odd_stack, graph_idx);
                history[graph_idx] = even_stack->n;
            // even
            } else {
                stack_push(even_stack, graph_idx);
                history[graph_idx] = odd_stack->n;
            }
        }
    }

    stack_free(odd_stack);
    stack_free(even_stack);
    free(history);
    // if the second last node is a forward edge destination, the
    // third last node also needs to be ignored
    uint8_t is_prev_last_forward_destination = !!( n_bits > 2 && graph_get_connection(graph->nodes[n_bits-2], graph->nodes[n_bits]) && data_begin + i < data_num_bits );
    n_bits = i-is_prev_last_forward_destination;
    // bit sequence may be smaller than expected due to mute nodes
    void* res = get_sequence_from_bit_arr(bits, n_bits, num_bytes);
    return res;
}

void* watermark_decode(GRAPH* graph, unsigned long* num_bytes) {

    unsigned long n_bits = graph->num_nodes-2;
    uint8_t bits[n_bits];
    bits[0] = 1;
    unsigned long i = 1;

    for(unsigned long graph_idx = 1; graph_idx < n_bits; graph_idx++, i++) {

        // if it isn't a mute node
        if(!( graph_idx > 1 && graph_get_connection(graph->nodes[graph_idx-2], graph->nodes[graph_idx])) ) {

            // if it has a forward edge
            if( graph_idx < n_bits && graph_get_connection(graph->nodes[graph_idx], graph->nodes[graph_idx+2]) ) {
                bits[i]=1;
            } else {
                CONNECTION* backedge = graph_get_backedge(graph->nodes[graph_idx]);
                bits[i] = backedge && (( graph_idx - backedge->node->graph_idx ) & 1);
            }
        } else {
            i--;
        }
    }
    // if the second last node is a forward edge destination, the
    // third last node also needs to be ignored
    uint8_t is_prev_last_forward_destination = !!( n_bits > 2 && graph_get_connection(graph->nodes[n_bits-2], graph->nodes[n_bits]) );
    n_bits = i-is_prev_last_forward_destination;
    // bit sequence may be smaller than expected due to mute nodes
    void* data = get_sequence_from_bit_arr(bits, n_bits, num_bytes);
    return data;
}

void* watermark2014_rs_decode(GRAPH* graph, unsigned long* num_parity_symbols) {

    unsigned long data_len;
    uint8_t* data = watermark2014_decode(graph, &data_len);
    return remove_rs_code(data, data_len, num_parity_symbols);
}

void* watermark_rs_decode(GRAPH* graph, unsigned long* num_parity_symbols) {

    unsigned long data_len;
    uint8_t* data = watermark_decode(graph, &data_len);
    return remove_rs_code(data, data_len, num_parity_symbols);
}

void* _watermark_decode_analysis(GRAPH* graph, unsigned long* num_bytes) {

    // load UTILS_NODE in every node
    for(unsigned long i = 0; i < graph->num_nodes; i++) {
        UTILS_NODE* utils_node = malloc(sizeof(UTILS_NODE));
        node_load_info(graph->nodes[i], utils_node, sizeof(UTILS_NODE));
    }

    unsigned long n_bits = graph->num_nodes-2;
    uint8_t bits[n_bits];
    bits[0] = 1;
    unsigned long i = 1;

    for(unsigned long graph_idx = 1; graph_idx < n_bits; graph_idx++, i++) {

        // if it isn't a mute node
        if(!( graph_idx > 1 && graph_get_connection(graph->nodes[graph_idx-2], graph->nodes[graph_idx])) ) {

            // if it has a forward edge
            if( graph_idx < n_bits && graph_get_connection(graph->nodes[graph_idx], graph->nodes[graph_idx+2]) ) {
                ((UTILS_NODE*)node_get_info(graph->nodes[graph_idx]))->checker_bit = BIT_1;
                bits[i]=1;
            } else {
                CONNECTION* backedge = graph_get_backedge(graph->nodes[graph_idx]);
                bits[i] = backedge && (( graph_idx - backedge->node->graph_idx ) & 1);
                ((UTILS_NODE*)node_get_info(graph->nodes[graph_idx]))->checker_bit = bits[i] == '1' ? BIT_1 : BIT_0;
            }
        } else {
            ((UTILS_NODE*)node_get_info(graph->nodes[graph_idx]))->checker_bit = BIT_MUTE;
            i--;
        }
    }
    // if the second last node is a forward edge destination, the
    // third last node also needs to be ignored
    uint8_t is_prev_last_forward_destination = !!( n_bits > 2 && graph_get_connection(graph->nodes[n_bits-2], graph->nodes[n_bits]) );
    n_bits = i-is_prev_last_forward_destination;
    // bit sequence may be smaller than expected due to mute nodes
    void* data = get_sequence_from_bit_arr(bits, n_bits, num_bytes);
    return data;
}
void* watermark_decode_analysis(GRAPH* graph, unsigned long* num_bytes) {

    uint8_t* data = _watermark_decode_analysis(graph, num_bytes);
    unsigned long starting_idx = get_first_positive_bit_index(data, *num_bytes);
    unsigned long n_bits = (*num_bytes)*8 - starting_idx;
    
    uint8_t* bits = malloc(n_bits);
    for(unsigned long i = 0; i < n_bits; i++) {
        bits[i] = get_bit(data, starting_idx+i);
    }
    free(data);
    *num_bytes = n_bits;
    return bits;
}

void* watermark_rs_decode_analysis(GRAPH* graph, unsigned long* num_parity_symbols) {

    uint8_t* data = watermark_rs_decode(graph, num_parity_symbols);

    unsigned long starting_idx = get_first_positive_bit_index(data, *num_parity_symbols);
    unsigned long n_bits = (*num_parity_symbols)*8 - starting_idx;
    
    uint8_t* bits = malloc(n_bits);
    for(unsigned long i = 0; i < n_bits; i++) {
        bits[i] = get_bit(data, starting_idx+i);
    }
    free(data);
    *num_parity_symbols = n_bits;
    return bits;
}
