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

// get node, returns NULL if out of bounds
NODE* graph_get(GRAPH* graph, unsigned long i) {

    return i < graph->num_nodes ? graph->nodes[i] : NULL;
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

// insert node at index
void graph_insert(GRAPH* graph, unsigned long idx) {

    if(idx > graph->num_nodes) return;
    graph->nodes = realloc(graph->nodes, sizeof(NODE*) * ++graph->num_nodes);
    unsigned long top = graph->num_nodes - idx - 1;
    for(unsigned long i = 0; i < top; i++) {
        unsigned long current_node_idx = graph->num_nodes - i - 1;
        graph->nodes[current_node_idx] = graph->nodes[current_node_idx-1];
        graph->nodes[current_node_idx]->graph_idx++;
    }
    graph->nodes[idx] = node_empty(graph, idx);
}

// swap the index of two nodes
void graph_swap(NODE* a, NODE* b) {

    unsigned long tmp = a->graph_idx;
    a->graph_idx = b->graph_idx;
    b->graph_idx = tmp;
    a->graph->nodes[a->graph_idx] = a;
    b->graph->nodes[b->graph_idx] = b;
}

// remove all connections that this node is a part of
void graph_isolate(NODE* node) {

    while((node->out)) graph_oriented_disconnect(node, node->out->node);
    while((node->in)) graph_oriented_disconnect(node->in->node, node);
}

// delete node
void graph_delete(NODE* node) {

    GRAPH* graph = node->graph;
    
    // isolate node from the rest of the nodes
    graph_isolate(node);
    // get rid of him in the array
    for(unsigned long i = node->graph_idx+1; i < graph->num_nodes; i++) {
        graph->nodes[i-1] = graph->nodes[i];
        graph->nodes[i-1]->graph_idx--;
    }
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
// returns true if connection existed
uint8_t graph_oriented_disconnect(NODE* from, NODE* to) {

    if( node_oriented_disconnect(from, to) ) {
        from->graph->num_connections--;
        return 1;
    }
    return 0;
}

// check if nodes are connected
CONNECTION* graph_get_connection(NODE* from, NODE* to) {

    return node_get_connection(from, to);
}
// return backedge connection (that goes to a node with lower index)
// if it exists
CONNECTION* graph_get_backedge(NODE* node) {
    for(CONNECTION* conn = node->out; conn; conn = conn->next)
        if(conn->node->graph_idx < node->graph_idx) return conn;
    return NULL;
}

// return forward edge connection (that goes to a node with greater index,
// but not immediately after) if it exists
CONNECTION* graph_get_forward(NODE* node) {
    for(CONNECTION* conn = node->out; conn; conn = conn->next)
        if(conn->node->graph_idx > node->graph_idx+1) return conn;
    return NULL;
}

typedef struct TOPO_NODE {
    uint8_t mark;
    CONNECTION* check_next;
} TOPO_NODE;

// sort topologically
void graph_topological_sort(GRAPH* graph) {

    if(!graph->num_nodes) return;

    STACK* stack = stack_create(graph->num_nodes);
    NODE** ordered_nodes = calloc(graph->num_nodes, sizeof(NODE*));

    for(unsigned long i = 0; i < graph->num_nodes; i++) {
        TOPO_NODE* topo = malloc(sizeof(TOPO_NODE));
        topo->mark = 0;
        topo->check_next = graph->nodes[i]->out;
        node_load_info(graph->nodes[i], topo, sizeof(TOPO_NODE));
    }
    unsigned long t = 0;
    stack_push(stack, 0);
    ((TOPO_NODE*)node_get_info(graph->nodes[0]))->mark=1;
    unsigned long node_idx = 0;
    while( ( node_idx = stack_get(stack) ) != ULONG_MAX ) {
        uint8_t has_unmarked_connection = 0;
        for(CONNECTION* conn = ((TOPO_NODE*)node_get_info(graph->nodes[node_idx]))->check_next; conn; conn = conn->next) {

            // if unmarked, mark and insert in data structure
            if(!((TOPO_NODE*)node_get_info(conn->node))->mark) {

                has_unmarked_connection=1;
                ((TOPO_NODE*)node_get_info(conn->node))->mark = 1;
                stack_push(stack, conn->node->graph_idx);
                ((TOPO_NODE*)node_get_info(graph->nodes[node_idx]))->check_next = conn->next;
                break;
            }
            ((TOPO_NODE*)node_get_info(graph->nodes[node_idx]))->check_next = conn->next;
        }
        if(!has_unmarked_connection) {
            ordered_nodes[graph->num_nodes - t++ - 1] = graph->nodes[stack_pop(stack)];
        }
    }
    graph_free_info(graph);
    free(graph->nodes);
    graph->nodes = ordered_nodes;
    stack_free(stack);
    // update indexes
    for(unsigned long i = 0; i < graph->num_nodes; i++) graph->nodes[i]->graph_idx = i;
}

// unload info from all nodes
void graph_unload_info(GRAPH* graph) {
    for(unsigned long i = 0; i < graph->num_nodes; i++) node_unload_info(graph->nodes[i]);
}

// unload all info from all nodes
void graph_unload_all_info(GRAPH* graph) {
    for(unsigned long i = 0; i < graph->num_nodes; i++) node_unload_all_info(graph->nodes[i]);
}

// free info from all nodes
void graph_free_info(GRAPH* graph) {
    for(unsigned long i = 0; i < graph->num_nodes; i++) node_free_info(graph->nodes[i]);
}

// free all info from all nodes
void graph_free_all_info(GRAPH* graph) { 
    for(unsigned long i = 0; i < graph->num_nodes; i++) node_free_all_info(graph->nodes[i]);
}
void graph_print_node_idx(FILE* f, NODE* node) {

    fprintf(f, "%lu", node->graph_idx);
}

// label can be NULL
void graph_write_hamiltonian_dot_generic(GRAPH* graph, const char* filename, const char* label, void (*print_func)(FILE*, NODE*)) {

    FILE* file = NULL;
    if(( file = fopen(filename, "w") )) {

        fprintf(file, "digraph G {\n");
        fprintf(file, "\tgraph [");
        if(label) fprintf(file, "label=\"%s\", ", label);
        fprintf(file, "rankdir=LR, splines=polyline, layout=dot, bgcolor=\"#262626\", fontcolor=\"#FFFDB8\"];\n");
        fprintf(file, "\tedge [style=invis, weight=100, overlap=0, constraint=true];\n");
        fprintf(file, "\tnode [group=main, shape=circle, color=\"#4DA6FF\", fontcolor=\"#FFFDB8\"];\n\t");
        node_write(graph->nodes[0], file, print_func);
        fprintf(file, ":e");
        for(unsigned long i = 1; i < graph->num_nodes; i++) {
            fprintf(file, " -> ");
            node_write(graph->nodes[i], file, print_func);
            fprintf(file, ":e");
        }
        fprintf(file, ";\n\tedge [style=solid, weight=1, overlap=scale, constraint=true];\n");
        for(unsigned long i = 0; i < graph->num_nodes; i++) {

            for(CONNECTION* conn = graph->nodes[i]->out; conn; conn = conn->next) {

                fprintf(file, "\t");
                node_write(graph->nodes[i], file, print_func);
                // if backedge
                if(conn->node->graph_idx < graph->nodes[i]->graph_idx) {
                    fprintf(file, ":nw -> ");
                    node_write(conn->node, file, print_func);
                    fprintf(file, ":ne [style=dashed, color=\"#FF263C\"]");
                // if forward edge
                } else if(conn->node->graph_idx > graph->nodes[i]->graph_idx+1) {
                    fprintf(file, ":se -> ");
                    node_write(conn->node, file, print_func);
                    fprintf(file, ":sw [color=green]");
                } else {
                    fprintf(file, " -> ");
                    node_write(conn->node, file, print_func);
                    fprintf(file, "[color=\"#4DA6FF\"]");
                }
                fprintf(file, ";\n");
            }
        }
        fprintf(file, "}");
        fclose(file);
    }

}

// label can be NULL
void graph_write_hamiltonian_dot(GRAPH* graph, const char* filename, const char* label) {
    graph_write_hamiltonian_dot_generic(graph, filename, label, graph_print_node_idx);
}

// label can be NULL
void graph_write_dot_generic(GRAPH* graph, const char* filename, const char* label, void (*print_func)(FILE*, NODE*)) {

    FILE* file = NULL;
    if(( file = fopen(filename, "w") )) {

        fprintf(file, "digraph G {\n");
        if(label) fprintf(file, "\tgraph [ label=\"%s\" ]", label);
        for(unsigned long i = 0; i < graph->num_nodes; i++) {

            for(CONNECTION* conn = graph->nodes[i]->out; conn; conn = conn->next) {

                fprintf(file, "\t");
                node_write(graph->nodes[i], file, print_func);
                fprintf(file, " -> ");
                node_write(conn->node, file, print_func);
                // if this a backedge
                if(conn->node->graph_idx < i) fprintf(file, "[style=dashed, color=\"#FF263C\"]");
                fprintf(file, ";\n");
            }
            if(!graph->nodes[i]->out) node_write(graph->nodes[i], file, print_func);
        }
        fprintf(file, "}");
        fclose(file);
    }
}

// label can be NULL
void graph_write_dot(GRAPH* graph, const char* filename, const char* label) {
    graph_write_dot_generic(graph, filename, label, graph_print_node_idx);
}

// copy the graph
GRAPH* graph_copy(GRAPH* graph) {

    GRAPH* new_graph = graph_create(graph->num_nodes);

    for(unsigned long i = 0; i < new_graph->num_nodes; i++) {

        // copy data
        node_alloc(new_graph->nodes[i], graph->nodes[i]->data_len);
        memcpy(new_graph->nodes[i]->data, graph->nodes[i]->data, new_graph->nodes[i]->data_len);
        for(CONNECTION* conn = graph->nodes[i]->out; conn; conn = conn->next) {

            graph_oriented_connect(
                new_graph->nodes[i],
                new_graph->nodes[conn->node->graph_idx]
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
    memcpy(cursor, &graph->num_nodes, sizeof(unsigned long));
    cursor+=sizeof(unsigned long);

    for(unsigned long i = 0; i < graph->num_nodes; i++) {
        // read data
        memcpy(cursor, &graph->nodes[i]->data_len, sizeof(unsigned long));
        cursor+=sizeof(unsigned long);
        memcpy(cursor, &graph->nodes[i]->data, graph->nodes[i]->data_len);
        cursor+=graph->nodes[i]->data_len;

        // neighbours
        memcpy(cursor, &graph->nodes[i]->num_out_neighbours, sizeof(unsigned long));
        cursor+=sizeof(unsigned long);
        for(CONNECTION* conn = graph->nodes[i]->out; conn; conn = conn->next) {

            memcpy(cursor, &conn->node->graph_idx, sizeof(unsigned long));
            cursor+=sizeof(unsigned long);
        }
    }

    return serialized;
}

// deserialize the graph
GRAPH* graph_deserialize(uint8_t* data) {

    unsigned long num_nodes = 0;
    memcpy(&num_nodes, data, sizeof(unsigned long));
    data += sizeof(unsigned long);
    GRAPH* graph = graph_create(num_nodes);

    for(unsigned long i = 0; i < graph->num_nodes; i++) {

        // read data
        unsigned long data_len=0;
        memcpy(&data_len, data, sizeof(unsigned long));
        data+=sizeof(unsigned long);
        node_alloc(graph->nodes[i], data_len);
        memcpy(graph->nodes[i]->data, data, graph->nodes[i]->data_len);
        data+=graph->nodes[i]->data_len;

        // neighbours
        unsigned long num_neighbours=0;
        memcpy(&num_neighbours, data, sizeof(unsigned long));
        data+=sizeof(unsigned long);
        unsigned long neighbour_idx=0;
        for(unsigned long j = 0; j < num_neighbours; j++) {

            memcpy(&neighbour_idx, data, sizeof(unsigned long));
            data+=sizeof(unsigned long);
            graph_oriented_connect(graph->nodes[i], graph->nodes[j]);
        }
    }

    return graph;
}
