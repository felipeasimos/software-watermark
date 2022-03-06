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
