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
    free(graph->nodes);
    free(graph);
}

// print graph
void graph_print(GRAPH* graph, void (*print_func)(FILE*, NODE*)) {

    for(unsigned long i = 0; i < graph->num_nodes; i++) {

        // print node
        node_print(graph->nodes[i], print_func);
        printf(" \x1b[33m\x1b[1m->\x1b[0m ");
        connection_print(graph->nodes[i]->out, print_func);
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

// disconnect node from another
void graph_oriented_disconnect(NODE* from, NODE* to) {

    node_oriented_disconnect(from, to);
    from->graph->num_connections--;
}

typedef struct TOPO_NODE {
    uint8_t mark;
    CONNECTION* check_next;
} TOPO_NODE;

// sort topologically
void graph_topological_sort(GRAPH* graph) {

    if(!graph->num_nodes) return;

    PSTACK* stack = pstack_create(graph->num_nodes);
    NODE** ordered_nodes = calloc(graph->num_nodes, sizeof(NODE*));

    for(unsigned long i = 0; i < graph->num_nodes; i++) {
        TOPO_NODE* topo = malloc(sizeof(TOPO_NODE));
        topo->mark = 0;
        topo->check_next = graph->nodes[i]->out;
        node_load_info(graph->nodes[i], topo, sizeof(TOPO_NODE));
    }
    unsigned long t = 0;
    pstack_push(stack, graph->nodes[0]);
    ((TOPO_NODE*)node_get_info(graph->nodes[0]))->mark=1;
    NODE* node = NULL;
    while( ( node = pstack_get(stack) ) ) {
        uint8_t has_unmarked_connection = 0;
        for(CONNECTION* conn = ((TOPO_NODE*)node_get_info(node))->check_next; conn; conn = conn->next) {

            // if unmarked, mark and insert in data structure
            if(conn->parent == conn->from && !((TOPO_NODE*)node_get_info(conn->to))->mark) {

                has_unmarked_connection=1;
                ((TOPO_NODE*)node_get_info(conn->to))->mark = 1;
                pstack_push(stack, conn->to);
                ((TOPO_NODE*)node_get_info(node))->check_next = conn->next;
                break;
            }
            ((TOPO_NODE*)node_get_info(node))->check_next = conn->next;
        }
        if(!has_unmarked_connection) {
            ordered_nodes[t++] = pstack_pop(stack);
        }
    }
    for(unsigned long i = 0; i < graph->num_nodes; i++) {

        free(node_get_info(graph->nodes[i]));
    }
    free(graph->nodes);
    graph->nodes = ordered_nodes;
    pstack_free(stack);
    graph_unload_info(graph);
}

// unload info from all nodes
void graph_unload_info(GRAPH* graph) {

    for(unsigned long i = 0; i < graph->num_nodes; i++) node_unload_info(graph->nodes[i]);
}

// unload all info from all nodes
void graph_unload_all_info(GRAPH* graph) {

    for(unsigned long i = 0; i < graph->num_nodes; i++) node_unload_all_info(graph->nodes[i]);
}

void graph_print_node_idx(FILE* f, NODE* node) {

    fprintf(f, "%lu", node->graph_idx);
}

// generate png image with the graph
// uses 'system' syscall
void graph_write_dot(GRAPH* graph, const char* filename) {

    FILE* file = NULL;
    if(( file = fopen(filename, "w") )) {

        fprintf(file, "digraph G {\n");
        for(unsigned long i = 0; i < graph->num_nodes; i++) {

            for(CONNECTION* conn = graph->nodes[i]->out; conn; conn = conn->next) {

                fprintf(file, "\t");
                node_write(graph->nodes[i], file, graph_print_node_idx);
                fprintf(file, " -> ");
                node_write(conn->to, file, graph_print_node_idx);
                fprintf(file, ";\n");
            }
        }
        fprintf(file, "}");
        fclose(file);
    }
}

// copy the graph
GRAPH* graph_copy(GRAPH* graph) {

    GRAPH* new_graph = graph_create(graph->num_nodes);

    for(unsigned long i = 0; i < graph->num_nodes; i++) {

        for(CONNECTION* conn = graph->nodes[i]->out; conn; conn = conn->next) {

            graph_oriented_connect(
                new_graph->nodes[i],
                new_graph->nodes[conn->to->graph_idx]
                    );
        }
    }
    return new_graph;
}

// serialize the graph
void* graph_serialize(GRAPH* graph, unsigned long* num_bytes) {

    // calculate total size
    // unsigned long for number of nodes + unsigned long data len for each node + unsigned long number of neighbours for each node
    *num_bytes = sizeof(unsigned long) * (1 + 2*graph->num_nodes);

    // add the data size and number of neighbours
    for(unsigned long i = 0; i < graph->num_nodes; i++) {
        *num_bytes += graph->nodes[i]->data_len + sizeof(NODE*) * graph->nodes[i]->num_out_neighbours;
    }
    void* serialized = malloc(*num_bytes);
    uint8_t* cursor = serialized;

    // get number of nodes
    memcpy(&graph->num_nodes, cursor, sizeof(unsigned long));
    cursor+=sizeof(unsigned long);

    for(unsigned long i = 0; i < graph->num_nodes; i++) {

        // read data
        memcpy(&graph->nodes[i]->data_len, cursor, sizeof(unsigned long));
        cursor+=sizeof(unsigned long);
        memcpy(&graph->nodes[i]->data, cursor, graph->nodes[i]->data_len);
        cursor+=graph->nodes[i]->data_len;

        // neighbours
        memcpy(&graph->nodes[i]->num_out_neighbours, cursor, sizeof(unsigned long));
        cursor+=sizeof(unsigned long);
        for(CONNECTION* conn = graph->nodes[i]->out; conn; conn = conn->next) {

            memcpy(&conn->to->graph_idx, cursor, sizeof(unsigned long));
            cursor+=sizeof(unsigned long);
        }
    }

    return serialized;
}

// deserialize the graph
GRAPH* graph_deserialize(uint8_t* data) {

    unsigned long num_nodes = 0;
    memcpy(data, &num_nodes, sizeof(unsigned long));
    data += sizeof(unsigned long);
    GRAPH* graph = graph_create(num_nodes);

    for(unsigned long i = 0; i < graph->num_nodes; i++) {

        // read data
        unsigned long data_len=0;
        memcpy(data, &data_len, sizeof(unsigned long));
        data+=sizeof(unsigned long);
        node_alloc(graph->nodes[i], data_len);
        memcpy(data, graph->nodes[i]->data, graph->nodes[i]->data_len);
        data+=graph->nodes[i]->data_len;

        // neighbours
        unsigned long num_neighbours=0;
        memcpy(data, &num_neighbours, sizeof(unsigned long));
        data+=sizeof(unsigned long);
        unsigned long neighbour_idx=0;
        for(unsigned long j = 0; j < num_neighbours; j++) {

            memcpy(data, &neighbour_idx, sizeof(unsigned long));
            data+=sizeof(unsigned long);
            graph_oriented_connect(graph->nodes[i], graph->nodes[j]);
        }
    }

    return graph;
}
