#include "ctdd/ctdd.h"
#include "encoder/encoder.h"
#include "decoder/decoder.h"
#include "rs_api/rs.h"
#include "code_generator/code_generator.h"
#include "metrics/metrics.h"

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

void _save_successfull_distortion_attack(GRAPH* graph, GRAPH* copy, unsigned long number_encoded, double error_percentage) {

    unsigned long num_bytes_graph=0;
    uint8_t* data_graph = graph_serialize(graph, &num_bytes_graph);

    unsigned long num_bytes_copy=0;
    uint8_t* data_copy = graph_serialize(copy, &num_bytes_copy);

    data_graph = realloc(data_graph, num_bytes_graph + num_bytes_copy);
    memcpy(data_graph + num_bytes_graph, data_copy, num_bytes_copy);
    free(data_copy);

    char filename[50]={0};
    sprintf(filename, "tests/%lu_%F.dat", number_encoded, error_percentage);

    FILE* f = fopen(filename, "wb");

    if(!f || fwrite(data_graph, 1, num_bytes_graph + num_bytes_copy, f) < num_bytes_copy + num_bytes_graph) {

        fprintf(stderr, "ERROR WHEN WRITING %s", filename);
    }

    free(data_graph);
    fclose(f);
}

unsigned long _get_num_non_hamiltonian_edges(GRAPH* graph) {

    unsigned long n=0;
    for(;graph; graph = graph->next)

        if(graph->connections)
            for(CONNECTION* conn = graph->connections->node == graph->next ? graph->connections->next : graph->connections; conn; conn = conn->next)
                n++;
    return n;
}

typedef struct _CONN_LIST {

    struct _CONN_LIST* next;
    CONNECTION* conn;
} _CONN_LIST;

_CONN_LIST* _conn_list_copy(_CONN_LIST* l) {

    if(!l) return NULL;
    _CONN_LIST* list = malloc(sizeof(_CONN_LIST));
    list->conn = l->conn;
    list->next = _conn_list_copy(l->next);
    return list;
}

_CONN_LIST* _conn_list_create(_CONN_LIST* l, CONNECTION* new_conn) {

    _CONN_LIST* list = malloc(sizeof(_CONN_LIST));
    list->conn = new_conn;
    list->next = _conn_list_copy(l);
    return list;
}

void _conn_list_free(_CONN_LIST* list) {

    if(!list) return;
    _conn_list_free(list->next);
    free(list);
}

void _test_with_removed_connections(double* total, double* error, unsigned long i, GRAPH* graph, _CONN_LIST* conns) {

    (*total)++;
    graph_load_copy(graph);

    for(_CONN_LIST* l = conns; l; l = l->next) {

        GRAPH* copy_node_from = graph_get_info(l->conn->parent);
        GRAPH* copy_node_to = graph_get_info(l->conn->node);

        graph_oriented_disconnect(copy_node_from, copy_node_to);
    }

    GRAPH* copy = graph_get_info(graph);
    graph_unload_all_info(graph);

    unsigned long num_bytes=0;
    uint8_t* result = watermark2017_decode(copy, &num_bytes);

    if( !result || !check((uint8_t*)&i, result, num_bytes, sizeof(i) ) ) {

        (*error)++;
        _save_successfull_distortion_attack(graph, copy, i, (*error)/(*total));
    }
    graph_free(copy);
}

void _multiple_removal_test(unsigned long n_removals, double* total, double* error, unsigned long i, GRAPH* root, GRAPH* graph, CONNECTION* conn, _CONN_LIST* l) {

    if(!n_removals) {

        _test_with_removed_connections(total, error, i, root, l);
    } else {

        uint8_t first=1;
        for(; graph; graph = graph->next) {

            if(graph->connections) {

                CONNECTION* c = graph->connections;
                if(first) {
                    first=0;
                    c = conn ? conn : c;
                }
                 
                for(c = c->node == graph->next ? c->next : c; c; c = c->next) {

                    _CONN_LIST* list = _conn_list_create(l, c);
                    _multiple_removal_test(n_removals-1, total, error, i, root, graph, c->next, list);
                    _conn_list_free(list);
                }
            }
        }
    }
}

int removal_attack_test() {

    double total = 0;
    double error = 0;
    double last_error = 0;
    unsigned long last_correct_number=0;

    unsigned long n_removals=1;
    unsigned long i=1;
    for(; total == 0 || (error/total) < 0.2; n_removals++) {

        printf("number of removals: %lu\n", n_removals);
        for(; i < 100000000 && (total == 0 || (error/total) < 0.2 ); i++) {

            GRAPH* graph = watermark2017_encode(&i, sizeof(i));
            ctdd_assert( graph );

            _multiple_removal_test(n_removals, &total, &error, i, graph, graph, NULL, NULL);

            if(total && error != last_error) {

                printf("error percentage: %F\n", error/total);
            } else {
                last_correct_number=i;
            }

            graph_free(graph);
            last_error = error;
        }
    }

    FILE* report = fopen("tests/report.txt", "w");
    fprintf(report, "number of removals: %lu\nlast correct number evaluated: %lu\nerror percentage: %F\nlast number evaluated: %lu", n_removals, i, error/total, i);
    fclose(report);

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

int run_tests() {

    ctdd_verify(removal_attack_test);
	ctdd_verify(reed_solomon_api_heavy_test);
	ctdd_verify(code_test);
	ctdd_verify(simple_2014_test);
	ctdd_verify(simple_2017_test);
	ctdd_verify(simple_2014_test_with_rs);
	ctdd_verify(simple_2017_test_with_rs);
    ctdd_verify(copy_test);
    ctdd_verify(serialization_test);

	//ctdd_verify(_2017_test);
	//ctdd_verify(_2014_test);
	//ctdd_verify(rs_encoder_decoder_test);

	return 0;
}

int main() {

	ctdd_setup_signal_handler();

	return ctdd_test(run_tests);
}
