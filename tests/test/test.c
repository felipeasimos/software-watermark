#include "ctdd/ctdd.h"
#include "graph/graph.h"
#include "encoder/encoder.h"
#include "decoder/decoder.h"
#include "dijkstra/dijkstra.h"

#define PRINT_K(k)\
        char str[100];\
        sprintf(str, "%hhu", k);\
        graph_write_hamiltonian_dot(graph, "dot.dot", str);

int graph_test() {

    GRAPH* graph = graph_create(8);
    ctdd_assert(!graph->num_connections);
    ctdd_assert(graph->num_nodes == 8);
    graph_add(graph);
    ctdd_assert(graph->num_nodes == 9);
    graph_delete(graph->nodes[3]);
    ctdd_assert(graph->num_nodes == 8);
    graph_insert(graph, 2);
    ctdd_assert(graph->num_nodes == 9);
    graph_delete(graph->nodes[4]);
    ctdd_assert(graph->num_nodes == 8);
    graph_insert(graph, 8);
    ctdd_assert(graph->num_nodes == 9);
    graph_delete(graph->nodes[0]);
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
    ctdd_assert( get_bit(&k, 1) == 0 );
    ctdd_assert( get_bit(&k, 2) == 1 );
    ctdd_assert( get_bit(&k, 3) == 1 );
    ctdd_assert( get_bit(&k, 4) == 0 );
    ctdd_assert( get_bit(&k, 5) == 0 );
    ctdd_assert( get_bit(&k, 6) == 1 );
    ctdd_assert( get_bit(&k, 7) == 1 );
    k = 0x1;
    ctdd_assert( get_bit(&k, 0) == 0 );
    ctdd_assert( get_bit(&k, 1) == 0 );
    ctdd_assert( get_bit(&k, 2) == 0 );
    ctdd_assert( get_bit(&k, 3) == 0 );
    ctdd_assert( get_bit(&k, 4) == 0 );
    ctdd_assert( get_bit(&k, 5) == 0 );
    ctdd_assert( get_bit(&k, 6) == 0 );
    ctdd_assert( get_bit(&k, 7) == 1 );
    return 0;
}

int watermark2014_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark2014_encode(&k, sizeof(k));
        unsigned long size;
        uint8_t* result = watermark2014_decode(graph, &size);
        ctdd_assert(size == 1);
        ctdd_assert(*result == k);
        free(result);
        graph_free(graph);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark2014_encode(&k, sizeof(k));
        unsigned long size;
        uint8_t* result = watermark2014_decode(graph, &size);
        ctdd_assert(binary_sequence_equal((uint8_t*)&k, result, sizeof(k), size));
        free(result);
        graph_free(graph);
    }
    return 0;
}

int watermark2014_rs_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark2014_rs_encode(&k, sizeof(k), 1);
        unsigned long size=1;
        uint8_t* result = watermark2014_rs_decode(graph, &size);
        ctdd_assert(size == 1);
        ctdd_assert(*result == k);
        free(result);
        graph_free(graph);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark2014_rs_encode(&k, sizeof(k), 1);
        unsigned long size=1;
        uint8_t* result = watermark2014_rs_decode(graph, &size);
        ctdd_assert(binary_sequence_equal((uint8_t*)&k, result, sizeof(k), size));
        free(result);
        graph_free(graph);
    }
    return 0;
}

int watermark2017_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark_encode(&k, sizeof(k));
        unsigned long size;
        uint8_t* result = watermark_decode(graph, &size);
        ctdd_assert(size == 1);
        ctdd_assert(*result == k);
        free(result);
        graph_free(graph);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark_encode(&k, sizeof(k));
        unsigned long size;
        uint8_t* result = watermark_decode(graph, &size);
        ctdd_assert(binary_sequence_equal((uint8_t*)&k, result, sizeof(k), size));
        free(result);
        graph_free(graph);
    }
    return 0;
}

int watermark2017_rs_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark_rs_encode(&k, sizeof(k), 2);
        unsigned long size=2;
        uint8_t* result = watermark_rs_decode(graph, &size);
        ctdd_assert(size == 1);
        ctdd_assert(*result == k);
        free(result);
        graph_free(graph);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark_rs_encode(&k, sizeof(k), 2);
        unsigned long size=2;
        uint8_t* result = watermark_rs_decode(graph, &size);
        ctdd_assert(binary_sequence_equal((uint8_t*)&k, result, sizeof(k), size));
        free(result);
        graph_free(graph);
    }
    return 0;
}

int dijkstra_recognition_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark_encode(&k, sizeof(k));
        uint8_t result = dijkstra_check(graph_copy(graph));
        if(!result) {
            PRINT_K(k);
        }
        graph_free(graph);
        ctdd_assert(result);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark_encode(&k, sizeof(k));
        ctdd_assert(dijkstra_check(graph));
    }
    return 0;
}

int dijkstra_code_test() {

    // bento 2017 - Fig. 8
    GRAPH* graph = graph_create(1);
    node_expand_to_if_then_else(graph->nodes[0]);
    node_expand_to_if_then(graph->nodes[2]);
    node_expand_to_sequence(graph->nodes[3]);
    node_expand_to_while(graph->nodes[1]);
    node_expand_to_sequence(graph->nodes[2]);
    node_expand_to_if_then_else(graph->nodes[3]);
    node_expand_to_sequence(graph->nodes[12]);
    graph_topological_sort(graph);
    char* code = dijkstra_get_code(graph);
    ctdd_assert( !strcmp(code, "161312111412161111121") );
    free(code);
    // 29
    uint8_t k = 29;
    graph = watermark_encode(&k, sizeof(k));
    code = dijkstra_get_code(graph);
    ctdd_assert( !strcmp(code, "151131151121") );
    free(code);
    // 28
    k = 28;
    graph = watermark_encode(&k, sizeof(k));
    code = dijkstra_get_code(graph);
    ctdd_assert( !strcmp(code, "1511311212121") );
    free(code);
    // bento 2017 - Fig 5
    graph = graph_create(1);
    node_expand_to_if_then_else(graph->nodes[0]);
    node_expand_to_while(graph->nodes[1]);
    node_expand_to_if_then(graph->nodes[4]);
    node_expand_to_sequence(graph->nodes[7]);
    code = dijkstra_get_code(graph);
    ctdd_assert( !strcmp(code, "1614111311121") );
    free(code);

    graph = dijkstra_generate("151131151121");
    code = dijkstra_get_code(graph);
    ctdd_assert(!strcmp(code, "151131151121"));
    free(code);

    return 0;
}

int dijkstra_watermark_code_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark_encode(&k, sizeof(k));
        char* code = dijkstra_get_code(graph);
        graph = dijkstra_generate(code);
        char* code2 = dijkstra_get_code(graph_copy(graph));
        // check that the resulting dijkstra code is the same
        ctdd_assert( !strcmp(code, code2) );
        unsigned long size;
        uint8_t* result = watermark_decode(graph, &size);
        graph_free(graph);
        ctdd_assert(*result == k);
        free(result);
        free(code);
        free(code2);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark_encode(&k, sizeof(k));
        char* code = dijkstra_get_code(graph);
        graph = dijkstra_generate(code);
        char* code2 = dijkstra_get_code(graph_copy(graph));
        // check that the resulting dijkstra code is the same
        ctdd_assert( !strcmp(code, code2) );
        unsigned long size;
        uint8_t* result = watermark_decode(graph, &size);
        graph_free(graph);
        ctdd_assert(binary_sequence_equal((uint8_t*)&k, result, sizeof(k), size));
        free(result);
        free(code);
        free(code2);
    }

    return 0;
}

int run_tests() {

    ctdd_verify(graph_test);
    ctdd_verify(get_bit_test);
    ctdd_verify(watermark2014_test);
    ctdd_verify(watermark2014_rs_test);
    ctdd_verify(watermark2017_test);
    ctdd_verify(watermark2017_rs_test);
    ctdd_verify(dijkstra_recognition_test);
    //ctdd_verify(dijkstra_code_test);
    //ctdd_verify(dijkstra_watermark_code_test);
	return 0;
}

int main() {

    srand(time(0));
	ctdd_setup_signal_handler();

	return ctdd_test(run_tests);
}
