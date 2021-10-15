#include "dijkstra/dijkstra.h"

NODE* dijkstra_get_sink(NODE* node) {

    NODE* sink = node_get_info(node);
    return sink ? sink : node;
}

void dijkstra_print_node(FILE* f, NODE* node) {

    fprintf(f, "\"%lu", node->graph_idx);
    NODE* sink = node_get_info(node);
	if( sink != node && sink ) {
        if(node_get_data(node)) {
		    fprintf(f, "[%lu](%s)", dijkstra_get_sink(node)->graph_idx, ((char**)node_get_data(node))[node->graph_idx]);
        } else {
            fprintf(f, "[%lu]", dijkstra_get_sink(node)->graph_idx);
        }
	} else {
        if(node_get_data(node))fprintf(f, "(%s)", ((char**)node_get_data(node))[node->graph_idx]);
    }
    if(!sink) fprintf(f, "!");
    fprintf(f, "\"");
}

unsigned long dijkstra_num_in_neighbours(NODE* source) {

    unsigned long n = 0;
    NODE* sink = dijkstra_get_sink(source);
    // in this case, there is no subgraph and all in neighbours are valid
    if(sink == source) return source->num_in_neighbours;

    // in 'dijkstra_update_code', we mark nodes with backedge as having a NULL sink.
    // for a backedge to connect to this node and be marked as so, it must have been
    // properly analyzed before
    for(CONNECTION* conn_in = source->in; conn_in; conn_in = conn_in->next) {
        n += ( !!node_get_info(conn_in->node) );
    }

    return n;
}

unsigned long dijkstra_num_in_backedges(NODE* source) {

    unsigned long n = 0;
    for(CONNECTION* conn = source->in; conn; conn = conn->next) {
        n += ( conn->node->graph_idx > source->graph_idx );
    }
    return n;
}

PRIME_SUBGRAPH dijkstra_is_non_trivial_prime(NODE* source) {

    PRIME_SUBGRAPH prime = { .sink = NULL, .type = INVALID};

    NODE* source_sink = dijkstra_get_sink(source);
    if(!source_sink->num_out_neighbours) return prime;

    // sequence or repeat
    if(source_sink->num_out_neighbours == 1 && dijkstra_num_in_neighbours(source_sink->out->node) == 1) {
        NODE* dest = source_sink->out->node;
        NODE* dest_sink = dijkstra_get_sink(dest);
        // if destination has two out neighbours and one of them is source
        if(dest_sink->num_out_neighbours == 2 && graph_get_connection(dest_sink, source)) {

            NODE* sink_node = dijkstra_get_sink(dest_sink->out->node == source ? dest_sink->out->next->node : dest_sink->out->node);
            prime.type = REPEAT;
            prime.sink = sink_node;
            // set sink of node with backedge to NULL
            node_get_info_struct(dest_sink)->info = NULL;
            return prime;
        // make sure this isn't the middle node of a while
        } else if(!graph_get_connection(dest_sink, source) && source->num_in_neighbours <= 1) {

            // make sure this won't create a cycle between a forward edge and a backedge
            CONNECTION* backedge = graph_get_backedge(dest_sink);
            if(source->num_in_neighbours && backedge &&
                backedge->node->num_out_neighbours > 1 &&
                graph_get_connection(backedge->node, source) &&
                // if inside repeat
                ((
                    dijkstra_num_in_backedges(backedge->node) == 1 &&
                    dijkstra_num_in_neighbours(source) == 2 &&
                    dijkstra_get_sink(backedge->node)->num_out_neighbours == 2
                    ) || (
                    dijkstra_num_in_backedges(backedge->node) == 2 &&
                    dijkstra_num_in_neighbours(source) == 1 &&
                    dijkstra_get_sink(backedge->node)->num_out_neighbours == 2   
                  ))) {
                return prime;
            }

            prime.type = SEQUENCE;
            prime.sink = dest_sink;
            return prime;
        }
    // while, if or p-case
    } else if(source_sink->num_out_neighbours > 1 &&
        source->graph_idx < source_sink->out->node->graph_idx &&
        source->graph_idx < source_sink->out->next->node->graph_idx) {

        NODE* node1 = source_sink->out->node;
        NODE* node2 = source_sink->out->next->node;
        NODE* node1_sink = dijkstra_get_sink(node1);
        NODE* node2_sink = dijkstra_get_sink(node2);

        // if this is a if-then, one of the destinations connects back to the other one
        if(graph_get_connection(node1_sink, node2) || graph_get_connection(node2_sink, node1)) {

            uint8_t node1_connects_to_node2 = !!graph_get_connection(node1_sink, node2);
            NODE* middle_node = node1_connects_to_node2 ? node1 : node2;
            NODE* final_node = node1_connects_to_node2 ? node2 : node1;
            NODE* middle_node_sink = dijkstra_get_sink(middle_node);
            NODE* final_node_sink = dijkstra_get_sink(final_node);

            if( dijkstra_num_in_neighbours(middle_node) == 1 && middle_node_sink->num_out_neighbours == 1 &&
                dijkstra_num_in_neighbours(final_node) == 2) {

                prime.type = IF_THEN;
                prime.sink = final_node_sink;
                return prime;
            }
        // if this is a while, one of the destinations connects back to source
        } else if(graph_get_connection(node1_sink, source) || graph_get_connection(node2_sink, source)) {

            uint8_t node1_connects_back = !!graph_get_connection(node1_sink, source);
            NODE* connect_back_node = node1_connects_back ? node1 : node2;
            NODE* final_node = node1_connects_back ? node2 : node1;
            NODE* connect_back_node_sink = dijkstra_get_sink(connect_back_node);
            NODE* final_node_sink = dijkstra_get_sink(final_node);

            if( dijkstra_num_in_neighbours(connect_back_node) == 1 && connect_back_node_sink->num_out_neighbours == 1 &&
                dijkstra_num_in_neighbours(final_node) == 1) {

                // set sink of a node with backedge to NULL
                node_get_info_struct(connect_back_node_sink)->info = NULL;
                prime.type = WHILE;
                prime.sink = final_node_sink;
                return prime;
            }
        // if-then-else and p-case (switch)
        } else {
            prime.type = source_sink->num_out_neighbours == 2 ? IF_THEN_ELSE : P_CASE;
            prime.sink = NULL;
            for(CONNECTION* conn = source_sink->out; conn; conn = conn->next) {

                NODE* node = conn->node;
                NODE* node_sink = dijkstra_get_sink(node);
                if( node_sink->num_out_neighbours != 1 || dijkstra_num_in_neighbours(node) != 1 ) {
                    prime.type = INVALID;
                    prime.sink = NULL;
                    return prime;
                } else if(!prime.sink){
                    prime.sink = node_sink->out->node;
                } else if(prime.sink != node_sink->out->node) {
                    prime.type = INVALID;
                    prime.sink = NULL;
                    return prime;
                }
            }

            if(prime.sink) prime.sink = dijkstra_get_sink(prime.sink);
            return prime;
        }
    }

    prime.type = INVALID;
    prime.sink = NULL;
    return prime;
}

int dijkstra_check(GRAPH* graph) {

    // graphs already come in topologically sorted from
    // the decoding and encoding processses

    if(graph->num_connections >= 2 * graph->num_nodes - 1) return 0;

    // allocate memory to save sink node address
    for(unsigned long i = 0; i < graph->num_nodes; i++) node_load_info(graph->nodes[i], graph->nodes[i], sizeof(NODE*));

    // iterate through graph in inverse topological order
    PRIME_SUBGRAPH prime;
    unsigned long num_nodes = graph->num_nodes;
    // iterate all nodes
    for(unsigned long i = 0; i < graph->num_nodes; i++) {
        NODE* node = graph->nodes[num_nodes-1-i];
        if(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID ) {
            // contract
            node_set_info(node, prime.sink, sizeof(NODE*));
            // repeat until no prime is found in this node
            while(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID) {
                node_set_info(node, prime.sink, sizeof(NODE*));
            }
        }
    }
    uint8_t result = dijkstra_get_sink(graph->nodes[0]) == graph->nodes[graph->num_nodes - 1];
    graph_unload_info(graph);

    return result;
}

// when type == INVALID == 0, no special codes is added, the codes are just appended
void dijkstra_merge_codes(char* codes[], NODE* a, NODE* b, STATEMENT_GRAPH type) {

    // update size
    node_set_info(a, dijkstra_get_sink(a), node_get_info_len(a) + node_get_info_len(b) - !type);
    codes[a->graph_idx] = realloc(codes[a->graph_idx], node_get_info_len(a));
    if(type) {
        char str[node_get_info_len(a)];
        sprintf(str, "%d%s", type, codes[b->graph_idx]);
        strcat(codes[a->graph_idx], str);
    } else {
        strcat(codes[a->graph_idx], codes[b->graph_idx]);
    }
}

void dijkstra_update_code(NODE* source, PRIME_SUBGRAPH prime, char* codes[]) {

    NODE* source_sink = dijkstra_get_sink(source);
    switch(prime.type) {

        // add code from the only out-neighbour
        case SEQUENCE: {
            dijkstra_merge_codes(codes, source, source_sink->out->node, prime.type);
            free(codes[source_sink->out->node->graph_idx]);
            break;
        }
        // add code from if block and final node
        case IF_THEN: {
            uint8_t node1_connects_to_node2 = !!graph_get_connection(dijkstra_get_sink(source_sink->out->node), source_sink->out->next->node);
            NODE* middle_node = node1_connects_to_node2 ? source_sink->out->node : source_sink->out->next->node;
            NODE* final_node = node1_connects_to_node2 ? source_sink->out->next->node : source_sink->out->node;
            dijkstra_merge_codes(codes, source, middle_node, prime.type);
            dijkstra_merge_codes(codes, source, final_node, 0);
            free(codes[middle_node->graph_idx]);
            free(codes[final_node->graph_idx]);
            break;
        }
        // add code from while block and final node
        case WHILE: {
            uint8_t node1_connects_back = !!graph_get_connection(dijkstra_get_sink(source_sink->out->node), source);
            NODE* connect_back_node = node1_connects_back ? source_sink->out->node : source_sink->out->next->node;
            NODE* final_node = node1_connects_back ? source_sink->out->next->node : source_sink->out->node;
            dijkstra_merge_codes(codes, source, connect_back_node, prime.type);
            dijkstra_merge_codes(codes, source, final_node, 0);
            free(codes[connect_back_node->graph_idx]);
            free(codes[final_node->graph_idx]);
            break;
        }
        case REPEAT: {
            NODE* middle_node = source_sink->out->node;
            NODE* middle_node_sink = dijkstra_get_sink(middle_node);
            NODE* final_node = middle_node_sink->out->node != source ? middle_node_sink->out->node : middle_node_sink->out->next->node;
            dijkstra_merge_codes(codes, source, middle_node, prime.type);
            dijkstra_merge_codes(codes, source, final_node, 0);
            free(codes[middle_node->graph_idx]);
            free(codes[final_node->graph_idx]);
            break;
        }
        case IF_THEN_ELSE:
        case P_CASE: {
            unsigned long id_num = source_sink->num_out_neighbours + 4;
            NODE* final_node = dijkstra_get_sink(source_sink->out->node)->out->node;
            // add 'id_num' to source node code
            char id_num_str[25];
            sprintf(id_num_str, "%lu", id_num);
            node_set_info(source, source_sink, node_get_info_len(source) + strlen(id_num_str));
            codes[source->graph_idx] = realloc(codes[source->graph_idx], node_get_info_len(source));
            strcat(codes[source->graph_idx], id_num_str);
            // add the code of each middle node, following the topological order
            for(CONNECTION* conn = source_sink->out; conn; conn = conn->next) {
                dijkstra_merge_codes(codes, source, conn->node, 0);
                free(codes[conn->node->graph_idx]);
            }
            // add final node code
            dijkstra_merge_codes(codes, source, final_node, 0);
            free(codes[final_node->graph_idx]);
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

    // allocate memory to save sink node address
    // 'info_len' will actually be the size of the string kept in the static array below
    for(unsigned long i = 0; i < graph->num_nodes; i++) node_load_info(graph->nodes[i], graph->nodes[i], 2);

    char* codes[graph->num_nodes];

    // set code of every node to 1
    for(unsigned long i=0; i < graph->num_nodes; i++) {
        codes[i] = malloc(2);
        codes[i][0] = '1';
        codes[i][1] = '\0';
        node_get_info_struct(graph->nodes[i])->data = codes;
    }

    // iterate through graph in inverse topological order
    PRIME_SUBGRAPH prime;
    // iterate all nodes
    for(unsigned long i = 0; i < graph->num_nodes; i++) {

        NODE* node = graph->nodes[graph->num_nodes-1-i];

        if(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID ) {

            dijkstra_update_code(node, prime, codes);
            node_set_info(node, prime.sink, node_get_info_len(node));

            // repeat until no prime is found in this node
            while(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID) {
                dijkstra_update_code(node, prime, codes);
                node_set_info(node, prime.sink, node_get_info_len(node));
            }
        }
    }
    graph_unload_info(graph);
    // make node->data point to NULL, to avoid a double free
    for(unsigned long i = 0; i < graph->num_nodes; i++) graph->nodes[i]->data = NULL;

    return codes[0];
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
