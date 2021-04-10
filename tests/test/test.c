#include "ctdd/ctdd.h"
#include "encoder/encoder.h"
#include "decoder/decoder.h"
#include "rs_api/rs.h"
#include "code_generator/code_generator.h"

void print_node_func(void* data, unsigned int data_len) {

	if( data && data_len ) {
		printf("\x1b[33m %lu \x1b[0m", *(unsigned long*)data);
	} else {
		printf("\x1b[33m null \x1b[0m");
	}
}

int encoder_test() {

	// big endian
	uint8_t n_be[4] = { 0xde, 0xad, 0xbe, 0xef };

	GRAPH* graph = watermark2014_encode(&n_be, 4);

	ctdd_assert(graph);
	graph_free(graph);

	return 0;
}

int decoder_test() {

	// we are representing this:
	/*
	      .-----------.
	      |-----.	    |.----.
	      |     |     ||    |
	      v     |     |v    |
	1 ->  0 ->  1 ->  0 ->  1
	# edges = 7
	# nodes = 5
	# cyclomatic complexity = 4
	# non hamiltoninan edges = 3
	*/

	unsigned long idx = 1;

	// 1
	GRAPH* graph = graph_create(&idx, sizeof(idx));
	GRAPH* final = graph;
	idx++;

	// 0
	graph_insert(final, graph_create(&idx, sizeof(idx)));
	graph_oriented_connect(final, final->next);
	final = final->next;
	idx++;

	// 1
	graph_insert(final, graph_create(&idx, sizeof(idx)));
	graph_oriented_connect(final, final->next);
	graph_oriented_connect(final->next, final);
	final = final->next;
	idx++;

	// 0
	graph_insert(final, graph_create(&idx, sizeof(idx)));
	graph_oriented_connect(final, final->next);
	graph_oriented_connect(final->next, final->prev);
	final = final->next;
	idx++;

	// 1
	graph_insert(final, graph_create(&idx, sizeof(idx)));
	graph_oriented_connect(final, final->next);
	graph_oriented_connect(final->next, final);
	final = final->next;
	idx++;

	// null node at the end
	graph_insert(final, graph_create(&idx, sizeof(idx)));
	graph_oriented_connect(final, final->next);

	unsigned long n=0;
	uint8_t* data = watermark2014_decode(graph, &n);

	ctdd_assert(data);
	ctdd_assert( n );
	ctdd_assert( n == 1 );
	ctdd_assert( data[0] == 0x15 );

	ctdd_assert( watermark_num_edges(graph) == 8 );
	ctdd_assert( watermark_num_nodes(graph) == 6 );
	ctdd_assert( watermark_cyclomatic_complexity(graph) == 4 );
	ctdd_assert( watermark_num_hamiltonian_edges(graph) == 5 );

	// test with missing hamiltonian edge
	graph_oriented_disconnect(graph->next, graph->next->next);
	ctdd_assert( watermark_num_hamiltonian_edges(graph) == 4 );	
	ctdd_assert( watermark_num_edges(graph) == 7 );
	ctdd_assert( watermark_num_nodes(graph) == 6 );
	ctdd_assert( watermark_cyclomatic_complexity(graph) == 3 );

	free(data);
	graph_free(graph);

	return 0;
}

int reed_solomon_api_test() {

	// RS(255, 223)
	int symbol_size = 8;
	int gfpoly = 0x187;
	int fcr = 0;
	int prim = 1;

	// also the number of roots in the generator polynomial
	int parity_len = 32; // 2T
	int data_len = 223;

	struct rs_control* rs = init_rs(symbol_size, gfpoly, fcr, prim, parity_len);
	ctdd_assert(rs);
	srand(time(0));

	uint8_t data[data_len];
	for(unsigned int i = 0; i < (unsigned int) data_len; i++) data[i] = rand();
	uint16_t par[parity_len];
	memset(par, 0x00, parity_len * sizeof(uint16_t));

	encode_rs8(rs, (uint8_t*)&data, data_len, (uint16_t*)&par, 0);
	
	uint8_t received_data[data_len];
	memcpy(received_data, data, data_len);
	for(unsigned int i = 0; i < 5; i++) received_data[rand() % data_len] = rand();

	decode_rs8(rs, (uint8_t*)&received_data, (uint16_t*)&par, data_len, NULL, 0, NULL, 0, NULL);

	for(unsigned int i=0; i < (unsigned int) data_len; i++) ctdd_assert( received_data[i] == data[i] );

	free_rs(rs);

	return 0;
}

int reed_solomon_api_heavy_test() {

	srand(time(0));

	for(unsigned int i = 5; i < 124; i++) {

		uint8_t data[i];
		memset(data, 0x00, i);
		for(unsigned int j=1; j < i; j++) data[j-1] = i;

		uint16_t par[i];
		memset(par, 0x00, i*2);

		rs_encode((uint8_t*)&data, i, (uint16_t*)&par, i);

		uint8_t received_data[i];
		memcpy(received_data, data, i);

		// tamper with message
		for(unsigned int j=0; j < i/2 - 1; j++) received_data[ rand() % i ] = rand();

		int result = rs_decode((uint8_t*)&received_data, i, (uint16_t*)&par, i);

		ctdd_assert(result > -1);

		ctdd_assert( !memcmp(received_data, data, i) );
	}

	return 0;
}

uint8_t check(uint8_t* i, uint8_t* result, unsigned long n, unsigned long i_size) {

	// start checking i from the end
	for(unsigned long j=0; j < n; j++) {
		if( i[i_size - j - 1] != result[n - j - 1] ) return 0;
	}
	return 1;
}

int rs_encoder_decoder_test() {

	for(unsigned long i=1; i < 100000000; i++) {

		if( !( i % 100000) ) printf("%lu\n", i);

		// get parity
		uint16_t par[sizeof(i)];
		memset( par, 0x00, sizeof(par) );

		rs_encode((uint8_t*)&i, sizeof(i), (uint16_t*)&par, sizeof(i));

		// get parity with the data inside the graph
		uint8_t final_data[sizeof(i) + sizeof(par)];
		memcpy(final_data, &i, sizeof(i));
		memcpy(final_data + sizeof(i), par, sizeof(par));

		GRAPH* graph = watermark2014_encode(&final_data, sizeof(final_data));
		ctdd_assert( graph );
		unsigned long num_bytes=0;
		uint8_t* result = watermark2014_decode(graph, &num_bytes);
		ctdd_assert( num_bytes );
		// check i
		ctdd_assert( check((uint8_t*)&i, result, num_bytes - sizeof(par), sizeof(i)) );

		// check parity
		ctdd_assert( !memcmp(par, result+num_bytes-sizeof(par), sizeof(par)) );

		// 10^8 tests won't be a good idea if we don't deallocate memory	
		free(result);
		graph_free(graph);
	}

	return 0;
}

int _1_to_10_8_test() {

	for(unsigned long i=1; i < 100000000; i++) {

		printf("i: %lu\n", i);

		if( !( i % 100000) ) printf("%lu\n", i);
		GRAPH* graph = watermark2014_encode(&i, sizeof(i));
		ctdd_assert( graph );
		unsigned long num_bytes=0;
		uint8_t* result = watermark2014_decode(graph, &num_bytes);
		ctdd_assert( num_bytes );
		ctdd_assert( check((uint8_t*)&i, result, num_bytes, sizeof(i) ) );

		// 10^8 tests won't be a good idea if we don't deallocate memory
		graph_free(graph);
		free(result);
	}
	return 0;
}

int code_test() {

	uint8_t n[] = {16};
	GRAPH* graph = watermark2014_encode(&n, sizeof(n));
	graph_print(graph, NULL);
	char* code = watermark_get_code(graph);
	printf("'%s'\n", code);
	free(code);
	graph_free(graph);

	return 0;
}

int run_tests() {

	//ctdd_verify(encoder_test);
	ctdd_verify(decoder_test);
	ctdd_verify(reed_solomon_api_test);
	ctdd_verify(reed_solomon_api_heavy_test);
	ctdd_verify(code_test);
	// ctdd_verify(rs_encoder_decoder_test);
	ctdd_verify(_1_to_10_8_test);

	return 0;
}

int main() {

	ctdd_setup_signal_handler();

	return ctdd_test(run_tests);
}
