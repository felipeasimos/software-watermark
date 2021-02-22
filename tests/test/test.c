#include <ctdd/ctdd.h>
#include "encoder/encoder.h"
#include "decoder/decoder.h"

void print_node_func(void* data, unsigned int data_len) {

	if( data ) {
		printf("\x1b[33m %lu \x1b[0m", *(unsigned long*)data);
	} else {
		printf("\x1b[33m null \x1b[0m");
	}
}

int encoder_test() {

	// big endian
	uint8_t n_be[4] = { 0b10101, 0b1110001, 0b0011010, 0b011001 };

	GRAPH* graph = watermark_encode(&n_be, 4);

	ctdd_assert(graph);

	graph_free(graph);

	return 0;
}

int decoder_test() {

	// we are representing this:
	/*
	      .-----------.
	      |-----.	  |.----.
	      |     |     ||    |
	      v     |     |v    |
	1     0     1     0     1
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
	uint8_t* data = watermark_decode(graph, &n);

	ctdd_assert(data);
	ctdd_assert( n );
	ctdd_assert( n == 1 );
	ctdd_assert( data[0] == 0b10101 );

	free(data);
	graph_free(graph);

	return 0;
}

uint8_t check(uint8_t* i, uint8_t* result, unsigned long n, unsigned long i_size) {

	// start checking i from the end
	for(unsigned long j=0; j < n; j++) {
		if( i[i_size - j - 1] != result[n - j - 1] ) return 0;
	}
	return 1;
}

int _1_to_10_8_test() {

	for(unsigned long i=1; i < 100000000; i++) {

		if( !( i % 100000) ) printf("%lu\n", i);
		GRAPH* graph = watermark_encode(&i, sizeof(i));
		ctdd_assert( graph );
		unsigned long num_bytes=0;
		uint8_t* result = watermark_decode(graph, &num_bytes);
		ctdd_assert( num_bytes );
		ctdd_assert( check((uint8_t*)&i, result, num_bytes, sizeof(i) ) );

		// 10^8 tests won't be a good idea if we don't deallocate memory
		graph_free(graph);
		free(result);
	}
	return 0;
}

int run_tests() {

	ctdd_verify(encoder_test);
	ctdd_verify(decoder_test);
	ctdd_verify(_1_to_10_8_test);

	return 0;
}

int main() {

	ctdd_setup_signal_handler();

	return ctdd_test(run_tests);
}
