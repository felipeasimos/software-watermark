#include "dijkstra/dijkstra.h"

typedef struct TOPO_NODE {
    uint8_t mark;
    CONNECTION* check_next;
} TOPO_NODE;

typedef struct STACK {

    GRAPH** nodes;
    unsigned long n;
} STACK;

STACK* stack_create(unsigned long max) {

    STACK* stack = malloc(sizeof(STACK));
    stack->n = 0;
    stack->nodes = calloc(max, sizeof(GRAPH*));
    return stack;
}

void stack_free(STACK* stack) {

    free(stack->nodes);
    free(stack);
}

void stack_push(STACK* stack, GRAPH* node) {

    stack->nodes[stack->n++] = node;
}

GRAPH* stack_pop(STACK* stack) {

    return stack->n ? stack->nodes[--stack->n] : NULL;
}

GRAPH* stack_get(STACK* stack) {

    return stack->n ? stack->nodes[stack->n-1] : NULL;
}

GRAPH** dijkstra_topological_sort_ignoring_cycle_edges(GRAPH* source, unsigned long n_nodes) {

    STACK* stack = stack_create(n_nodes);
    GRAPH** ordered_nodes = calloc(n_nodes, sizeof(GRAPH*));
    for(GRAPH* node = source; node; node = node->next) {

        TOPO_NODE* topo = malloc(sizeof(TOPO_NODE));
        topo->mark = 0;
        topo->check_next = node->connections;
        graph_load_info(node, topo, sizeof(TOPO_NODE));
    }
    unsigned long t = 0;
    stack_push(stack, source);
    ((TOPO_NODE*)graph_get_info(source))->mark=1;
    GRAPH* node = NULL;
    while( ( node = stack_get(stack) ) ) {
        uint8_t has_unmarked_connection = 0;
        for(CONNECTION* conn = ((TOPO_NODE*)graph_get_info(node))->check_next; conn; conn = conn->next) {

            // if unmarked, mark and insert in data structure
            if(!((TOPO_NODE*)graph_get_info(conn->node))->mark) {

                has_unmarked_connection=1;
                ((TOPO_NODE*)graph_get_info(conn->node))->mark = 1;
                stack_push(stack, conn->node);
                ((TOPO_NODE*)graph_get_info(node))->check_next = conn->next;
                break;
            }
            ((TOPO_NODE*)graph_get_info(node))->check_next = conn->next;
        }
        if(!has_unmarked_connection) {
            ordered_nodes[t++] = stack_pop(stack);
        }
    }
    stack_free(stack);
    graph_unload_all_info(source);
    return ordered_nodes;
}

// aka is closed and isomorphic to non-trivial statement graph
GRAPH* dijkstra_is_prime_subgraph(GRAPH* source) {

    // if source has only one connection
    if( source->connections && !source->connections->next ) {

        /* repeat */
        // if the connected node connects back to the source, this
        // can only be a a repeat block or not valid
        if( connection_search_node(source->connections->node->connections, source) ) {

            printf("repeat\n");
            graph_print(source, NULL);
            // if the connected node connects to at least three nodes, this is not valid
            return source->connections->node->connections->next->next ?
                NULL :
                connection_search_different_node(source->connections->node->connections, &source, 1)->node;
        /* repeat */
        /* sequence */
        // if it doesn't connect back to source
        } else {
            printf("sequence\n");
            graph_print(source, NULL);
            return source->connections->node;
        }
        /* sequence */
    // if it only has two connections
    } else if( source->connections->next && !source->connections->next->next ) {

        /* while */
        GRAPH* node1 = source->connections->node;
        GRAPH* node2 = source->connections->next->node;

        CONNECTION* node1_connects_to_source = connection_search_node(node1->connections, source);
        CONNECTION* node2_connects_to_source = connection_search_node(node2->connections, source);

        // both of them can't connect at the same time (no statement graph like this)
        if( node1_connects_to_source && node2_connects_to_source ) {
            return NULL;
        // if one connects to the source, it must ONLY connect to the source
        } else if( node1_connects_to_source && node1->connections->next ) {
            return NULL;
        } else if( node2_connects_to_source && node2->connections->next ) {
            return NULL;
        }
        // if only one of them connects to the source, this is a
        // while
        if( node1_connects_to_source || node2_connects_to_source ) {

            return node1_connects_to_source ? node2 : node1;
        }

        // check for recursive while (sink of one node must connect back)
        GRAPH* sink1 = dijkstra_is_prime_subgraph(node1);
        GRAPH* sink2 = dijkstra_is_prime_subgraph(node2);

        CONNECTION* sink1_connects_to_source = sink1 ? connection_search_node(sink1->connections, source) : NULL;
        CONNECTION* sink2_connects_to_source = sink2 ? connection_search_node(sink2->connections, source) : NULL;

         // both of them can't connect at the same time (no statement graph like this)
        if( sink1_connects_to_source && sink2_connects_to_source ) {
            return NULL;
        // if one connects to the source, it must ONLY connect to the source
        } else if( sink1_connects_to_source && sink1->connections->next ) {
            return NULL;
        } else if( sink2_connects_to_source && sink2->connections->next ) {
            return NULL;
        }
        // if only one of them connects to the source, this is a
        // while
        if( sink1_connects_to_source || sink2_connects_to_source ) {

            return sink1_connects_to_source ? sink2 : sink1;
        }
        /* while */

        /* if */
        CONNECTION* node1_connects_to_node2 = connection_search_node(node1->connections, node2);
        CONNECTION* node2_connects_to_node1 = connection_search_node(node2->connections, node1);
        // if they both connect to one another, this is not
        // an statement graph
        if( node1_connects_to_node2 && node2_connects_to_source ) {
            return NULL;
        } else if( node1_connects_to_node2 && node1->connections->next ) {
            return NULL;
        } else if( node2_connects_to_node1 && node2->connections->next ) {
            return NULL;
        }

        // if only one of them connects to the source, this is a if
        if( node2_connects_to_node1 || node1_connects_to_node2 ) {
            return node2_connects_to_node1 ? node1 : node2;
        }
        CONNECTION* sink1_connects_to_sink2 = connection_search_node(sink1->connections, sink2);
        CONNECTION* sink2_connects_to_sink1 = connection_search_node(sink2->connections, sink1);
        // if they both connect to one another, this is not
        // an statement graph
        if( sink1_connects_to_sink2 && sink2_connects_to_source ) {
            return NULL;
        } else if( sink1_connects_to_sink2 && sink1->connections->next ) {
            return NULL;
        } else if( sink2_connects_to_sink1 && sink2->connections->next ) {
            return NULL;
        }

        // if only one of them connects to the source, this is a if
        if( sink2_connects_to_sink1 || sink1_connects_to_sink2 ) {
            return sink2_connects_to_sink1 ? sink1 : sink2;
        }
        /* if */       
    } else {

        /// the only alternative left is for this to be a p-case or if-else
        /// so let's just check that all the nodes that the sources connects
        /// to go only to the same node
        GRAPH* sink = NULL; 
        for(CONNECTION* conn = source->connections; conn; conn = conn->next) {

            GRAPH* node = conn->node;
            // if 'node' doesn't have only one connection, this is not a valid
            // statement graph
            if( node->connections->next || !node->connections ) return NULL;

            if( sink ) {
                if( node->connections->node != sink ) return NULL;
            } else {
                sink = node->connections->node;
            }

            GRAPH* sub_sink = dijkstra_is_prime_subgraph(conn->node);
            // if 'sub_sink' doesn't have only one connection, this is not a valid
            // statement graph
            if( sub_sink->connections->next || !sub_sink->connections ) return NULL;

            if( sink ) {
                if( sub_sink->connections->node != sink ) return NULL;
            } else {
                sink = sub_sink->connections->node;
            }
        }
        return sink;
    }
    
    return NULL;
}

unsigned long dijkstra_contract(GRAPH** ordered_nodes, unsigned long n, unsigned long source_idx, GRAPH* sink) {
    for(unsigned long i = 0; i < n; i++) printf("node[%lu]: %p, data: %p\n", i, (void*)ordered_nodes[i], (void*)ordered_nodes[i]->data);
    // delete all nodes between source and sink
    unsigned long sink_idx=0;
    printf("source_idx: %lu\n", source_idx);

    for(unsigned long i = n+source_idx+1; i < n; i++) {

        printf("node: %p\n", (void*)ordered_nodes[i]);
        if(ordered_nodes[n-i-1] == sink) {
            sink_idx = n-i-1;
            printf("sink idx found: %lu\n", sink_idx);
            break;
        }
        graph_delete(ordered_nodes[i]);
    }

    // move all connections going to the source to the sink
    for(GRAPH* node = ordered_nodes[source_idx]; node; node = node->next) {

        CONNECTION* conn = connection_search_node(node->connections, ordered_nodes[source_idx]);
        if(conn) {
            conn->node = sink;
        }
    }
    return sink_idx;
}

unsigned long dijkstra_get_first_node_in_graph(GRAPH** nodes, unsigned long idx, unsigned long n, GRAPH* graph) {

    for(GRAPH* node = graph; node; node = node->next) {

        for(unsigned long i = idx; i < n; i++) {

            if(nodes[i] == node) return i;
        }
    }
    return ULONG_MAX;
}

int watermark_is_dijkstra(GRAPH* source) {

    unsigned long n = watermark_num_nodes(source);
    unsigned long m = watermark_num_edges(source);

    if( m > 2*n-1 ) return 0;

    GRAPH** ordered_nodes = dijkstra_topological_sort_ignoring_cycle_edges(source, n);
    printf("n=%lu\n", n);
    GRAPH* sink = NULL;
    for(unsigned long i = 0; i < n; i++) {
        //i = n-dijkstra_get_first_node_in_graph(ordered_nodes, n-i-1, n, source)-1;
        source = ordered_nodes[n-i-1];
        // check if graph is now trivial
        if( !source->connections ) {
            graph_free(source);
            free(ordered_nodes);
            return 1;
        } else if(( sink = dijkstra_is_prime_subgraph(source) )) {
            i = n-dijkstra_contract(ordered_nodes, n, n-i-1, sink)-2;
            printf("i=%lu\n", i+1);
        } else {
            printf("no prime subgraph detected for idx: %lu\n", n-i-1);
        }
        scanf("%p", (void**)&source);
    }
    graph_free(source);
    free(ordered_nodes);
    return 0;
}

unsigned long* watermark_dijkstra_code(GRAPH* source, unsigned long* size) {

    source = (GRAPH*)size;
    return NULL;
}

int watermark_dijkstra_equal(GRAPH* a, GRAPH* b) {

    unsigned long a_size;
    unsigned long* a_code = watermark_dijkstra_code(a, &a_size);

    unsigned long b_size;
    unsigned long* b_code = watermark_dijkstra_code(b, &b_size);

    int res = ( a_size == b_size && !memcmp(a_code, b_code, b_size) );
    free(a_code);
    free(b_code);
    return res;
}
