#include "ctdd/ctdd.h"
#include "graph/graph.h"
#include "encoder/encoder.h"
#include "decoder/decoder.h"
#include "rs_api/rs.h"

int graph_test() {

    GRAPH* graph = graph_create(8);
    ctdd_assert(!graph->num_connections);
    ctdd_assert(graph->num_nodes == 8);
    graph_add(graph);
    ctdd_assert(graph->num_nodes == 9);
    graph_delete(graph->nodes[3]);
    ctdd_assert(graph->num_nodes == 8);

    // make a hamiltonian path
    graph_oriented_connect(graph->nodes[0], graph->nodes[1]);
    graph_oriented_connect(graph->nodes[1], graph->nodes[2]);
    graph_oriented_connect(graph->nodes[2], graph->nodes[3]);
    graph_oriented_connect(graph->nodes[3], graph->nodes[4]);
    graph_oriented_connect(graph->nodes[4], graph->nodes[5]);
    graph_oriented_connect(graph->nodes[5], graph->nodes[6]);
    graph_oriented_connect(graph->nodes[6], graph->nodes[7]);
    ctdd_assert(graph->num_connections == 7);
    ctdd_assert(graph->num_nodes == 8);
    graph_topological_sort(graph);
    ctdd_assert(graph->num_connections == 7);
    ctdd_assert(graph->num_nodes == 8);
    unsigned long len = 0;
    void* data = graph_serialize(graph, &len);
    GRAPH* tmp = graph_deserialize(data);
    free(data);
    ctdd_assert(tmp->num_nodes == graph->num_nodes);
    ctdd_assert(tmp->num_connections == graph->num_connections);
    graph_free(tmp);
    graph_oriented_disconnect(graph->nodes[2], graph->nodes[6]);
    graph_oriented_disconnect(graph->nodes[2], graph->nodes[6]);
    graph_delete(graph->nodes[2]);
    ctdd_assert(graph->num_nodes == 7);
    GRAPH* new_graph = graph_copy(graph);
    ctdd_assert( new_graph->num_nodes == graph->num_nodes );
    ctdd_assert( new_graph->num_connections == graph->num_connections );
    graph_free(graph);
    graph_free(new_graph);

    return 0;
}

int get_bit_test() {

    uint8_t k = 179;
    ctdd_assert( get_bit(&k, 0) == 1 );
    ctdd_assert( get_bit(&k, 1) == 1 );
    ctdd_assert( get_bit(&k, 2) == 0 );
    ctdd_assert( get_bit(&k, 3) == 1 );
    ctdd_assert( get_bit(&k, 4) == 1 );
    ctdd_assert( get_bit(&k, 5) == 0 );
    ctdd_assert( get_bit(&k, 6) == 0 );
    ctdd_assert( get_bit(&k, 7) == 1 );
    k = 0x1;
    ctdd_assert( get_bit(&k, 0) == 1 );
    ctdd_assert( get_bit(&k, 1) == 0 );
    ctdd_assert( get_bit(&k, 2) == 0 );
    ctdd_assert( get_bit(&k, 3) == 0 );
    ctdd_assert( get_bit(&k, 4) == 0 );
    ctdd_assert( get_bit(&k, 5) == 0 );
    ctdd_assert( get_bit(&k, 6) == 0 );
    ctdd_assert( get_bit(&k, 7) == 0 );
    return 0;
}

int watermark2014_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark2014_encode(&k, 1);
        graph_write_dot(graph, "dot.dot", "panquecas");
        graph_print(graph, NULL);
        unsigned long size;
        uint8_t* result = watermark2014_decode(graph, &size);
        ctdd_assert(size == 1);
        ctdd_assert(*result == k);
        free(result);
        graph_free(graph);
        invert_binary_sequence(&k, 1);
    }
    return 0;
}

int run_tests() {
    ctdd_verify(graph_test);
    ctdd_verify(get_bit_test);
    ctdd_verify(watermark2014_test);
	return 0;
}

int main() {

	ctdd_setup_signal_handler();

	return ctdd_test(run_tests);
}
