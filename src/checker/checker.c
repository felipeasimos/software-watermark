#include "checker/checker.h"

typedef enum CHECKER_BIT {
    BIT_0='0',
    BIT_1='1',
    BIT_MUTE='m',
    BIT_0_BACKEDGE='b',
    BIT_1_BACKEDGE='B',
    BIT_1_FORWARD_EDGE_AND_BIT_0='f',
    BIT_1_FORWARD_EDGE_AND_BIT_1='F'
} CHECKER_BIT;

uint8_t node_only_has_hamiltonian_edge(NODE* node) {

    if(node->graph_idx == node->graph->num_nodes-1) {
        return node->num_out_neighbours==0;
    } else {
        return node->num_out_neighbours==1 && node->out->to->graph_idx==node->graph_idx+1 && (node->num_in_neighbours==1 || node->graph_idx==0);
    }
}

// return '1', '0' or 'x' for unknown or 'm' for mute node
CHECKER_BIT watermark_check_get_bit(GRAPH* graph, unsigned long idx, uint8_t bit, uint8_t last_four_only_have_hamiltonian, uint8_t can_have_removed_backedge) {

    CONNECTION* backedge = NULL;
    // if node is mute, ignore it
    if( idx > 1 && graph_get_connection(graph->nodes[idx-2], graph->nodes[idx]) ) {
        return BIT_MUTE;
    // if node is origin of a forward edge, check the value of the next node
    // by verifying the existence of a hamiltonian edge
    } else if( graph_get_connection(graph->nodes[idx], graph->nodes[idx+2]) ) {
        return graph_get_connection(graph->nodes[idx+1], graph->nodes[idx+2]) ? BIT_1_FORWARD_EDGE_AND_BIT_0 : BIT_1_FORWARD_EDGE_AND_BIT_1;
    // if there is an odd backedge, return 1
    } else if(( backedge = graph_get_backedge(graph->nodes[idx]) ) && ((idx - backedge->to->graph_idx) & 1)) {
        return BIT_1_BACKEDGE;
    // if there is an even backedge, return 0
    } else if( backedge && !((idx - backedge->to->graph_idx) & 1) ) {
        return BIT_0_BACKEDGE;
    // if there isn't a hamiltonian edge in the next node, return 1
    } else if( !graph_get_connection(graph->nodes[idx+1], graph->nodes[idx+2]) ) {
        return BIT_1_FORWARD_EDGE_AND_BIT_1;
    // if current node is the fourth last node and the last four nodes
    // only have a hamiltonian edge, return 1
    } else if( last_four_only_have_hamiltonian && idx == graph->num_nodes-4) {
        return BIT_1_FORWARD_EDGE_AND_BIT_0;
    // if this node is the first one in a sequence of three nodes without a
    // backedge or forward edge, should represent a positive bit, and it isn't possible
    // to create a backedge for it, return 1
    } else if( can_have_removed_backedge &&
            bit &&
            node_only_has_hamiltonian_edge(graph->nodes[idx]) &&
            node_only_has_hamiltonian_edge(graph->nodes[idx+1]) &&
            node_only_has_hamiltonian_edge(graph->nodes[idx+2]) ) {
        return BIT_1_FORWARD_EDGE_AND_BIT_0;
    } else {
        return BIT_0;
    }
}

// return true if it matches
uint8_t watermark_check(GRAPH* graph, void* data, unsigned long* num_bytes) {

    //unsigned long starting_idx = 
    //unsigned long n_bits = graph->num_nodes-2;
    //uint8_t bits[n_bits];
    //bits[0] = 1;
    //unsigned long i = 1;
    num_bytes = data;

    uint8_t last_four_nodes_only_have_hamiltonian_edges = graph->num_nodes > 3 &&
        node_only_has_hamiltonian_edge(graph->nodes[graph->num_nodes-1]) &&
        node_only_has_hamiltonian_edge(graph->nodes[graph->num_nodes-2]) &&
        node_only_has_hamiltonian_edge(graph->nodes[graph->num_nodes-3]) &&
        node_only_has_hamiltonian_edge(graph->nodes[graph->num_nodes-4]);
    watermark_check_get_bit(graph, 0, 0, last_four_nodes_only_have_hamiltonian_edges, 0);
    return 1;
}

uint8_t watermark_rs_check(GRAPH* graph, void* data, unsigned long* num_bytes, unsigned long num_parity_symbols);

// return bit array, in which the values can be '1', '0' or 'x' (for unknown)
uint8_t* watermark_check_analysis(GRAPH* graph, void* data, unsigned long* num_bytes);
uint8_t* watermark_rs_check_analysis(GRAPH* graph, void* data, unsigned long* num_bytes, unsigned long num_parity_symbols);
