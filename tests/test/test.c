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
	ctdd_assert(1);

	unsigned long n_bits = 0;
	uint8_t* data = watermark_decode(graph, &n_bits);

	ctdd_assert( n_bits == 4 );
	ctdd_assert( !!data );

	ctdd_assert( n_be[0] == data[0] );
	ctdd_assert( n_be[1] == data[1] );
	ctdd_assert( n_be[2] == data[2] );
	ctdd_assert( n_be[3] == data[3] );

	graph_free(graph);

	return 0;
}

int run_tests() {

	ctdd_verify(encoder_test);

	return 0;
}

int main() {

	ctdd_setup_signal_handler();

	return ctdd_test(run_tests);
}
