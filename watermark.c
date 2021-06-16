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

double _check(void* bit_sequence, unsigned long bit_seq_size, uint8_t* bit_arr, unsigned long bit_arr_size) {

    unsigned long correct = 0;
    unsigned long bit_seq_num_bits = bit_seq_size * 8;
    for(unsigned long i = 0; i < bit_arr_size; i++) {

        if(bit_arr[bit_arr_size -i - 1] == 'x') continue;

        uint8_t arr_value = !!(bit_arr[bit_arr_size - i - 1] - '0');
        uint8_t seq_value = !!get_bit(bit_sequence, bit_seq_num_bits - i - 1);
        if(  seq_value != arr_value ) continue;
        correct++;
    }
    return ((double)correct)/bit_arr_size;
}

double _test_with_removed_connections(unsigned long i, GRAPH* graph, _CONN_LIST* conns) {

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

    double correct = 0.0;

    if(result) correct = _check(&i, sizeof(unsigned long), result, num_bytes);

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
    return correct;
}

void _multiple_removal_test(unsigned long n_removals, double* total, double* correct, unsigned long i, GRAPH* root, GRAPH* node, CONNECTION* conn, _CONN_LIST* l) {

    (*total)++;
    if(!n_removals) {

        (*correct) += _test_with_removed_connections(i, root, l);
    } else {

        GRAPH* initial_node = node;
        for(; node; node = node->next) {

            if(node->connections) {

                CONNECTION* c = node->connections;
                if(node == initial_node) c = conn ? conn : c;
                 
                for(c = c->node == node->next ? c->next : c; c; c = c->next) {

                    _CONN_LIST* list = _conn_list_create(l, c);
                    _multiple_removal_test(n_removals-1, total, correct, i, root, node, c->next, list);
                    _conn_list_free(list);
                }
            }
        }
    }
}

void removal_attack() {

    for(unsigned long n_removals = 1; n_removals < 2; n_removals++) {

        double identifiers_evaluated = 0;
        double correct_mean_sum = 0;

        printf("number of removals: %lu\n", n_removals);
        for(unsigned long i = 1; i < 100000000; i++) {

            double total = 0;
            double correct = 0;

            unsigned long inverse_i = invert_unsigned_long(i);
            GRAPH* graph = watermark2017_encode(&inverse_i, sizeof(inverse_i));

            _multiple_removal_test(n_removals, &total, &correct, inverse_i, graph, graph, NULL, NULL);
            graph_free(graph);
            identifiers_evaluated++;

            correct_mean_sum += correct/total;

            printf("%lu %F %F\n", i, correct_mean_sum/identifiers_evaluated, total);
        }
    }
}

CONNECTION* _graph_create_random_connection(GRAPH* graph, unsigned long num_nodes) {

    unsigned long node_from_idx = rand() % (num_nodes-1);
    GRAPH* node_from = graph;
    for(unsigned long i = 0; i < node_from_idx; i++) node_from = node_from->next;

    unsigned long node_to_idx = rand() % (num_nodes-1);

    GRAPH* node_to = graph;
    for(unsigned long i=0; i< node_to_idx; i++) node_to = graph;

    if( node_to == node_from ) {

        if(node_to->next && node_to->next->next) {
            node_to = node_to->next;
        } else {
            node_to = node_to->prev;
        }
    }

    CONNECTION* conn = connection_create(node_from);
    conn->node = node_to;
    return conn;
}

//void _test_with_added_connections(double* total, double* error, unsigned long i, GRAPH* graph, _CONN_LIST* conns) {

    //(*total)++;
    //graph_load_copy(graph);

    //for(_CONN_LIST* l = conns; l; l = l->next) {

    //    GRAPH* copy_node_from = graph_get_info(l->conn->parent);
    //    GRAPH* copy_node_to = graph_get_info(l->conn->node);

    //    graph_oriented_connect(copy_node_from, copy_node_to);
    //}

    //GRAPH* copy = graph_get_info(graph);
    //graph_unload_all_info(graph);


    //if( !result || !check_unsigned_long(i, result, num_bytes ) ) {

    //    (*error)++;
        //_save_successfull_attack(copy, invert_unsigned_long(i), (*error)/(*total));
    //}
    //graph_free(copy);
    //free(result);
//}

void _multiple_addition_test(GRAPH* graph, _CONN_LIST* l, double* total, double* error, unsigned long num_nodes, unsigned long i, unsigned long n_additions) {

    if(!n_additions) {

   //     _test_with_added_connections(total, error, i, graph, l);
    } else {

        _CONN_LIST* list = _conn_list_create(l, _graph_create_random_connection(graph, num_nodes));
        _multiple_addition_test(graph, list, total, error, num_nodes, i, n_additions-1);
        _conn_list_free(list);
    }
}

void addition_random_attack() {

    double error = 0;
    double total = 0;
    double last_error = 0;

    for(unsigned long n_additions = 1; n_additions < 10; n_additions++) {

        char filename[50] = {0};
        sprintf(filename, "tests/report_add_%lu.plt", n_additions);
        FILE* report = fopen(filename, "w");

        for(unsigned long i = 1; i < 100000000 && (!total || (error/total) < 0.2 ); i++) {

            unsigned long inverse_i = invert_unsigned_long(i);

            GRAPH* graph = watermark2017_encode(&inverse_i, sizeof(inverse_i));

            unsigned long num_nodes = graph_num_nodes(graph);

            _multiple_addition_test(graph, NULL, &total, &error, num_nodes, i, n_additions);

            if((total && last_error != error) || i==1) {

                printf("%lu %F\n", i, error/total);
                fprintf(report, "%lu %F\n", i, error/total);
                fflush(report);
            }

            graph_free(graph);
            last_error = error;
        }
        fclose(report);
    }
}

void print_saved_graph() {

    char filename[50]={0};
    printf("filename with the saved graph:");
    scanf("%s", filename);
    FILE* file = fopen(filename, "r");

    fseek(file, 0L, SEEK_END);
    unsigned long num_bytes = ftell(file);
    rewind(file);

    uint8_t graph_data[num_bytes];

    fread(graph_data, 1, num_bytes, file);
    GRAPH* graph = graph_deserialize(graph_data);

    graph_print(graph, NULL);

    fclose(file);
}

int main() {

	uint8_t quit=0;
	while(!quit) {
        printf("1) encode string\n");
        printf("2) encode number\n");
        printf("3) run removal test\n");
        printf("4) print saved graph\n");
        printf("5) run addition test\n");
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
            case 4:
                print_saved_graph();
                break;
            case 5:
                addition_random_attack();
                break;
			default:
				quit=1;
		}
	}
}
