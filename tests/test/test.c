#include <ctdd/ctdd.h>
#include "encoder/encoder.h"
#include "decoder/decoder.h"

void print(void* data, unsigned int data_len) {

	if( data ) {
		printf("\x1b[33m %lu \x1b[0m", *(unsigned long*)data);
	} else {
		printf("\x1b[33m null \x1b[0m");
	}
}

int encoder_test() {

	// big endian
	uint8_t n_be[4] = {0b10101, 0b1110001, 0b0011010, 0b011001};

	GRAPH* graph = watermark_encode(&n_be, 4);

	graph_print(graph, print);
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

	// 1
	GRAPH* graph = graph_empty();
	GRAPH* final = graph;

	// 0
	graph_insert(final, graph_empty());
	graph_oriented_connect(final->next, final);
	final = final->next;

	// 1
	graph_insert(final, graph_empty());
	graph_oriented_connect(final->next, final->prev);
	final = final->next;

	// 0
	graph_insert(final, graph_empty());
	final = final->next;

	// 1
	graph_insert(final, graph_empty());
	graph_oriented_connect(final->next, final);

	unsigned long n=0;
	uint8_t* data = watermark_decode(graph, &n);

	ctdd_assert(data);
	ctdd_assert( n );
	ctdd_assert( n == 1 );

	free(data);
	graph_free(graph);
}

int run_tests() {

	ctdd_verify(encoder_test);
	ctdd_verify(decoder_test);

	return 0;
}

int main() {

	ctdd_setup_signal_handler();

	return ctdd_test(run_tests);
}
