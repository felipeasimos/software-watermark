#include "encoder/encoder.h"
#include "check/check.h"
#include "code_generator/code_generator.h"
#include "utils/utils.h"

#define MAX_SIZE 4095
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define MAX_SIZE_STR STRINGIFY(MAX_SIZE)

uint8_t get_uint8_t() {

	uint8_t n;
	scanf(" %hhu", &n);
	return n;
}

void encode_string() {

	char s[MAX_SIZE+1]={0};
	printf("input numeric string to be encoded (max size is %lu): ", (unsigned long) MAX_SIZE);
	scanf(" %" MAX_SIZE_STR "s", s);

    unsigned long data_len;
    void* data = encode_numeric_string(s, &data_len);

	GRAPH* g = watermark2017_encode(data, data_len);

    graph_print(g, NULL);
	char* code = watermark_get_code2014(g);

	printf("code generated:\n");
	printf("%s\n", code);

    free(data);
	graph_free(g);
	free(code);
}

uint8_t is_little_endian_machine() {

	uint16_t x = 1;

	// if it is little endian, 1 should be the first byte
	return ((uint8_t*)(&x))[0];
}

void encode_number() {

	unsigned long n;
	printf("input positive number to be encoded: ");
	scanf(" %lu", &n);

	if( is_little_endian_machine() ) {

		// reverse number
		unsigned long tmp;
		for( uint8_t i = 0; i < sizeof(n); i++ ) {

			((uint8_t*)(&tmp))[i] = ((uint8_t*)(&n))[sizeof(n) - i - 1];
		}
		n = tmp;
	}

	GRAPH* g = watermark2017_encode(&n, sizeof(n));

    GRAPH* _2014 = watermark2014_encode(&n, sizeof(n));

    graph_print(g, NULL);
	
    char* code = watermark_get_code2014(_2014);

    printf("%s\n", code);

	graph_free(g);
	//free(code);
}

unsigned long invert_unsigned_long(unsigned long n) {

    unsigned long tmp = n;
    for(uint8_t i=0; i < sizeof(n); i++) {

        ((uint8_t*)&n)[i] = ((uint8_t*)&tmp)[sizeof(n)-i-1];
    }
    return n;
}

void _save_successfull_distortion_attack(GRAPH* copy, unsigned long number_encoded, double error_percentage) {

    unsigned long num_bytes_copy=0;
    uint8_t* data_copy = graph_serialize(copy, &num_bytes_copy);

    char filename[50]={0};
    sprintf(filename, "tests/%lu_%F.dat", number_encoded, error_percentage);

    FILE* f = fopen(filename, "wb");

    if(!f || fwrite(data_copy, 1, num_bytes_copy, f) < num_bytes_copy) {

        fprintf(stderr, "ERROR WHEN WRITING %s", filename);
    }

    free(data_copy);
    fclose(f);
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

unsigned long _check(void* bit_sequence, unsigned long bit_seq_size, uint8_t* bit_arr, unsigned long bit_arr_size) {

    unsigned long correct = 0;
    unsigned long bit_seq_num_bits = bit_seq_size * 8;
    for(unsigned long i = 0; i < bit_arr_size; i++) {

        if(bit_arr[bit_arr_size -i - 1] == 'x') continue;

        uint8_t arr_value = !!(bit_arr[bit_arr_size - i - 1] - '0');
        uint8_t seq_value = !!get_bit(bit_sequence, bit_seq_num_bits - i - 1);
        if(  seq_value != arr_value ) continue;
        correct++;
    }
    return correct;
}

unsigned long _test_with_removed_connections(unsigned long i, unsigned long* worst_case, GRAPH* graph, _CONN_LIST* conns) {

    graph_load_copy(graph);

    for(_CONN_LIST* l = conns; l; l = l->next) {

        GRAPH* copy_node_from = graph_get_info(l->conn->parent);
        GRAPH* copy_node_to = graph_get_info(l->conn->node);

        graph_oriented_disconnect(copy_node_from, copy_node_to);
    }

    GRAPH* copy = graph_get_info(graph);
    graph_unload_all_info(graph);

    unsigned long num_bytes = sizeof(unsigned long);
    uint8_t* result = watermark_check_analysis(copy, &i, &num_bytes);

    unsigned long correct = 0;

    correct = _check(&i, sizeof(unsigned long), result, num_bytes);

    if( correct ) {

        //for(unsigned long j = 0; j < num_bytes; j++) fprintf(stderr, "%c ", result[j]);
        //fprintf(stderr, "\n");
        /*unsigned int u=1;
        for(GRAPH* node = copy; node; node = node->next) {
            graph_alloc(node, sizeof(unsigned int));
            memcpy(node->data, &u, sizeof(unsigned int));
            u++;
        }*/
        //graph_print(copy, NULL);
    } else {
        //_save_successfull_attack(copy, i, (*error)/(*total));
    }
    graph_free(copy);
    free(result);
    if( correct < *worst_case ) *worst_case = correct;
    return correct;
}

void _multiple_removal_test(
        unsigned long n_removals,
        unsigned long* total,
        unsigned long* correct,
        unsigned long* worst_case,
        unsigned long i,
        GRAPH* root,
        GRAPH* node,
        CONNECTION* conn,
        _CONN_LIST* l) {

    if(!n_removals) {

        (*total)++;
        (*correct) += _test_with_removed_connections(i, worst_case, root, l);
    } else {

        GRAPH* initial_node = node;
        for(; node; node = node->next) {

            if(node->connections) {

                CONNECTION* c = node->connections;
                if(node == initial_node) c = conn ? conn : c;
                 
                for(c = c->node == node->next ? c->next : c; c; c = c->next) {

                    _CONN_LIST* list = _conn_list_create(l, c);
                    _multiple_removal_test(n_removals-1, total, correct, worst_case, i, root, node, c->next, list);
                    _conn_list_free(list);
                }
            }
        }
    }
}

unsigned long get_lower_bound(unsigned long n_bits) {

    return 1 << (n_bits-1);
}

unsigned long get_higher_bound(unsigned long n_bits) {

    return 1 << (n_bits);
}

void removal_attack() {

    for(unsigned long n_removals = 1; n_removals < 2; n_removals++) {

        char filename[50]={0};
        sprintf(filename, "tests/report_rm_%lu.plt", n_removals);
        FILE* file = fopen(filename, "w");

        printf("number of removals: %lu\n", n_removals);
        for(unsigned long n_bits = 1; n_bits < 24; n_bits++) {

            printf("\tnumber of bits: %lu\n", n_bits);
            unsigned long lower_bound = get_lower_bound(n_bits);

            unsigned long higher_bound = get_higher_bound(n_bits);

            double identifiers_evaluated = 0;
            double correct_mean_sum = 0;
            double worst_case_mean_sum = 0;

            for(unsigned long i = lower_bound; i < higher_bound; i++) {

                unsigned long total = 0;
                unsigned long correct = 0;
                unsigned long worst_case = n_bits-1;

                unsigned long inverse_i = invert_unsigned_long(i);
                GRAPH* graph = watermark2017_encode(&inverse_i, sizeof(inverse_i));

                _multiple_removal_test(n_removals, &total, &correct, &worst_case, inverse_i, graph, graph, NULL, NULL);
                graph_free(graph);
                identifiers_evaluated++;

                correct_mean_sum += total ? ((double)correct)/total : n_bits;
                worst_case_mean_sum += worst_case;
            }
            fprintf(file, "%lu %F %F\n", n_bits, correct_mean_sum/identifiers_evaluated, worst_case_mean_sum/identifiers_evaluated);
            fflush(file);
        }
        fclose(file);
    }
}

int main() {

    printf("1) encode string\n");
    printf("2) encode number\n");
    printf("3) run removal test\n");
    printf("else) exit\n");
    switch( get_uint8_t() ) {

        // string
        case 1:
            encode_string();
            break;
        // number
        case 2:
            encode_number();
            break;
        case 3:
            removal_attack();
            break;
    }
}
