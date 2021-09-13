#include "decoder/decoder.h"

uint8_t* watermark2014_decode(GRAPH* graph, unsigned long* num_bytes) {

    unsigned long n_bits = graph->num_nodes-1;
    uint8_t* bits = malloc(n_bits);
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
    free(bits);
    return data;
}

uint8_t* watermark_decode(GRAPH*, unsigned long* num_bytes);
