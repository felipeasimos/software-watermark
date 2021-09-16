#include "decoder/decoder.h"

void* watermark2014_decode(GRAPH* graph, unsigned long* num_bytes) {

    unsigned long n_bits = graph->num_nodes-1;
    uint8_t bits[n_bits];
    bits[0] = 1;
    for(unsigned long i = 1; i < n_bits; i++) {

        // if this node has two out connections
        if(graph->nodes[i]->num_out_neighbours == 2) {

            // get backedge
            CONNECTION* conn = graph->nodes[i]->out->to->graph_idx < graph->nodes[i]->graph_idx ?
                graph->nodes[i]->out : graph->nodes[i]->out->next;
            bits[i] = ( graph->nodes[i]->graph_idx - conn->to->graph_idx ) & 1;
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
                // if it has an odd backedge
                CONNECTION* backedge = graph_get_backedge(graph->nodes[graph_idx]);
                if( backedge && (( graph_idx - backedge->to->graph_idx ) & 1) ) {
                    bits[i]=1;
                // if it has an even backedge
                } else {
                    bits[i]=0;
                }
            }
        } else {
            i--;
        }
    }
    // get the index of the last bit, which must be subtracted, in case it 
    // is a forward edge destination
    n_bits = i-!!( graph->num_nodes > 4 && graph_get_connection(graph->nodes[n_bits-2], graph->nodes[n_bits]) );
    // bit sequence may be smaller than expected due to mute nodes
    void* data = get_sequence_from_bit_arr(bits, n_bits, num_bytes);
    return data;
}

// 'num_parity_symbols' will hold the original data sequence size
uint8_t* remove_rs_code(uint8_t* data, unsigned long data_len, unsigned long* num_parity_symbols) {

    unsigned long original_size = data_len - sizeof(uint16_t) * (*num_parity_symbols);
    if( rs_decode(data, original_size, (uint16_t*)(data+original_size), *num_parity_symbols) != -1 ) {
        *num_parity_symbols = original_size;
        return realloc(data, original_size);
    } else {
        free(data);
        return NULL;
    }
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
