#include "dijkstra/dijkstra.h"

PRIME_SUBGRAPH dijkstra_is_non_trivial_prime(NODE* source) {

    PRIME_SUBGRAPH prime = { .sink = NULL, .type = INVALID};

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
        // this is a sequence if it isn't part of an while, a repeat or a if-then
        // make sure no cycle is created, where dest connects back to the node previous from the source
        } else if(!graph_get_connection(dest, source) && source->num_in_neighbours <= 1 ){

            // make sure it is not the sink of a while, by searching for a backedge in the node
            // it comes from
            if(source->num_in_neighbours) {

                for(CONNECTION* conn = source->in->from->in; conn; conn = conn->next) {
                    if(conn->from->graph_idx > source->in->from->graph_idx && conn->from != dest) {
                        return prime;
                    }
                }
            }

            prime.type = SEQUENCE;
            prime.sink = dest;
            return prime;
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

                prime.type = IF_THEN;
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

            prime.type = source->num_out_neighbours == 2 ? IF_THEN_ELSE : P_CASE;
            prime.sink = NULL;
            for(CONNECTION* conn = source->out; conn; conn = conn->next) {

                NODE* node = conn->to;
                if( node->num_out_neighbours != 1 || node->num_in_neighbours != 1 ) {
                    prime.type = INVALID;
                    prime.sink = NULL;
                    return prime;
                } else if(!prime.sink){
                    prime.sink = node->out->to;
                } else if(prime.sink != node->out->to) {
                    prime.type = INVALID;
                    prime.sink = NULL;
                    return prime;
                }
            }

            return prime;
        }
    }

    prime.type = INVALID;
    prime.sink = NULL;
    return prime;
}

void dijkstra_contract(NODE* source, NODE* sink, STATEMENT_GRAPH type) {

    // delete all nodes from the statement subgraph that aren't the
    // source or the sink
    switch(type) {
        case REPEAT: {
            node_free_info(source->out->to);
            graph_delete(source->out->to);
            break;
        }
        case IF_THEN:
        case WHILE: {
            NODE* node_to_delete = source->out->to != sink ? source->out->to : source->out->next->to;
            node_free_info(node_to_delete);
            graph_delete(node_to_delete);
            break;
        }
        case IF_THEN_ELSE:
        case P_CASE: {
            while(source->out) {
                node_free_info(source->out->to);
                graph_delete(source->out->to);
            }
            break;
        }
        case INVALID:
        case SEQUENCE:
        case TRIVIAL:
            break;
    }

    // move all outer neighbours from sink to source
    while(sink->out) {

        graph_oriented_connect(source, sink->out->to);
        graph_oriented_disconnect(sink, sink->out->to);
    }

    // delete sink
    node_free_info(sink);
    graph_delete(sink);
}

int dijkstra_check(GRAPH* graph) {

    // graphs already come in topologically sorted from
    // the decoding and encoding processses

    if(graph->num_connections >= 2 * graph->num_nodes - 1) return 0;

    // iterate through graph in inverse topological order
    PRIME_SUBGRAPH prime;
    unsigned long num_nodes = graph->num_nodes;
    // iterate all nodes
    for(unsigned long i = 0; i < num_nodes; i++) {
        NODE* node = graph->nodes[num_nodes-1-i];
        if(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID ) {
            dijkstra_contract(node, prime.sink, prime.type);
            // repeat until no prime is found in this node
            while(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID) {
                dijkstra_contract(node, prime.sink, prime.type);
            }
        }
    }
    uint8_t result = graph->num_nodes == 1 && !graph->nodes[0]->num_out_neighbours;
    graph_free(graph);

    return result;
}

void dijkstra_update_code(NODE* source, PRIME_SUBGRAPH prime) {

    char* new_code = NULL;
    switch(prime.type) {

        // add code from the only out-neighbour
        case SEQUENCE: {
            unsigned long new_len = node_get_info_len(source) + node_get_info_len(source->out->to);
            new_code = malloc(new_len);
            sprintf(new_code, "%s2%s", (char*)node_get_info(source), (char*)node_get_info(source->out->to));
            node_free_info(source);
            node_load_info(source, new_code, new_len);
            break;
        }
        // add code from if block and final node
        case IF_THEN: {
            uint8_t node1_connects_to_node2 = !!graph_get_connection(source->out->to, source->out->next->to);
            NODE* middle_node = node1_connects_to_node2 ? source->out->to : source->out->next->to;
            NODE* final_node = node1_connects_to_node2 ? source->out->next->to : source->out->to;

            unsigned long new_len = node_get_info_len(source) + node_get_info_len(middle_node) + node_get_info_len(final_node);
            new_code = malloc(new_len);
            sprintf(new_code, "%s3%s%s", (char*)node_get_info(source), (char*)node_get_info(middle_node), (char*)node_get_info(final_node));
            node_free_info(source);
            node_load_info(source, new_code, new_len);
            break;
        }
        // add code from while block and final node
        case WHILE: {
            uint8_t node1_connects_back = !!graph_get_connection(source->out->to, source);
            NODE* connect_back_node = node1_connects_back ? source->out->to : source->out->next->to;
            NODE* final_node = node1_connects_back ? source->out->next->to : source->out->to;

            unsigned long new_len = node_get_info_len(source) + node_get_info_len(connect_back_node) + node_get_info_len(final_node);
            new_code = malloc(new_len);
            sprintf(new_code, "%s4%s%s", (char*)node_get_info(source),
                                        (char*)node_get_info(connect_back_node),
                                        (char*)node_get_info(final_node));
            node_free_info(source);
            node_load_info(source, new_code, new_len);
            break;
        }
        case REPEAT: {
            NODE* middle_node = source->out->to;
            NODE* final_node = middle_node->out->to != source ? middle_node->out->to : middle_node->out->next->to;

            unsigned long new_len = node_get_info_len(source) + node_get_info_len(middle_node) + node_get_info_len(final_node);
            new_code = malloc(new_len);
            sprintf(new_code, "%s5%s%s", (char*)node_get_info(source),
                                        (char*)node_get_info(middle_node),
                                        (char*)node_get_info(final_node));
            node_free_info(source);
            node_load_info(source, new_code, new_len);
            break;
        }
        case IF_THEN_ELSE:
        case P_CASE: {
            unsigned long id_num = source->num_out_neighbours + 4;
            NODE* final_node = source->out->to->out->to;
            GRAPH* graph = source->graph;
            unsigned long new_len=node_get_info_len(source) + node_get_info_len(final_node);
            for(NODE* node = graph->nodes[source->graph_idx+1]; node != final_node; node = graph->nodes[node->graph_idx+1]) {
                new_len += node_get_info_len(node); 
            }
            new_code = malloc(new_len);
            sprintf(new_code, "%s%lu", (char*)node_get_info(source), id_num);
            for(NODE* node = graph->nodes[source->graph_idx+1]; node != final_node; node = graph->nodes[node->graph_idx+1]) {
                strcat(new_code, (char*)node_get_info(node));
            }
            strcat(new_code, (char*)node_get_info(final_node));
            node_free_info(source);
            node_load_info(source, new_code, new_len);
            break;
        }
        case INVALID:
        case TRIVIAL:
            break;
    }
}

char* dijkstra_get_code(GRAPH* graph) {

    // graphs already come in topologically sorted from
    // the decoding and encoding processses

    if(graph->num_connections >= 2 * graph->num_nodes - 1) return 0;

    // set code of every node to 1
    for(unsigned long i=0; i < graph->num_nodes; i++) {
        char* str = malloc(2);
        str[0] = '1';
        str[1] = '\0';
        node_load_info(graph->nodes[i], str, 2);
    }

    // iterate through graph in inverse topological order
    PRIME_SUBGRAPH prime;
    unsigned long num_nodes = graph->num_nodes;
    // iterate all nodes
    for(unsigned long i = 0; i < num_nodes; i++) {

        NODE* node = graph->nodes[num_nodes-1-i];
        if(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID ) {

            dijkstra_update_code(node, prime);
            dijkstra_contract(node, prime.sink, prime.type);

            // repeat until no prime is found in this node
            while(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID) {
                dijkstra_update_code(node, prime);
                dijkstra_contract(node, prime.sink, prime.type);
            }
        }
    }
    NODE* source = graph->nodes[0];
    char* code = malloc(node_get_info_len(source));
    strncpy(code, node_get_info(source), node_get_info_len(source));
    graph_free_all_info(graph);
    graph_free(graph);

    return code;
}

int dijkstra_is_equal(GRAPH* a, GRAPH* b) {

    char* a_code = dijkstra_get_code(a);
    char* b_code = dijkstra_get_code(b);

    uint8_t result = strlen(a_code) == strlen(b_code) && !strcmp(a_code, b_code);
    free(a_code);
    free(b_code);
    return result;
}

// return updated dijkstra_code pointer, from where the next node should be read from
char* dijkstra_generate_recursive(char* dijkstra_code, NODE* source) {

    // if first character isn't a '1', there is an error, since this function should
    // be called for every node, and every node code should begin and end with an '1'
    if(!dijkstra_code || dijkstra_code[0] != '1') return NULL;

    // check if this is just a trivial node at the end of the string
    if( dijkstra_code[1] == '\0' ) return ++dijkstra_code;

    // if this isn't a trivial node, get the type from the second character
    STATEMENT_GRAPH type = dijkstra_code[1] - '0';

    if( type >= P_CASE || type == IF_THEN_ELSE ) {
        unsigned long p = type - 4;
        node_expand_to_p_case(source, p);
        NODE* next_p_node = source->graph->nodes[source->graph_idx+2];
        NODE* current_node = source->graph->nodes[source->graph_idx+1];
        for(unsigned long i = 0; i < p; i++) {
            dijkstra_code = dijkstra_generate_recursive(dijkstra_code+2, next_p_node);
            current_node = next_p_node;
            next_p_node = next_p_node->graph->nodes[next_p_node->graph_idx+1];
        }
        return dijkstra_generate_recursive(dijkstra_code+2, next_p_node);
    }
    switch(type) {
        case TRIVIAL:
            return ++dijkstra_code;
        case SEQUENCE:
            return dijkstra_generate_recursive(dijkstra_code+2, node_expand_to_sequence(source));
        case IF_THEN: {
            NODE* sink_node = node_expand_to_if_then(source);
            NODE* middle_node = source->graph->nodes[source->graph_idx+1];
            dijkstra_code = dijkstra_generate_recursive(dijkstra_code+2, middle_node);
            return dijkstra_generate_recursive(dijkstra_code, sink_node);
        }
        case WHILE: {
            NODE* sink_node = node_expand_to_while(source);
            NODE* connect_back_node = source->graph->nodes[source->graph_idx+1];
            dijkstra_code = dijkstra_generate_recursive(dijkstra_code+2, connect_back_node);
            return dijkstra_generate_recursive(dijkstra_code, sink_node);
        }
        case REPEAT: {
            NODE* sink_node = node_expand_to_repeat(source);
            NODE* middle_node = source->graph->nodes[source->graph_idx+1];
            dijkstra_code = dijkstra_generate_recursive(dijkstra_code+2, middle_node);
            return dijkstra_generate_recursive(dijkstra_code, sink_node);
        }
        case IF_THEN_ELSE:
        case P_CASE:
        case INVALID:
        default:
            return NULL;
    }
}

// generate graph from dijkstra code
GRAPH* dijkstra_generate(char* dijkstra_code) {

    GRAPH* graph = graph_create(1);
    if(!dijkstra_generate_recursive(dijkstra_code, graph->nodes[0])) {
        graph_free(graph);
        return NULL;
    }

    return graph;
}
