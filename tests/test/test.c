#include <ctdd/ctdd.h>
#include "encoder/encoder.h"
#include "decoder/decoder.h"

void print(void* data, unsigned int data_len) {

	printf("\x1b[33m %u \x1b[0m", *(uint8_t*)data);
}

int encoder_test() {

	uint8_t n = 0b1110100;
	GRAPH* graph = watermark_encode(&n, sizeof(n));

	graph_print(graph, print);
	ctdd_assert(1);

	// take out bits from the nodes
	GRAPH* node = graph;
	for(; node; node = node->next) {
		free(node->data);
		node->data = NULL;
	}

	unsigned long num_byte=0;
	void* data = watermark_decode(graph, &num_byte);

	ctdd_assert( data );
	printf("%d\n", *(uint8_t*)data);
	ctdd_assert( *(uint8_t*)data == n );

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
