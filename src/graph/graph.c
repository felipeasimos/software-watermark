#include "graph/graph.h"

// create graph with N empty nodes;
GRAPH* graph_create(unsigned long num_nodes) {

    GRAPH* graph = malloc(sizeof(GRAPH));
    graph->nodes = calloc(num_nodes, sizeof(NODE*));
    graph->num_nodes = num_nodes;
    graph->num_connections = 0;

    for(unsigned long i = 0; i < graph->num_nodes; i++) graph->nodes[i] = node_empty(graph, i);

    return graph;
}

// free graph and all structures in it
void graph_free(GRAPH* graph) {

    for(unsigned long i = 0; i < graph->num_nodes; i++) node_free(graph->nodes[i]);
    free(graph);
}

// print graph
void graph_print(GRAPH* graph, void (*print_func)(NODE*, void*, unsigned long)) {

    for(unsigned long i = 0; i < graph->num_nodes; i++) {

        // print node
        node_print(graph->nodes[i], print_func);
        printf("\x1b[33m\x1b[1m->\x1b[0m");
        connection_print(graph->nodes[i]->connections, print_func);
        printf("\n");
    }
}

// add node to graph
void graph_add(GRAPH* graph) {

    graph->nodes = realloc(graph->nodes, sizeof(NODE*) * ++graph->num_nodes);
    graph->nodes[graph->num_nodes-1] = node_empty(graph, graph->num_nodes-1);
}

// delete node
void graph_delete(NODE* node) {

    GRAPH* graph = node->graph;
    
    // isolate node from the rest of the nodes
    node_isolate(node);
    // get rid of him in the array
    for(unsigned long i = node->graph_idx+1; i < graph->num_nodes; i++) graph->nodes[i] = graph->nodes[i-1];
    // update number of nodes
    graph->num_nodes--;
    // free node
    node_free(node);
}

// connect node to another
void graph_oriented_connect(NODE* from, NODE* to) {

    node_oriented_connect(from, to);
    from->graph->num_connections++;
}

// sort topologically
void graph_topological_sort(GRAPH* graph) {

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
    for(GRAPH* node = source; node; node = node->next) free(graph_get_info(node));
    stack_free(stack);
    graph_unload_all_info(source);
    return ordered_nodes;
}

// generate png image with the graph
// uses 'system' syscall
void graph_write_dot(GRAPH*, const char* filename);

// copy the graph
GRAPH* graph_copy(GRAPH*);

// serialize the graph
void* graph_serialize(GRAPH*, unsigned long* num_bytes);

// deserialize the graph
GRAPH* graph_deserialize(void*, unsigned long num_bytes);



