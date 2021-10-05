#include "dijkstra/dijkstra.h"

typedef enum {
    SEQUENCE,
    REPEAT,
    IF,
    WHILE,
    IF_ELSE,
    P_CASE,
    INVALID
} STATEMENT_GRAPH;

typedef struct PRIME_SUBGRAPH {

    NODE* source;
    NODE* sink;
    STATEMENT_GRAPH type;
} PRIME_SUBGRAPH;

PRIME_SUBGRAPH dijkstra_is_non_trivial_prime(NODE* source) {

    PRIME_SUBGRAPH prime = { .source = NULL, .sink = NULL, .type = INVALID};

    if(!source->num_out_neighbours) return prime;

    // sequence or repeat
    if(source->num_out_neighbours == 1 && source->out->to->num_in_neighbours == 1) {
        NODE* dest = source->out->to;
        // if destination has two out neighbours and one of them
        // is source
        if(dest->num_out_neighbours == 2 && graph_get_connection(dest, source)) {

            prime.type = REPEAT;
            prime.sink = dest->out->to == source ? dest->out->next->to : dest->out->to;
            return prime;
        // sequence if this doesn't create a cycle where this node has backedge to a 
        // node that has a forward edge to it (which is different from a sequence edge)
        } else if(!graph_get_connection(dest, source)){
            CONNECTION* backedge_conn = graph_get_backedge(dest);
            if( !( backedge_conn && graph_get_connection(backedge_conn->to, source) ) ) {

                prime.type = SEQUENCE;
                prime.sink = dest;
                return prime;
            }
        }
    // while, if or p-case
    } else if(source->num_out_neighbours > 1 &&
        source->graph_idx < source->out->to->graph_idx && source->graph_idx < source->out->next->to->graph_idx) {

        NODE* node1 = source->out->to;
        NODE* node2 = source->out->next->to;

        // if this is a if-then, one of the destinations connects back to the other one
        if(graph_get_connection(node1, node2) || graph_get_connection(node2, node1)) {

            uint8_t node1_connects_to_node2 = !!graph_get_connection(node1, node2);
            NODE* middle_node = node1_connects_to_node2 ? node1 : node2;
            NODE* final_node = node1_connects_to_node2 ? node2 : node1;

            if( middle_node->num_in_neighbours == 1 && middle_node->num_out_neighbours == 1 &&
                final_node->num_in_neighbours == 2) {

                prime.type = IF;
                prime.sink = final_node;
                return prime;
            }
        // if this is a while, one of the destinations connects back to source
        } else if(graph_get_connection(node1, source) || graph_get_connection(node2, source)) {

            uint8_t node1_connects_back = !!graph_get_connection(node1, source);
            NODE* connect_back_node = node1_connects_back ? node1 : node2;
            NODE* final_node = node1_connects_back ? node2 : node1;

            if( connect_back_node->num_in_neighbours == 1 && connect_back_node->num_out_neighbours == 1 &&
                final_node->num_in_neighbours == 1) {

                prime.type = WHILE;
                prime.sink = final_node;
                return prime;
            }
        // if-then-else and p-case (switch)
        } else {

            NODE* sink = NULL;
            for(CONNECTION* conn = source->out; conn; conn = conn->next) {

                NODE* node = conn->to;
                if( node->num_out_neighbours != 1 || node->num_in_neighbours != 1 ) {
                    prime.type = INVALID;
                    prime.sink = NULL;
                    return prime;
                } else if(!sink){
                    sink = node->out->to;
                } else if(sink != node->out->to) {
                    prime.type = INVALID;
                    prime.sink = NULL;
                    return prime;
                }
            }
        }
    }

    prime.type = INVALID;
    prime.sink = NULL;
    return prime;
}

void dijkstra_contract(NODE* source, NODE* sink) {

    // delete all nodes between source and sin
    while(source->graph_idx+1 != sink->graph_idx) {
        graph_delete(source->graph->nodes[source->graph_idx+1]);
    }

    // move all outer neighbours from sink to source
    while(sink->out) {

        graph_oriented_connect(source, sink->out->to);
        graph_oriented_disconnect(sink, sink->out->to);
    }

    // delete sink
    graph_delete(sink);

    // delete auto-references
    while(graph_get_connection(source, source)) {
        graph_oriented_disconnect(source, source);
    }
}

int watermark_is_dijkstra(GRAPH* watermark) {

    // graphs already come in topologically sorted from
    // the decoding and encoding processses

    if(watermark->num_connections >= 2 * watermark->num_nodes - 1) return 0;

    // iterate through graph in inverse topological order
    PRIME_SUBGRAPH prime;
    unsigned long num_nodes = watermark->num_nodes;
    // iterate all nodes
    for(unsigned long i = 0; i < num_nodes; i++) {

        NODE* node = watermark->nodes[num_nodes-1-i];
        if(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID ) {

            dijkstra_contract(node, prime.sink);
            // repeat until no prime is found in this node
            while(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID) {
                dijkstra_contract(node, prime.sink);
            }
        }
    }
    uint8_t result = watermark->num_nodes == 1;
    graph_free(watermark);

    return result;
}

void dijkstra_update_code(NODE* source, PRIME_SUBGRAPH prime) {

    char* new_code = NULL;
    switch(prime.type) {

        // add code from the only out-neighbour
        case SEQUENCE: {
            unsigned long new_len = node_get_data_len(source) + node_get_data_len(source->out->to);
            new_code = malloc(new_len);
            sprintf(new_code, "%s2%s", (char*)node_get_data(source), (char*)node_get_data(source->out->to));
            node_set_data(source, new_code, new_len);
            break;
        }
        // add code from if block and final node
        case IF: {
            uint8_t node1_connects_to_node2 = !!graph_get_connection(source->out->to, source->out->next->to);
            NODE* middle_node = node1_connects_to_node2 ? source->out->to : source->out->next->to;
            NODE* final_node = node1_connects_to_node2 ? source->out->next->to : source->out->to;

            unsigned long new_len = node_get_data_len(source) + node_get_data_len(middle_node) + node_get_data_len(final_node);
            new_code = malloc(new_len);
            sprintf(new_code, "%s3%s%s", (char*)source->data, (char*)middle_node->data, (char*)final_node->data);
            node_set_data(source, new_code, new_len);
            break;
        }
        // add code from while block and final node
        case WHILE: {
            uint8_t node1_connects_back = !!graph_get_connection(source->out->to, source);
            NODE* connect_back_node = node1_connects_back ? source->out->to : source->out->next->to;
            NODE* final_node = node1_connects_back ? source->out->next->to : source->out->to;

            unsigned long new_len = node_get_data_len(source) + node_get_data_len(connect_back_node) + node_get_data_len(final_node);
            new_code = malloc(new_len);
            sprintf(new_code, "%s4%s%s", (char*)node_get_data(source),
                                        (char*)node_get_data(connect_back_node),
                                        (char*)node_get_data(final_node));
            node_set_data(source, new_code, new_len);
            break;
        }
        case REPEAT: {
            NODE* middle_node = source->out->to;
            NODE* final_node = middle_node->out->to != source ? middle_node->out->to : middle_node->out->next->to;

            unsigned long new_len = node_get_data_len(source) + node_get_data_len(middle_node) + node_get_data_len(final_node);
            new_code = malloc(new_len);
            sprintf(new_code, "%s5%s%s", (char*)node_get_data(source),
                                        (char*)node_get_data(middle_node),
                                        (char*)node_get_data(final_node));
            node_set_data(source, new_code, new_len);
            break;
        }
        case IF_ELSE:
        case P_CASE: {
            unsigned long id_num = source->num_out_neighbours + 4;
            NODE* final_node = source->out->to->out->to;
            GRAPH* graph = source->graph;
            unsigned long new_len=node_get_data_len(source);
            for(NODE* node = graph->nodes[source->graph_idx+1]; node != final_node; node = graph->nodes[node->graph_idx+1]) {
                new_len += node_get_data_len(node); 
            }
            new_code = malloc(new_len);
            sprintf(new_code, "%s%lu", (char*)node_get_data(source), id_num);
            for(NODE* node = graph->nodes[source->graph_idx+1]; node != final_node; node = graph->nodes[node->graph_idx+1]) {
                strcat(new_code, (char*)node_get_data(node));
            }
            node_set_data(source, new_code, new_len);
            break;
        }
        case INVALID: {
        }
    }
    free(new_code);
}

char* watermark_dijkstra_code(GRAPH* watermark) {

    // graphs already come in topologically sorted from
    // the decoding and encoding processses

    if(watermark->num_connections >= 2 * watermark->num_nodes - 1) return 0;

    // set code of every node to 1
    char str[2] = "1";
    for(unsigned long i=0; i < watermark->num_nodes; i++) {
        node_set_data(watermark->nodes[i], str, sizeof(str)); 
    }

    // iterate through graph in inverse topological order
    PRIME_SUBGRAPH prime;
    unsigned long num_nodes = watermark->num_nodes;
    // iterate all nodes
    for(unsigned long i = 0; i < num_nodes; i++) {

        NODE* node = watermark->nodes[num_nodes-1-i];
        if(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID ) {

            dijkstra_update_code(node, prime);
            dijkstra_contract(node, prime.sink);
            // repeat until no prime is found in this node
            while(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID) {
                dijkstra_update_code(node, prime);
                dijkstra_contract(node, prime.sink);
            }
        }
    }
    NODE* source = watermark->nodes[0];
    char* code = malloc(node_get_data_len(source));
    strncpy(code, node_get_data(source), node_get_data_len(source));
    graph_free(watermark);

    return code;
}

int watermark_dijkstra_equal(GRAPH* a, GRAPH* b) {

    char* a_code = watermark_dijkstra_code(a);
    char* b_code = watermark_dijkstra_code(b);

    uint8_t result = strlen(a_code) == strlen(b_code) && !strcmp(a_code, b_code);
    free(a_code);
    free(b_code);
    return result;
}
