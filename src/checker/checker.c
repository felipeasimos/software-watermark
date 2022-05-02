#include "checker/checker.h"

uint8_t node_only_has_hamiltonian_edge(NODE* node) {

    if(node->graph_idx == node->graph->num_nodes-1) {
        return node->num_out_neighbours==0;
    } else {
        return node->num_out_neighbours==1 && node->out->node->graph_idx==node->graph_idx+1 && (node->num_in_neighbours==1 || node->graph_idx==0);
    }
}

// return '1', '0' or 'x' for unknown or 'm' for mute node
STATUS_BIT watermark_check_get_bit(GRAPH* graph, unsigned long idx, uint8_t bit, uint8_t last_four_only_have_hamiltonian, uint8_t can_have_removed_backedge) {

    CONNECTION* backedge = NULL;
    // if node is mute, ignore it
    if( idx > 1 && graph_get_connection(graph->nodes[idx-2], graph->nodes[idx]) ) {
        return BIT_MUTE;
    // if node is origin of a forward edge, check the value of the next node
    // by verifying the existence of a hamiltonian edge
    } else if( graph_get_connection(graph->nodes[idx], graph->nodes[idx+2]) ) {
        return graph_get_connection(graph->nodes[idx+1], graph->nodes[idx+2]) ? BIT_1_FORWARD_EDGE_AND_BIT_0 : BIT_1_FORWARD_EDGE_AND_BIT_1;
    // if there is an odd backedge, return 1
    } else if(( backedge = graph_get_backedge(graph->nodes[idx]) ) && ((idx - backedge->node->graph_idx) & 1)) {
        return BIT_1_BACKEDGE;
    // if there is an even backedge, return 0
    } else if( backedge && !((idx - backedge->node->graph_idx) & 1) ) {
        return BIT_0_BACKEDGE;
    // if there isn't a hamiltonian edge in the next node, return 1
    } else if( !graph_get_connection(graph->nodes[idx+1], graph->nodes[idx+2]) ) {
        return BIT_1_FORWARD_EDGE_AND_BIT_1;
    // if current node is the fourth last node, represents a 1 bit and the last four nodes
    // only have a hamiltonian edge, return 1
    } else if( last_four_only_have_hamiltonian && idx == graph->num_nodes-4 && bit) {
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
uint8_t watermark_check(GRAPH* graph, void* data, unsigned long num_bytes) {

    unsigned long total_number_of_bits = num_bytes*8;
    unsigned long starting_idx = get_first_positive_bit_index(data, num_bytes);
    unsigned long n_bits = total_number_of_bits - starting_idx;
    unsigned long max_num_nodes = graph->num_nodes - 2;

    // check if number of bits represented by the graph and number of bits in the given sequence match
    unsigned long num_forward_edges = 0;
    for(unsigned long i = 0; i < graph->num_nodes; i++) if(graph_get_forward(graph->nodes[i])) num_forward_edges++;
    if(n_bits != graph->num_nodes - 2 - num_forward_edges) return 0;

    // load UTILS_NODE struct to all nodes
    UTILS_NODE checker_nodes[graph->num_nodes];
    for(unsigned long i = 0; i < graph->num_nodes; i++) {
        checker_nodes[i].backedge_idx = ULONG_MAX;
        node_load_info(graph->nodes[i], &checker_nodes[i], sizeof(UTILS_NODE));
    }

    unsigned long i = starting_idx+1;

    STACK* odd_stack = stack_create(n_bits);
    STACK* even_stack = stack_create(n_bits);
    stack_push(odd_stack, 0);
    ((UTILS_NODE*)node_get_info(graph->nodes[0]))->backedge_idx = 0;

    unsigned long history[graph->num_nodes];
    history[0] = 0;

    uint8_t last_four_nodes_only_have_hamiltonian_edges = graph->num_nodes > 3 &&
        node_only_has_hamiltonian_edge(graph->nodes[graph->num_nodes-1]) &&
        node_only_has_hamiltonian_edge(graph->nodes[graph->num_nodes-2]) &&
        node_only_has_hamiltonian_edge(graph->nodes[graph->num_nodes-3]) &&
        node_only_has_hamiltonian_edge(graph->nodes[graph->num_nodes-4]);

    for(unsigned long graph_idx = 1; graph_idx < max_num_nodes; graph_idx++, i++) {

        uint8_t is_odd = !(graph_idx&1);
        uint8_t bit = get_bit(data, i);

        // backedge management
        STACK* possible_backedges = (is_odd && bit) || (!is_odd && !bit) ? even_stack : odd_stack;
        STACK* other_stack = possible_backedges == even_stack ? odd_stack : even_stack;

        STATUS_BIT checker_flag = watermark_check_get_bit(
                graph,
                graph_idx,
                bit,
                last_four_nodes_only_have_hamiltonian_edges,
                has_possible_backedge(possible_backedges, graph, graph_idx));
        switch(checker_flag) {
            case BIT_0:
            case BIT_1:
                if(bit != checker_flag - '0') {
                    graph_unload_info(graph);
                    stack_free(odd_stack);
                    stack_free(even_stack);
                    return 0;
                }
                break;
            case BIT_MUTE:
                i--;
                continue;
            case BIT_0_BACKEDGE:
            case BIT_1_BACKEDGE:
                if( (checker_flag == BIT_0_BACKEDGE && bit) || (checker_flag == BIT_1_BACKEDGE && !bit)) {
                    graph_unload_info(graph);
                    stack_free(odd_stack);
                    stack_free(even_stack);
                    return 0;
                }
                CONNECTION* backedge_conn = graph_get_backedge(graph->nodes[graph_idx]);
                if( !backedge_conn || (( bit && !((graph_idx - backedge_conn->node->graph_idx) & 1)) ||
                ( !bit && ((graph_idx - backedge_conn->node->graph_idx) & 1) )) ||
                ((UTILS_NODE*)node_get_info(backedge_conn->node))->backedge_idx >= possible_backedges->n) {
                    graph_unload_info(graph);
                    stack_free(odd_stack);
                    stack_free(even_stack);
                    return 0;
                }
                stack_pop_until(possible_backedges, ((UTILS_NODE*)node_get_info(backedge_conn->node))->backedge_idx);
                stack_pop_until(other_stack, history[backedge_conn->node->graph_idx]);
                continue;
            case BIT_1_FORWARD_EDGE_AND_BIT_0:
                if(!bit || ( i != total_number_of_bits-1 && get_bit(data, ++i) )) {
                    graph_unload_info(graph);
                    stack_free(odd_stack);
                    stack_free(even_stack);
                    return 0;
                }
                break;
            case BIT_1_FORWARD_EDGE_AND_BIT_1:
                if(!bit || ( i != total_number_of_bits-1 && !get_bit(data, ++i) )) {
                    graph_unload_info(graph);
                    stack_free(odd_stack);
                    stack_free(even_stack);
                    return 0;
                }
                break;
            case BIT_UNKNOWN:
                break;
        }
        // save stacks
        // odd
        if(is_odd) {
            ((UTILS_NODE*)node_get_info(graph->nodes[graph_idx]))->backedge_idx = odd_stack->n;
            stack_push(odd_stack, graph_idx);
            history[graph_idx] = even_stack->n;
        // even
        } else {
            ((UTILS_NODE*)node_get_info(graph->nodes[graph_idx]))->backedge_idx = even_stack->n;
            stack_push(even_stack, graph_idx);
            history[graph_idx] = odd_stack->n;
        }
        // if this was the origin of a forward node, we already processed the inner forward node and the
        // next node will be ignored
        if(checker_flag == BIT_1_FORWARD_EDGE_AND_BIT_0 || checker_flag == BIT_1_FORWARD_EDGE_AND_BIT_1) graph_idx+=2;
    }
    // bit sequence may be smaller than expected due to mute nodes
    graph_unload_info(graph);
    stack_free(odd_stack);
    stack_free(even_stack);
    return 1;
}
uint8_t watermark_rs_check(GRAPH* graph, void* data, unsigned long num_bytes, unsigned long num_parity_symbols) {

    unsigned long payload_n_bytes = num_bytes;
    unsigned long starting_idx = get_first_positive_bit_index(data, payload_n_bytes);

    // get correct parity numbers
    uint16_t parity[num_parity_symbols];
    memset(parity, 0x00, num_parity_symbols*2);
    rs_encode(data, num_bytes, parity, num_parity_symbols);

    unsigned long data_with_parity_n_bits = num_parity_symbols*2+payload_n_bytes;
    uint8_t data_with_parity[num_parity_symbols*2+payload_n_bytes];
    memcpy(data_with_parity, data, payload_n_bytes);
    memcpy(data_with_parity+payload_n_bytes, parity, num_parity_symbols*2);

    // decode using checker
    unsigned long total_n_bits = data_with_parity_n_bits;
    uint8_t* bits = watermark_check_analysis(graph, data_with_parity, &total_n_bits);
    graph_free_info(graph);
    unsigned long payload_n_bits = total_n_bits - num_parity_symbols*16;

    // in the payload, turn 'x' into wrong bit, and ascii numbers into numbers
    for(unsigned long i = 0; i < payload_n_bits; i++) {
        switch(bits[i]) {
            case 'x':
                bits[i] = !get_bit(data, starting_idx+i);
                break;
            case '0':
                bits[i] = 0;
                break;
            case '1':
                bits[i] = 1;
                break;
        }
    }
    // write correct parity to bit arr
    for(unsigned long i = payload_n_bits; i < total_n_bits; i++) {
        bits[i] = get_bit((uint8_t*)parity, i-payload_n_bits);
    }

    // convert bit arr into binary sequence
    unsigned long final_data_n_bytes = payload_n_bytes;
    uint8_t* final_data = get_sequence_from_bit_arr(bits, total_n_bits, &final_data_n_bytes);
    free(bits);
    // get correct rs code
    final_data = remove_rs_code(final_data, final_data_n_bytes, &num_parity_symbols);
    uint8_t result = binary_sequence_equal(data, final_data, payload_n_bytes, num_parity_symbols);
    free(final_data);
    return result;
}

// return bit array, in which the values can be '1', '0' or 'x' (for unknown)
void* watermark_check_analysis(GRAPH* graph, void* data, unsigned long* num_bytes) {

    unsigned long total_number_of_bits = (*num_bytes)*8;
    unsigned long starting_idx = get_first_positive_bit_index(data, *num_bytes);
    unsigned long n_bits = total_number_of_bits - starting_idx;
    unsigned long max_num_nodes = graph->num_nodes - 2;

    // check if number of bits represented by the graph and number of bits in the given sequence match
    unsigned long num_forward_edges = 0;
    for(unsigned long i = 0; i < graph->num_nodes; i++) if(graph_get_forward(graph->nodes[i])) num_forward_edges++;
    if(n_bits != graph->num_nodes - 2 - num_forward_edges) return 0;

    *num_bytes = n_bits;
    uint8_t* bits = malloc(n_bits);
    bits[0]='1';
    unsigned long bit_arr_idx=1;

    // load UTILS_NODE struct to all nodes
    for(unsigned long i = 0; i < graph->num_nodes; i++) {
        UTILS_NODE* utils_node = malloc(sizeof(UTILS_NODE));
        utils_node->backedge_idx = ULONG_MAX;
        node_load_info(graph->nodes[i], utils_node, sizeof(UTILS_NODE));
    }

    unsigned long i = starting_idx+1;

    STACK* odd_stack = stack_create(n_bits);
    STACK* even_stack = stack_create(n_bits);
    stack_push(odd_stack, 0);
    ((UTILS_NODE*)node_get_info(graph->nodes[0]))->backedge_idx = 0;
    ((UTILS_NODE*)node_get_info(graph->nodes[0]))->checker_bit = BIT_1;
    ((UTILS_NODE*)node_get_info(graph->nodes[0]))->bit_idx = 0;

    unsigned long history[graph->num_nodes];
    history[0] = 0;

    uint8_t last_four_nodes_only_have_hamiltonian_edges = graph->num_nodes > 3 &&
        node_only_has_hamiltonian_edge(graph->nodes[graph->num_nodes-1]) &&
        node_only_has_hamiltonian_edge(graph->nodes[graph->num_nodes-2]) &&
        node_only_has_hamiltonian_edge(graph->nodes[graph->num_nodes-3]) &&
        node_only_has_hamiltonian_edge(graph->nodes[graph->num_nodes-4]);

    for(unsigned long graph_idx = 1; graph_idx < max_num_nodes; graph_idx++, i++) {

        uint8_t is_odd = !(graph_idx&1);
        uint8_t bit = get_bit(data, i);

        // backedge management
        STACK* possible_backedges = (is_odd && bit) || (!is_odd && !bit) ? even_stack : odd_stack;
        STACK* other_stack = possible_backedges == even_stack ? odd_stack : even_stack;

        STATUS_BIT checker_flag = watermark_check_get_bit(
                graph,
                graph_idx,
                bit,
                last_four_nodes_only_have_hamiltonian_edges,
                has_possible_backedge(possible_backedges, graph, graph_idx));
        ((UTILS_NODE*)node_get_info(graph->nodes[graph_idx]))->checker_bit = checker_flag;
        ((UTILS_NODE*)node_get_info(graph->nodes[graph_idx]))->bit_idx = i;
        switch(checker_flag) {
            case BIT_0:
            case BIT_1:
                bits[bit_arr_idx++] = (bit != checker_flag - '0') ? 'x' : checker_flag;
                break;
            case BIT_MUTE:
                i--;
                continue;
            case BIT_0_BACKEDGE:
            case BIT_1_BACKEDGE:
                if( (checker_flag == BIT_0_BACKEDGE && bit) || (checker_flag == BIT_1_BACKEDGE && !bit)) {
                    bits[bit_arr_idx++] = 'x';
                    break;
                }
                CONNECTION* backedge_conn = graph_get_backedge(graph->nodes[graph_idx]);
                if( !backedge_conn || (( bit && !((graph_idx - backedge_conn->node->graph_idx) & 1)) ||
                ( !bit && ((graph_idx - backedge_conn->node->graph_idx) & 1) )) ||
                ((UTILS_NODE*)node_get_info(backedge_conn->node))->backedge_idx >= possible_backedges->n) {
                    bits[bit_arr_idx++] = 'x';
                    break;
                }
                stack_pop_until(possible_backedges, ((UTILS_NODE*)node_get_info(backedge_conn->node))->backedge_idx);
                stack_pop_until(other_stack, history[backedge_conn->node->graph_idx]);
                bits[bit_arr_idx++] = (checker_flag == BIT_0_BACKEDGE) ? '0' : '1';
                continue;
            case BIT_1_FORWARD_EDGE_AND_BIT_0:
                bits[bit_arr_idx++] = !bit ? 'x' : '1';
                if(i!=total_number_of_bits-1) {
                    bits[bit_arr_idx++] = get_bit(data, ++i) ? 'x' : '0';
                    ((UTILS_NODE*)node_get_info(graph->nodes[graph_idx+1]))->checker_bit = BIT_MUTE;
                }
                break;
            case BIT_1_FORWARD_EDGE_AND_BIT_1:
                bits[bit_arr_idx++] = !bit ? 'x' : '1';
                if(i!=total_number_of_bits-1) {
                    bits[bit_arr_idx++] = !get_bit(data, ++i) ? 'x' : '1';
                    ((UTILS_NODE*)node_get_info(graph->nodes[graph_idx+1]))->checker_bit = BIT_MUTE;
                }
                break;
            case BIT_UNKNOWN:
                break;
        }
        // save stacks
        // odd
        if(is_odd) {
            ((UTILS_NODE*)node_get_info(graph->nodes[graph_idx]))->backedge_idx = odd_stack->n;
            stack_push(odd_stack, graph_idx);
            history[graph_idx] = even_stack->n;
        // even
        } else {
            ((UTILS_NODE*)node_get_info(graph->nodes[graph_idx]))->backedge_idx = even_stack->n;
            stack_push(even_stack, graph_idx);
            history[graph_idx] = odd_stack->n;
        }
        // if this was the origin of a forward node, we already processed the inner forward node
        if(checker_flag == BIT_1_FORWARD_EDGE_AND_BIT_0 || checker_flag == BIT_1_FORWARD_EDGE_AND_BIT_1) graph_idx+=2;
    }
    // bit sequence may be smaller than expected due to mute nodes
    stack_free(odd_stack);
    stack_free(even_stack);
    return bits;
}

void* watermark_rs_check_analysis(GRAPH* graph, void* data, unsigned long* num_bytes, unsigned long num_parity_symbols) {

    unsigned long payload_n_bytes = *num_bytes;
    unsigned long data_starting_idx = get_first_positive_bit_index(data, payload_n_bytes);

    // get correct parity numbers
    uint16_t parity[num_parity_symbols];
    memset(parity, 0x00, num_parity_symbols*2);
    rs_encode(data, *num_bytes, parity, num_parity_symbols);

    unsigned long data_with_parity_n_bits = num_parity_symbols*2+payload_n_bytes;
    uint8_t data_with_parity[num_parity_symbols*2+payload_n_bytes];
    memcpy(data_with_parity, data, payload_n_bytes);
    memcpy(data_with_parity+payload_n_bytes, parity, num_parity_symbols*2);

    // decode using checker
    unsigned long total_n_bits = data_with_parity_n_bits;
    uint8_t* bits = watermark_check_analysis(graph, data_with_parity, &total_n_bits);
    unsigned long payload_n_bits = total_n_bits - num_parity_symbols*16;

    // in the payload, turn 'x' into wrong bit, and ascii numbers into numbers
    for(unsigned long i = 0; i < payload_n_bits; i++) {
        switch(bits[i]) {
            case 'x':
                bits[i] = !get_bit(data, data_starting_idx+i);
                break;
            case '0':
                bits[i] = 0;
                break;
            case '1':
                bits[i] = 1;
                break;
        }
    }
    // write correct parity to bit arr
    for(unsigned long i = payload_n_bits; i < total_n_bits; i++) {
        bits[i] = get_bit((uint8_t*)parity, i-payload_n_bits);
    }

    // convert bit arr into binary sequence
    unsigned long final_data_n_bytes = payload_n_bytes;
    uint8_t* final_data = get_sequence_from_bit_arr(bits, total_n_bits, &final_data_n_bytes);
    // get correct rs code
    final_data = remove_rs_code(final_data, final_data_n_bytes, &num_parity_symbols);

    unsigned long final_data_len = num_parity_symbols;

    // change bits that are not equal in 'final_data' and 'data' to 'x'
    // starting from the first positive bit in each sequence
    unsigned long final_data_starting_idx = get_first_positive_bit_index(final_data, final_data_len);
    unsigned long final_data_n_bits = final_data_len*8 - final_data_starting_idx;
    unsigned long data_n_bits = payload_n_bytes*8 - data_starting_idx;
    // cut bit_arr to only contain bits from the payload
    unsigned long bit_arr_n_bits = data_n_bits > final_data_n_bits ? data_n_bits : final_data_n_bits;
    bits = realloc(bits, bit_arr_n_bits);
    for(unsigned long i = 0; i < bit_arr_n_bits; i++) {
        uint8_t final_data_bit = get_bit(final_data, final_data_starting_idx+i);
        uint8_t data_bit = get_bit(data, data_starting_idx+i);
        bits[i] = data_bit == final_data_bit ? data_bit : 'x';
    }
    *num_bytes = bit_arr_n_bits;
    free(final_data);
    return bits;
}
