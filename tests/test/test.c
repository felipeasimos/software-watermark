#include "ctdd/ctdd.h"
#include "graph/graph.h"
#include "encoder/encoder.h"
#include "decoder/decoder.h"
#include "dijkstra/dijkstra.h"
#include "checker/checker.h"
#include "sequence_alignment/sequence_alignment.h"
#include "rs_api/rslib.h"
#include <limits.h>

#define show_bits(bits,len) fprintf(stderr, "%s:%d:" #bits ": ", __FILE__, __LINE__);\
  for(unsigned long i = 0; i < len; i++) {\
    fprintf(stderr, "%hhu", get_bit(bits, i));\
  }\
  fprintf(stderr, "\n");

#define PRINT_K(k)\
        char str[100];\
        sprintf(str, "%hhu", k);\
        graph_write_hamiltonian_dot(graph, "dot.dot", str);

unsigned long invert_unsigned_long(unsigned long n) {

    invert_byte_sequence((uint8_t*)&n, sizeof(n));
    return n;
}
void get_key_from_k(unsigned long** k, unsigned long symsize, unsigned long num_data_symbols) {
  // for(unsigned long i = 0; i < sizeof(*k); i++) {
  //   invert_binary_sequence(&((uint8_t*)k)[i], 1);
  // }
  **k = invert_unsigned_long(**k);
  // show_bits((void*)*k, sizeof(**k) * 8 );
  unsigned long total_n_bits = sizeof(**k) * 8;
  unsigned long data_bits = num_data_symbols * symsize;
  unsigned long left_zeros = get_number_of_left_zeros((void*)*k, sizeof(**k));
  long zeros_in_symbol = ( data_bits + left_zeros ) - total_n_bits;
  unsigned long num_bytes = sizeof(*k);
  // fprintf(stderr, "zeros_in_symbol: %ld, left_zeros: %lu, total_n_bits: %lu, data_bits: %lu, k: %lu\n", zeros_in_symbol, left_zeros, total_n_bits, data_bits, **k);
  remove_left_zeros((void*)*k, &num_bytes);
  // show_bits((void*)*k, data_bits );
  add_left_zeros((uint8_t**)k, &num_bytes, zeros_in_symbol);
  // show_bits((void*)*k, data_bits );
}

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
    uint8_t arr[]={(uint8_t)179, (uint8_t)1};
    ctdd_assert( get_bit((uint8_t*)&arr, 0) == 1 );
    ctdd_assert( get_bit((uint8_t*)&arr, 1) == 0 );
    ctdd_assert( get_bit((uint8_t*)&arr, 2) == 1 );
ctdd_assert( get_bit((uint8_t*)&arr, 3) == 1 );
    ctdd_assert( get_bit((uint8_t*)&arr, 4) == 0 );
    ctdd_assert( get_bit((uint8_t*)&arr, 5) == 0 );
    ctdd_assert( get_bit((uint8_t*)&arr, 6) == 1 );
    ctdd_assert( get_bit((uint8_t*)&arr, 7) == 1 );
    ctdd_assert( get_bit((uint8_t*)&arr, 8) == 0 );
    ctdd_assert( get_bit((uint8_t*)&arr, 9) == 0 );
    ctdd_assert( get_bit((uint8_t*)&arr, 10) == 0 );
    ctdd_assert( get_bit((uint8_t*)&arr, 11) == 0 );
    ctdd_assert( get_bit((uint8_t*)&arr, 12) == 0 );
    ctdd_assert( get_bit((uint8_t*)&arr, 13) == 0 );
    ctdd_assert( get_bit((uint8_t*)&arr, 14) == 0 );
    ctdd_assert( get_bit((uint8_t*)&arr, 15) == 1 );
    return 0;
}

int rs_test(void) {
  { 
    // uint8_t data[] = { 6, 1, 3 };// 0b110_001_011 -> 100_101 (4, 5);
    //
    // unsigned long num_parity_symbols = 2;
    // uint16_t parity[num_parity_symbols];
    // memset(parity, 0x00, num_parity_symbols * sizeof(uint16_t));
    //
    // rs_encode(data, sizeof(data)/sizeof(data[0]), parity, num_parity_symbols, 3);
    //
    // ctdd_assert( parity[0] == 4 );
    // ctdd_assert( parity[1] == 5 );
    //
    // // noise
    // data[1] = 5;
    //
    // rs_decode(data, sizeof(data)/sizeof(data[0]), parity, num_parity_symbols, 3);
    //
    // ctdd_assert( data[0] == 6 );
    // ctdd_assert( data[1] == 1 );
    // ctdd_assert( data[2] == 3 );
  }
  // uint8_t data[] = { 1, 0 };
  // unsigned long num_parity_symbols = 1;
  // uint16_t parity[num_parity_symbols];
  // memset(parity, 0x00, num_parity_symbols * sizeof(uint16_t));
  // 
  // void* rs = init_rs(3, 0x3, 1, 1, num_parity_symbols);
  // parity[0] = 0x01;
  // decode_rs8(rs, data, parity, 2, NULL, 0, NULL, 0, NULL);
  //
  // ctdd_assert( data[0] == 1 );
  // ctdd_assert( data[1] == 0 );
  //
  // data[0] = 1;
  // data[1] = 2;
  // memset(parity, 0x00, sizeof(uint16_t) * num_parity_symbols);
  // encode_rs8(rs, data, 2, parity, 0x00);
  // ctdd_assert( parity[0] );
  // free_rs(rs);
  return 0;
}

int merge_unmerge_test() {

  uint8_t data[] = { 181, 1 };

  uint8_t* res = NULL;
  unsigned long num_symbols = 3;
  unmerge_arr(&data, num_symbols, 1, 3, (void**)&res);
  
  ctdd_assert( num_symbols == 3 );
  ctdd_assert( res[0] == 5 );
  ctdd_assert( res[1] == 5 );
  ctdd_assert( res[2] = 3 );
  free(res);

  for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {
    unsigned long symsize = 3; //(rand() % 6) + 1;
    num_symbols = (rand() % (((sizeof(k) * 8) / symsize)-1) + 1);
    unsigned long num_bits = num_symbols * symsize;
    unsigned long num_bytes = num_bits / 8 + !!(num_bits % 8);
    res = NULL;
    unmerge_arr(&k, num_symbols, 1, symsize, (void**)&res);

    ctdd_assert( res );

    merge_arr(res, &num_symbols, 1, symsize);
    ctdd_assert( num_symbols == num_bytes );

    for(unsigned long i = 0; i < num_bits; i++) {
      ctdd_assert( get_bit((void*)&k, i) == get_bit((void*)res, i) );
    }
    free(res);
  }

  return 0;
}

void last_n_to_zero(uint8_t* arr, unsigned long size, unsigned long num_zeros) {
  unsigned long n_bits = size * 8;
  for(unsigned long i = 0; i < num_zeros; i++) {
    unsigned long idx = n_bits - i - 1;
    set_bit(arr, idx, 0);
  }
}

int append_remove_rs_code_test() {  

  uint8_t data[] = { 183, 128 };

  uint8_t symsize = 3;
  unsigned long num_data_symbols = 3;
  unsigned long num_parity_symbols = 2;
  unsigned long n_bits = num_data_symbols;
  uint8_t* data_with_rs = append_rs_code(data, &n_bits, num_parity_symbols, symsize);
 
  ctdd_assert( n_bits == 15 );
  ctdd_assert( data_with_rs[0] == 183 );
  ctdd_assert( data_with_rs[1] == 246 );

  uint8_t* data_without_rs = remove_rs_code(data_with_rs, num_data_symbols, num_parity_symbols, symsize);
  free(data_with_rs);

  ctdd_assert( data_without_rs );
  ctdd_assert( data[0] == data_without_rs[0] );
  ctdd_assert( data[1] == data_without_rs[1] );
  free(data_without_rs);

  num_parity_symbols = 1;
  unsigned long num_data_symbols_top = sizeof(unsigned short) * 8 / symsize;
  for(unsigned long num_data_symbols = 1; num_data_symbols < num_data_symbols_top; num_data_symbols++) {

    unsigned long n_bits = num_data_symbols * symsize;
    // unsigned long num_parity_symbols_top = num_data_symbols / 2 + 1;
    unsigned long num_parity_symbols_top = 1;

    for(unsigned long num_parity_symbols = 1; num_parity_symbols <= num_parity_symbols_top; num_parity_symbols++) {
      unsigned long lower_bound = 1 << ((num_data_symbols-1) * symsize);
      unsigned long upper_bound = 1 << (num_data_symbols * symsize);
      for(unsigned long k = lower_bound; k < upper_bound; k++) {
        unsigned long* key = malloc(sizeof(unsigned long));
        *key = k;
        get_key_from_k((unsigned long**)&key, symsize, num_data_symbols);
        n_bits = num_data_symbols;

        data_with_rs = append_rs_code(key, &n_bits, num_parity_symbols, symsize);
        show_bits((void*)key, num_data_symbols * symsize);

        ctdd_assert( data_with_rs );
        ctdd_assert( n_bits == (num_data_symbols + num_parity_symbols) * symsize);
        data_without_rs = remove_rs_code(data_with_rs, num_data_symbols, num_parity_symbols, symsize);
        ctdd_assert( data_without_rs );
        // show_bits(data_without_rs, num_data_symbols * symsize);
        free(data_with_rs);

        n_bits = num_data_symbols * symsize;
        for(unsigned long i = 0; i < n_bits; i++) {
          ctdd_assert( get_bit((void*)key, i) == get_bit((void*)data_without_rs, i) );
        }
        free(data_without_rs);
        free(key);
      }
    }
  }

  return 0;
}

int numeric_encoding_string_test() {

    for(uint8_t k = 1; k < 255; k++) {

        char s[10];
        sprintf(s, "%hhu", k);
        unsigned long data_len;
        void* data = encode_numeric_string(s, &data_len);
        uint8_t* final_s = decode_numeric_string(data, &data_len);
        ctdd_assert( !memcmp(final_s, s, data_len) );
        free(final_s);
        free(data);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        char s[50];
        sprintf(s, "%lu", k);
        unsigned long data_len;
        void* data = encode_numeric_string(s, &data_len);
        uint8_t* final_s = decode_numeric_string(data, &data_len);
        ctdd_assert( !memcmp(final_s, s, data_len) );
        free(final_s);
        free(data);
    }
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

        GRAPH* graph = watermark_encode8(&k, sizeof(k));
        unsigned long size;
        uint8_t* result = watermark_decode(graph, &size);
        ctdd_assert(size == 1);
        ctdd_assert(*result == k);
        free(result);
        graph_free(graph);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark_encode8(&k, sizeof(k));
        unsigned long size;
        uint8_t* result = watermark_decode(graph, &size);
        ctdd_assert(binary_sequence_equal((uint8_t*)&k, result, sizeof(k), size));
        free(result);
        graph_free(graph);
    }
    return 0;
}

int watermark2017_improved_test() {

    for(uint8_t k = 1; k < 255; k++) {
        GRAPH* graph = watermark_encode8(&k, sizeof(k));
        unsigned long size=1;
        uint8_t* result = watermark_decode_improved8(graph, &k, &size);
        ctdd_assert(size == 1);
        uint8_t res = *result == k;
        if(!res) {
          PRINT_K(k);
          printf("%hhu\n", *result);
        }
        ctdd_assert(res);
        free(result);
        graph_free(graph);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark_encode8(&k, sizeof(k));
        unsigned long size=sizeof(unsigned long);
        uint8_t* result = watermark_decode_improved8(graph, (uint8_t*)&k, &size);
        ctdd_assert(binary_sequence_equal((uint8_t*)&k, result, sizeof(k), size));
        free(result);
        graph_free(graph);
    }
    return 0;
}

int watermark2017_rs_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark_rs_encode8(&k, sizeof(k), 2);
        unsigned long size=2;
        uint8_t* result = watermark_rs_decode(graph, &size);
        ctdd_assert(size == 1);
        ctdd_assert(*result == k);
        free(result);
        graph_free(graph);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark_rs_encode8(&k, sizeof(k), 2);
        unsigned long size=2;
        uint8_t* result = watermark_rs_decode(graph, &size);
        ctdd_assert(binary_sequence_equal((uint8_t*)&k, result, sizeof(k), size));
        free(result);
        graph_free(graph);
    }
    return 0;
}

int watermark2017_rs_decode_improved_test() {

    for(uint8_t k = 1; k < 255; k++) {

        unsigned long num_parity_symbols = 2;
        unsigned long size = sizeof(k);
        GRAPH* graph = watermark_rs_encode8(&k, sizeof(k), num_parity_symbols);
        uint8_t* result = watermark_rs_decode_improved8(graph, &k, &size, num_parity_symbols);
        ctdd_assert(size == 1);
        ctdd_assert(*result == k);
        free(result);
        graph_free(graph);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        unsigned long num_parity_symbols = 2;
        unsigned long size = sizeof(k);
        GRAPH* graph = watermark_rs_encode8(&k, sizeof(k), num_parity_symbols);
        uint8_t* result = watermark_rs_decode_improved8(graph, &k, &size, num_parity_symbols);
        ctdd_assert(binary_sequence_equal((uint8_t*)&k, result, sizeof(k), size));
        free(result);
        graph_free(graph);
    }
    return 0;
}

int watermark2017_decode_analysis_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark_encode8(&k, sizeof(k));
        unsigned long size;
        uint8_t* result = watermark_decode_analysis(graph, &size);
        graph_free_info(graph);
        uint8_t* result_seq = get_sequence_from_bit_arr(result, size, &size);
        ctdd_assert(size == 1);
        ctdd_assert(binary_sequence_equal(&k, result_seq, sizeof(k), size));
        free(result);
        free(result_seq);
        graph_free(graph);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark_encode8(&k, sizeof(k));
        unsigned long size;
        uint8_t* result = watermark_decode_analysis(graph, &size);
        graph_free_info(graph);
        uint8_t* result_seq = get_sequence_from_bit_arr(result, size, &size);
        ctdd_assert(binary_sequence_equal((uint8_t*)&k, result_seq, sizeof(k), size));
        free(result);
        free(result_seq);
        graph_free(graph);
    }
    return 0;
}

int watermark2017_rs_decode_analysis_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark_rs_encode8(&k, sizeof(k), 3);
        unsigned long size=3;
        uint8_t* result = watermark_rs_decode_analysis(graph, &size);
        graph_free_info(graph);
        uint8_t* result_seq = get_sequence_from_bit_arr(result, size, &size);
        ctdd_assert(size == 1);
        ctdd_assert(binary_sequence_equal(&k, result_seq, sizeof(k), size));
        free(result);
        free(result_seq);
        graph_free(graph);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark_rs_encode8(&k, sizeof(k), 24);
        unsigned long size=24;
        uint8_t* result = watermark_rs_decode_analysis(graph, &size);
        graph_free_info(graph);
        uint8_t* result_seq = get_sequence_from_bit_arr(result, size, &size);
        ctdd_assert(binary_sequence_equal((uint8_t*)&k, result_seq, sizeof(k), size));
        free(result);
        free(result_seq);
        graph_free(graph);
    }
    return 0;
}

int dijkstra_recognition_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark_encode8(&k, sizeof(k));
        uint8_t result = dijkstra_check(graph);
        if(!result) {
            PRINT_K(k);
        }
        graph_free(graph);
        ctdd_assert(result);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark_encode8(&k, sizeof(k));
        ctdd_assert(dijkstra_check(graph));
        graph_free(graph);
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
    /*node_expand_to_while(graph->nodes[2]);
    node_expand_to_sequence(graph->nodes[3]);
    node_expand_to_if_then_else(graph->nodes[4]);
    node_expand_to_if_then(graph->nodes[1]);
    node_expand_to_sequence(graph->nodes[2]);
    node_expand_to_sequence(graph->nodes[12]);*/

    char* code = dijkstra_get_code(graph);
    ctdd_assert( !strcmp(code, "161312111412161111121") );
    graph_free(graph);
    free(code);
    // 29
    /*
    uint8_t k = 29;
    graph = watermark_encode(&k, sizeof(k));
    code = dijkstra_get_code(graph);
    graph_write_hamiltonian_dot(graph, "dot.dot", code);
    // ctdd_assert( !strcmp(code, "151151311121") );
    ctdd_assert( !strcmp(code, "151131151121") );
    graph_free(graph);
    free(code);
    // 28
    k = 28;
    graph = watermark_encode(&k, sizeof(k));
    code = dijkstra_get_code(graph);
    ctdd_assert( !strcmp(code, "15113112121") );
    graph_free(graph);
    free(code);
    */
    // bento 2017 - Fig 5
    graph = graph_create(1);
    node_expand_to_if_then_else(graph->nodes[0]);
    /*node_expand_to_while(graph->nodes[1]);
    node_expand_to_if_then(graph->nodes[4]);
    node_expand_to_sequence(graph->nodes[7]);*/
    node_expand_to_if_then(graph->nodes[1]);
    node_expand_to_while(graph->nodes[4]);
    node_expand_to_sequence(graph->nodes[7]);
    graph_topological_sort(graph);
    code = dijkstra_get_code(graph);
    ctdd_assert( !strcmp(code, "1614111311121") );
    graph_free(graph);
    free(code);

    return 0;
}

int dijkstra_watermark_code_test() {

    // bento 2017 - dijkstra Fig. 8
    GRAPH* graph = graph_create(1);
    node_expand_to_if_then_else(graph->nodes[0]);
    node_expand_to_if_then(graph->nodes[2]);
    node_expand_to_sequence(graph->nodes[3]);
    node_expand_to_while(graph->nodes[1]);
    node_expand_to_sequence(graph->nodes[2]);
    node_expand_to_if_then_else(graph->nodes[3]);
    node_expand_to_sequence(graph->nodes[12]);
    GRAPH* new = dijkstra_generate("161312111412161111121");
    ctdd_assert(new);
    ctdd_assert(dijkstra_is_equal(graph, new));
    graph_free(graph);
    graph_free(new);

    // bento 2017 - Fig 5
    graph = graph_create(1);
    node_expand_to_if_then_else(graph->nodes[0]);
    node_expand_to_if_then(graph->nodes[1]);
    node_expand_to_while(graph->nodes[4]);
    node_expand_to_sequence(graph->nodes[7]);
    new = dijkstra_generate("1614111311121");
    ctdd_assert(new);
    ctdd_assert( dijkstra_is_equal(graph, new) );
    graph_free(graph);
    graph_free(new);

    // a random graph i generated from a dijkstra code (with a 3-case switch case)
    GRAPH* g = dijkstra_generate("161512161213111111716111121151111");
    char* code = dijkstra_get_code(g);
    ctdd_assert( !strcmp(code, "161512161213111111716111121151111") );
    free(code);
    graph_free(g);

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* original = watermark_encode8(&k, sizeof(k));
        char* code = dijkstra_get_code(original);
        GRAPH* new = dijkstra_generate(code);
        // check that the resulting dijkstra code is the same
        ctdd_assert( dijkstra_is_equal(new, original) );
        unsigned long size;
        uint8_t* result = watermark_decode(new, &size);

        uint8_t res = *result == k;
        if(!res) {
          char* str = dijkstra_get_code(new);
          fprintf(stderr, "%hhu %hhu\n", *result, k);
          fprintf(stderr, "original(%s)\n", code);
          graph_print(original, NULL);
          fprintf(stderr, "new(%s):\n", str);
          free(str);
          graph_print(new, NULL);
        }
        free(result);
        graph_free(new);
        graph_free(original);
        free(code);
        ctdd_assert(res);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* original = watermark_encode8(&k, sizeof(k));
        char* code = dijkstra_get_code(original);
        GRAPH* new = dijkstra_generate(code);
        // check that the resulting dijkstra code is the same
        ctdd_assert( dijkstra_is_equal(original, new) );
        unsigned long size;
        uint8_t* result = watermark_decode(new, &size);
        graph_free(new);
        graph_free(original);
        free(code);
        uint8_t res = binary_sequence_equal((uint8_t*)&k, result, sizeof(k), size);
        free(result);
        ctdd_assert(res);
    }

    return 0;
}

int watermark_check_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark_encode8(&k, sizeof(k));
        uint8_t result = watermark_check(graph, &k, sizeof(k));
        if(!result) {
            PRINT_K(k);
        }
        graph_free(graph);
        ctdd_assert(result);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark_encode8(&k, sizeof(k));
        uint8_t result = watermark_check(graph, &k, sizeof(k));
        graph_free(graph);
        ctdd_assert(result);
    }
    return 0;
}

uint8_t has_x(uint8_t* bits, unsigned long size) {
    for(unsigned long i = 0; i < size; i++) {
        if(bits[i] == 'x') return 0;
    }
    return 1;
}

int watermark_check_analysis_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark_encode8(&k, sizeof(k));
        unsigned long size = sizeof(k);
        uint8_t* bit_arr = watermark_check_analysis(graph, &k, &size);
        graph_free_info(graph);
        uint8_t result = has_x(bit_arr, size);
        if(!result) {
            PRINT_K(k);
        }
        free(bit_arr);
        graph_free(graph);
        ctdd_assert(result);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark_encode8(&k, sizeof(k));
        unsigned long size = sizeof(k);
        uint8_t* bit_arr = watermark_check_analysis(graph, &k, &size);
        graph_free_info(graph);
        uint8_t result = has_x(bit_arr, size);
        free(bit_arr);
        graph_free(graph);
        ctdd_assert(result);
    }
    return 0;
}

int watermark_check_rs_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark_rs_encode8(&k, sizeof(k), 3);
        unsigned long size=sizeof(k);
        uint8_t result = watermark_rs_check(graph, &k, size, 3);
        if(!result) {
            PRINT_K(k);
        }
        graph_free(graph);
        ctdd_assert(result);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark_rs_encode8(&k, sizeof(k), 24);
        unsigned long size=sizeof(k);
        uint8_t result = watermark_rs_check(graph, &k, size, 24);
        graph_free(graph);
        ctdd_assert(result);
    }
    return 0;
}

int watermark_check_rs_analysis_test() {

    for(uint8_t k = 1; k < 255; k++) {

        GRAPH* graph = watermark_rs_encode8(&k, sizeof(k), 3);
        unsigned long size = sizeof(k);
        uint8_t* bit_arr = watermark_rs_check_analysis(graph, &k, &size, 3);
        graph_free_info(graph);
        uint8_t result = has_x(bit_arr, size);
        if(!result) {
            PRINT_K(k);
        }
        free(bit_arr);
        graph_free(graph);
        ctdd_assert(result);
    }
    for(unsigned long k = 1; k < 10e13; k=(k<<1)-(k>>1)) {

        GRAPH* graph = watermark_rs_encode8(&k, sizeof(k), 24);
        unsigned long size = sizeof(k);
        uint8_t* bit_arr = watermark_rs_check_analysis(graph, &k, &size, 24);
        graph_free_info(graph);
        uint8_t result = has_x(bit_arr, size);
        free(bit_arr);
        graph_free(graph);
        ctdd_assert(result);
    }
    return 0;
}

int sequence_alignment_score_test() {

    watermark_needleman_wunsch("GATTACA", "GTCGACGCA", 10, -10, -1);

    return 0;
}

// unsigned short get_key_from_k(unsigned short k, unsigned long symsize, unsigned long num_data_symbols) {
//   unsigned long key = 0;
//   for(unsigned long i = 0; i < sizeof(k); i++) {
//     invert_binary_sequence(&((uint8_t*)&k)[i], 1);
//   }
//   unsigned long n_bits = symsize * num_data_symbols;
//   for(unsigned long i = 0; i < n_bits; i++) {
//     set_bit((void*)&key, i, get_bit((void*)&k, i));
//   }
//   return key;
// }

int watermark2017_rs_3_bit_test() {

    uint8_t symsize = 3;
    {
      uint8_t data[] = { 183, 128 }; // 0b101_101_11|1_0000000 

      unsigned long num_parity_symbols = 2;
      unsigned long num_data_symbols = 3;
      GRAPH* graph = watermark_rs_encode(&data, num_data_symbols, num_parity_symbols, symsize);
      ctdd_assert( graph );
      uint8_t* result = watermark_rs_decode_improved(graph, &data, &num_data_symbols, num_parity_symbols, symsize);
      ctdd_assert( result );
      unsigned long size = num_data_symbols;
      ctdd_assert( size == 2 );
      ctdd_assert( result[0] == data[0] );
      ctdd_assert( result[1] == data[1] );
      free(result);
      graph_free(graph);
    }

    unsigned long num_data_symbols_top = sizeof(unsigned short) * 8 / symsize;
    for(unsigned long num_data_symbols = 1; num_data_symbols < num_data_symbols_top; num_data_symbols++) {

      unsigned long n_bits = num_data_symbols * symsize;
      unsigned long n_bytes = n_bits / 8 + !!(n_bits % 8);
      unsigned long num_parity_symbols = num_data_symbols / 2 + 1;

      unsigned long lower_bound = 1 << ((num_data_symbols-1) * symsize);
      unsigned long upper_bound = 1 << (num_data_symbols * symsize);
      for(unsigned long k = lower_bound; k < upper_bound; k++) {
        unsigned long* key = malloc(sizeof(unsigned long));
        *key = k; 
        get_key_from_k((unsigned long**)&key, symsize, num_data_symbols);
        GRAPH* graph = watermark_rs_encode(key, num_data_symbols, num_parity_symbols, symsize);
        graph_write_hamiltonian_dot(graph, "original.dot", NULL);
        // it should be able to take all cases of 1 edge removals like a champ
        if( num_parity_symbols >= 2 ) {
          CONNECTION* conn1 = conn_next_non_hamiltonian_edge(graph->nodes[0]->out);
          if(conn1) graph_oriented_disconnect(conn1->parent, conn1->node);
        } 
        graph_write_hamiltonian_dot(graph, "copy.dot", NULL);
        
        ctdd_assert( graph );

        unsigned long num_result_bytes = num_data_symbols;
        uint8_t* result = watermark_rs_decode_improved(graph, key, &num_result_bytes, num_parity_symbols, symsize);

        ctdd_assert( result );
        graph_free(graph);

        ctdd_assert( num_result_bytes == n_bytes );

        for(unsigned long i = 0; i < n_bits; i++) {
          ctdd_assert( get_bit((void*)key, i) == get_bit((void*)result, i) );
        }
        free(result);
        free(key);
      }
    }

    return 0;
}


int run_tests() {

    ctdd_verify(graph_test);
    ctdd_verify(numeric_encoding_string_test);
    ctdd_verify(get_bit_test);
    ctdd_verify(rs_test);
    ctdd_verify(merge_unmerge_test);
    ctdd_verify(append_remove_rs_code_test);
    ctdd_verify(watermark2014_test);
    ctdd_verify(watermark2014_rs_test);
    ctdd_verify(watermark2017_test);
    ctdd_verify(watermark2017_improved_test);
    ctdd_verify(watermark2017_rs_test);
    ctdd_verify(watermark2017_rs_decode_improved_test);
    ctdd_verify(watermark2017_decode_analysis_test);
    ctdd_verify(watermark2017_rs_decode_analysis_test);
    ctdd_verify(dijkstra_recognition_test);
    ctdd_verify(dijkstra_code_test);
    ctdd_verify(dijkstra_watermark_code_test);
    ctdd_verify(watermark_check_test);
    ctdd_verify(watermark_check_analysis_test);
    ctdd_verify(watermark_check_rs_test);
    ctdd_verify(watermark_check_rs_analysis_test);
    ctdd_verify(sequence_alignment_score_test);
    ctdd_verify(watermark2017_rs_3_bit_test);

    return 0;
}

int main() {

    srand(0);
	ctdd_setup_signal_handler();

	return ctdd_test(run_tests);
}
