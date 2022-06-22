#include "dijkstra/dijkstra.h"

NODE* dijkstra_get_sink(NODE* node) {

    if(!node) return NULL;
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

unsigned long dijkstra_num_in_backedges(NODE* source) {

    unsigned long n = 0;
    for(CONNECTION* conn = source->in; conn; conn = conn->next) {
        n += ( conn->node->graph_idx > source->graph_idx );
    }
    return n;
}

unsigned long dijkstra_num_out_backedges(NODE* source) {

    unsigned long n = 0;
    for(CONNECTION* conn = source->out; conn; conn = conn->next) {
        n += ( conn->node->graph_idx < source->graph_idx );
    }
    return n;
}

// count out neighbors ignoring backedges
unsigned long dijkstra_num_out_neighbors(NODE* source) {
  return source->num_out_neighbours - dijkstra_num_out_backedges(source);
}

// count in neighbours ignoring backedges
unsigned long dijkstra_num_in_neighbors(NODE* source) {
  return source->num_in_neighbours - dijkstra_num_in_backedges(source);
}

PRIME_SUBGRAPH dijkstra_is_pcase_or_if_else_prime(NODE* source_sink) {

    PRIME_SUBGRAPH prime = { .sink = NULL, .type = INVALID};
    // check if p-case of if-else
    if( source_sink->num_out_neighbours >= 2 ) {
      // condition: direct connections all point to same node
      NODE* middle_node_sink = dijkstra_get_sink(source_sink->out->node);
      if(middle_node_sink->num_out_neighbours != 1) {
        return prime;
      }
      prime.type = source_sink->num_out_neighbours == 2 ? IF_THEN_ELSE : P_CASE;
      prime.sink = dijkstra_get_sink(middle_node_sink->out->node);
      for(CONNECTION* conn = source_sink->out; conn; conn = conn->next) {

          NODE* node = conn->node;
          NODE* node_sink = dijkstra_get_sink(node);
          if( node_sink->num_out_neighbours != 1 || dijkstra_get_sink(node_sink->out->node) != prime.sink ) {
              prime.type = INVALID;
              prime.sink = NULL;
              return prime;
          }
      }
    }
    return prime;
}

PRIME_SUBGRAPH dijkstra_is_if_prime(NODE* source_sink) {

  PRIME_SUBGRAPH prime = { .sink = NULL, .type = INVALID};
  if( source_sink->num_out_neighbours == 2 ) {

    NODE* node1 = source_sink->out->node;
    NODE* node2 = source_sink->out->next->node;
    NODE* node1_sink = dijkstra_get_sink(node1);
    NODE* node2_sink = dijkstra_get_sink(node2);

    if(graph_get_connection(node1_sink, node2) || graph_get_connection(node2_sink, node1)) {
      prime.type = IF_THEN;
      prime.sink = graph_get_connection(node1_sink, node2) ? node2_sink : node1_sink;
      return prime;
    }
  }
  return prime;
}

PRIME_SUBGRAPH dijkstra_is_sequence_prime(NODE* source, NODE* source_sink) {

  PRIME_SUBGRAPH prime = { .sink = NULL, .type = INVALID};
  if( source_sink->num_out_neighbours == 1 && dijkstra_num_out_backedges(source_sink) == 0 ) {

    NODE* node = source_sink->out->node;
    NODE* node_sink = dijkstra_get_sink(node);

    if( graph_get_backedge(node_sink) // node has backedge
      ) {

        NODE* backedge_node_sink = dijkstra_get_sink(graph_get_backedge(node_sink)->node);
        // avoid closing cycles, by only contracting a sequence with a node with a backedge
        // in this specific scenario:
        //  .-------.
        // 4 -> 5 -> 6
        // `------------> 7 -> 8 -> ...
        // where the source for sequence is 5       
        if( node_sink->num_out_neighbours == 1 && // node has only one outgoing connection (while loop)
            graph_get_connection(backedge_node_sink, source) && // backedge node connects directly to source
            backedge_node_sink->num_out_neighbours == 2
          ) {
          prime.type = SEQUENCE;
          prime.sink = node_sink;
          return prime;
        }

        // avoid closing cycles, by only contracting a sequence with a node with a backedge
        // in this specific scenario:
        //  .-------.
        // 4 -> 5 -> 6 -> 7 -> ...
        // where the source for sequence is 5
        if( node_sink->num_out_neighbours == 2 && // node has two outgoing connection (repeat loop)
            graph_get_connection(backedge_node_sink, source) && // backedge node connects directly to source
            backedge_node_sink->num_out_neighbours == 1
          ) {
          prime.type = SEQUENCE;
          prime.sink = node_sink;
          return prime;
        }
      return prime;
    }

    // must have only one in neighbour
    if( dijkstra_num_in_neighbors(node) == 1 ) {
      prime.type = SEQUENCE;
      prime.sink = node_sink;
      return prime;
    }
  }
  return prime;
}

PRIME_SUBGRAPH dijkstra_is_while_prime(NODE* source, NODE* source_sink) {

  PRIME_SUBGRAPH prime = { .sink = NULL, .type = INVALID};
  if( dijkstra_num_out_neighbors(source_sink) == 2 && dijkstra_num_in_backedges(source) ) {

    NODE* node1 = source_sink->out->node;
    NODE* node2 = source_sink->out->next->node;
    NODE* node1_sink = dijkstra_get_sink(node1);
    NODE* node2_sink = dijkstra_get_sink(node2);

    // there must be a direct connection between one of the the sink connections and source
    if(!(graph_get_connection(node1_sink, source) || graph_get_connection(node2_sink, source))) {
      return prime;
    }

    prime.type = WHILE;
    prime.sink = graph_get_connection(node1_sink, source) ? node2_sink : node1_sink;
    return prime;
  }
  return prime;
}

PRIME_SUBGRAPH dijkstra_is_repeat_prime(NODE* source, NODE* source_sink) {

  PRIME_SUBGRAPH prime = { .sink = NULL, .type = INVALID};
  if( source_sink->num_out_neighbours == 1 && dijkstra_num_in_backedges(source) > 0 ) {

    NODE* middle_node_sink = dijkstra_get_sink(source_sink->out->node);
    if( middle_node_sink->num_out_neighbours != 2 || dijkstra_num_out_backedges(middle_node_sink) != 1 ) {
      return prime;
    } 
    prime.type = REPEAT;
    prime.sink = middle_node_sink->out->node->graph_idx < middle_node_sink->graph_idx ?
      dijkstra_get_sink(middle_node_sink->out->next->node) :
      dijkstra_get_sink(middle_node_sink->out->node);
    return prime;
  }
  return prime;
}

PRIME_SUBGRAPH dijkstra_is_non_trivial_prime(NODE* source) {

    PRIME_SUBGRAPH prime = { .sink = NULL, .type = INVALID};

    NODE* source_sink = dijkstra_get_sink(source);
    if(!source_sink->num_out_neighbours) return prime;

    // 1. check if this is a p-case or if-else
    if( (prime = dijkstra_is_pcase_or_if_else_prime(source_sink)).type != INVALID ) {
      return prime;
    }

    // 2. check if this is an if-then
    if( (prime = dijkstra_is_if_prime(source_sink)).type != INVALID ) {
      return prime;
    }

    // 3. check if this is a sequence
    if( (prime = dijkstra_is_sequence_prime(source, source_sink)).type != INVALID ) {
      return prime;
    }

    // 4. check if this is a while
    if( (prime = dijkstra_is_while_prime(source, source_sink)).type != INVALID ) {
      return prime;
    }

    // 5. check if this is a repeat
    if( (prime = dijkstra_is_repeat_prime(source, source_sink)).type != INVALID ) {
      return prime;
    }

    prime.type = INVALID;
    prime.sink = NULL;
    return prime;
}

int dijkstra_check(GRAPH* graph) {

    // graphs already come in topologically sorted from
    // the decoding and encoding processses

    if(graph->num_connections >= 2 * graph->num_nodes - 1) return 0;
    if(graph->num_connections == 0) return 1;

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
            // fprintf(stderr, "idx: %lu, type: %d\n", node->graph_idx, prime.type);
            // graph_print(graph, dijkstra_print_node);
            // graph_write_dot_generic(graph, "dot.dot", "161312111412161111121", dijkstra_print_node);
            // repeat until no prime is found in this node
            while(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID) {
                node_set_info(node, prime.sink, sizeof(NODE*));
                // fprintf(stderr, "idx: %lu, type: %d\n", node->graph_idx, prime.type);
                // graph_print(graph, dijkstra_print_node);
                // graph_write_dot_generic(graph, "dot.dot", "161312111412161111121", dijkstra_print_node);
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
            codes[source_sink->out->node->graph_idx] = NULL;
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
            codes[middle_node->graph_idx] = codes[final_node->graph_idx] = NULL;
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
            codes[connect_back_node->graph_idx] = codes[final_node->graph_idx] = NULL;
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
            codes[middle_node->graph_idx] = codes[final_node->graph_idx] = NULL;
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
            // add the code of each middle node, starting from the last connection
            for(CONNECTION* conn = source_sink->out; conn; conn = conn->next) {
                dijkstra_merge_codes(codes, source, conn->node, 0);
                free(codes[conn->node->graph_idx]);
                codes[conn->node->graph_idx] = NULL;
            }
            // add final node code
            dijkstra_merge_codes(codes, source, final_node, 0);
            free(codes[final_node->graph_idx]);
            codes[final_node->graph_idx] = NULL;
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
    if(graph->num_connections == 0) {
        char* code = malloc(2);
        code[0]='1';
        code[1]='\0';
        return code;
    }

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
            fprintf(stderr, "idx: %lu, type: %d\n", node->graph_idx, prime.type);
            graph_print(graph, dijkstra_print_node);
            graph_write_dot_generic(graph, "dot.dot", NULL, dijkstra_print_node);

            // repeat until no prime is found in this node
            while(( prime = dijkstra_is_non_trivial_prime(node) ).type != INVALID) {
                dijkstra_update_code(node, prime, codes);
                node_set_info(node, prime.sink, node_get_info_len(node));
                fprintf(stderr, "idx: %lu, type: %d\n", node->graph_idx, prime.type);
                graph_print(graph, dijkstra_print_node);
                graph_write_dot_generic(graph, "dot.dot", NULL, dijkstra_print_node);
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
        NODE* sink_node = node_expand_to_p_case(source, p);
        dijkstra_code+=2;
        // get last connection from source code and start expanding from it
        for(CONNECTION* conn = source->out; conn; conn = conn->next) {
            dijkstra_code = dijkstra_generate_recursive(dijkstra_code, conn->node);
        }
        return dijkstra_generate_recursive(dijkstra_code, sink_node);
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
