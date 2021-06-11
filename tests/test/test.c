#include "ctdd/ctdd.h"
#include "encoder/encoder.h"
#include "decoder/decoder.h"
#include "rs_api/rs.h"
#include "code_generator/code_generator.h"
#include "metrics/metrics.h"
#include "check/check.h"

void print_node_func(void* data, unsigned int data_len) {

	if( data && data_len ) {
		printf("\x1b[33m %lu \x1b[0m", *(unsigned long*)data);
	} else {
		printf("\x1b[33m null \x1b[0m");
	}
}

int reed_solomon_api_heavy_test() {

	srand(time(0));

	for(unsigned int i = 5; i < 124; i++) {

		uint8_t data[i];
		memset(data, 0x00, i);
		for(unsigned int j=0; j < i; j++) data[j] = i;

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

int _2014_test() {

	unsigned long n=100000000;
	for(unsigned long i=1; i < n; i++) {

		if( !( i % 100000 )) fprintf(stderr, "2014: %lu\n", i);
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

int _2017_test() {

	unsigned long n=100000000;
	for(unsigned long i=1; i < n; i++) {

		if( !( i % 100000) ) fprintf(stderr, "%lu\n", i);
		GRAPH* graph = watermark2017_encode(&i, sizeof(i));
		ctdd_assert( graph );
		unsigned long num_bytes=0;
		uint8_t* result = watermark2017_decode(graph, &num_bytes);
		ctdd_assert( num_bytes );
		ctdd_assert( check((uint8_t*)&i, result, num_bytes, sizeof(i) ) );

		// 10^8 tests won't be a good idea if we don't deallocate memory
		graph_free(graph);
		free(result);
	}
	return 0;
}


int simple_2017_test() {

	for(uint8_t i=1; i < 255; i++) {
		GRAPH* graph = watermark2017_encode(&i, sizeof(i));
		ctdd_assert( graph );

		unsigned long num_bytes=0;
		uint8_t* result = watermark2017_decode(graph, &num_bytes);
		ctdd_assert( num_bytes );
		ctdd_assert( check((uint8_t*)&i, result, num_bytes, sizeof(i) ) );

		// 10^8 tests won't be a good idea if we don't deallocate memory
		graph_free(graph);
		free(result);
	}
	return 0;
}

int simple_2014_test() {

	for(uint8_t i=1; i < 255; i++) {
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

int simple_2017_test_with_rs() {
	
	for(uint8_t k=5; k < 127; k++) {
		uint8_t i[k];
		for(int j=0; j < k; j++) i[j] = ( rand() % 254 ) + 1;
		GRAPH* graph = watermark2017_encode_with_rs(&i, k, k);
		ctdd_assert( graph );
		unsigned long num_bytes = 0;
		uint8_t* result = watermark2017_decode_with_rs(graph, &num_bytes, k);
		ctdd_assert( num_bytes );
		ctdd_assert( check((uint8_t*)&i, result, k, num_bytes ) );

		// 10^8 tests won't be a good idea if we don't deallocate memory
		graph_free(graph);
		free(result);
	}
	return 0;
}

int simple_2014_test_with_rs() {

	for(uint8_t k=5; k < 127; k++) {
		uint8_t i[k];
		for(int j=0; j < k; j++) i[j] = ( rand() % 254 ) + 1;
		GRAPH* graph = watermark2014_encode_with_rs(&i, k, k);
		ctdd_assert( graph );
		unsigned long num_bytes = 0;
		uint8_t* result = watermark2014_decode_with_rs(graph, &num_bytes, k);
		ctdd_assert( num_bytes );
		ctdd_assert( check((uint8_t*)&i, result, k, num_bytes ) );

		// 10^8 tests won't be a good idea if we don't deallocate memory
		graph_free(graph);
		free(result);
	}
	return 0;
}


int code_test() {

	uint8_t n[] = {16};
	GRAPH* graph = watermark2014_encode(&n, sizeof(n));
	//graph_print(graph, NULL);
	char* code = watermark_get_code2014(graph);
	//printf("'%s'\n", code);
	free(code);
	graph_free(graph);

	return 0;
}

int serialization_test() {

    for(uint8_t i = 1; i < 255; i++) {

        GRAPH* graph = watermark2017_encode(&i, sizeof(uint8_t));
        
        unsigned long num_bytes = 0;
        uint8_t* data = graph_serialize(graph, &num_bytes);
        ctdd_assert(num_bytes);

        GRAPH* copy = graph_deserialize(data);
        uint8_t* result = watermark2017_decode(copy, &num_bytes);
        graph_free(copy);
        graph_free(graph);
        free(data);
        ctdd_assert( *result == i );
        free(result);
    }
    return 0;
}

int copy_test() {

    for(uint8_t i = 1; i < 255; i++) {

        GRAPH* graph = watermark2017_encode(&i, sizeof(uint8_t));
        
        GRAPH* copy = graph_copy(graph);
        unsigned long num_bytes;
        uint8_t* result = watermark2017_decode(copy, &num_bytes);
        graph_free(copy);
        graph_free(graph);
        ctdd_assert( *result == i );
        free(result);
    }
    return 0;
}

int simple_checker_test() {

    for(uint8_t i=1; i < 255; i++) {
		GRAPH* graph = watermark2017_encode(&i, sizeof(uint8_t));
		ctdd_assert( graph );
		ctdd_assert( watermark_check(graph, &i, sizeof(uint8_t)) );

		// 10^8 tests won't be a good idea if we don't deallocate memory
		graph_free(graph);
	}
	return 0;
}

int ascii_numbers_test() {

    for(unsigned long i=1; i < 10000000; i*=2) {

        char str[10]={0};
        sprintf(str, "%lu", i);

        unsigned long data_len = 0;
        void* data = encode_numeric_string(str, &data_len);

        char* new_str = decode_numeric_string(data, data_len);
        ctdd_assert(!strcmp( str, new_str ));

        free(data);
        free(new_str);
    } 

    return 0;
}

int run_tests() {

    ctdd_verify(ascii_numbers_test);
	ctdd_verify(reed_solomon_api_heavy_test);
	ctdd_verify(code_test);
	ctdd_verify(simple_2014_test);
	ctdd_verify(simple_2017_test);
	ctdd_verify(simple_2014_test_with_rs);
	ctdd_verify(simple_2017_test_with_rs);
    ctdd_verify(copy_test);
    ctdd_verify(serialization_test);
    ctdd_verify(simple_checker_test);

    //ctdd_verify(_2017_test);
	//ctdd_verify(_2014_test);
	//ctdd_verify(rs_encoder_decoder_test);

	return 0;
}

int main() {

	ctdd_setup_signal_handler();

	return ctdd_test(run_tests);
}
