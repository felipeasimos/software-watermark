#include "encoder/encoder.h"
#include "decoder/decoder.h"
#include "check/check.h"
#include "code_generator/code_generator.h"
#include "utils/utils.h"

#define MAX_SIZE 4095
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define MAX_SIZE_STR STRINGIFY(MAX_SIZE)

//#define MAX_N_BITS 25
#define MAX_N_BITS 18
#define MAX_N_SYMBOLS 5
#define MAX_RS_REMOVALS 4

#define debug fprintf(stderr, "%s: %d\n", __FILE__, __LINE__)

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

    graph_print(g, utils_print_node);
	
    char* code = watermark_get_code2017(g);

    printf("%s\n", code);

	graph_free(g);
	free(code);
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

/*unsigned long _check(void* bit_sequence, unsigned long bit_seq_size, uint8_t* bit_arr, unsigned long bit_arr_size) {

    unsigned long correct = 0;
    unsigned long bit_seq_num_bits = bit_seq_size * 8;
    for(unsigned long i = 0; i < bit_arr_size; i++) {

        if(bit_arr[bit_arr_size - i - 1] == 'x') continue;

        uint8_t arr_value = !!(bit_arr[bit_arr_size - i - 1] - '0');
        uint8_t seq_value = !!get_bit(bit_sequence, bit_seq_num_bits - i - 1);
        if(  seq_value != arr_value ) continue;
        correct++;
    }
    return correct;
}*/

unsigned long _check(GRAPH* graph, GRAPH* copy) {

    GRAPH* graph_node = graph;
    GRAPH* copy_node = copy;

    unsigned long errors=0;
    while(graph_node->data && copy_node->data) {

        UTILS_NODE* graph_utils = graph_node->data;
        UTILS_NODE* copy_utils = copy_node->data;

        // if one of them if mute, skip it
        if(graph_utils->bit == UTILS_MUTE_NODE) {
            graph_node = graph_node->next;
        } else if(copy_utils->bit == UTILS_MUTE_NODE) {
            copy_node = copy_node->next;
        // make sure they point to the same bit index
        } else if( copy_utils->bit_idx < graph_utils->bit_idx ) {
            copy_node = copy_node->next;
        } else if( graph_utils->bit_idx < copy_utils->bit_idx ) {
            graph_node = graph_node->next;
        // at this point on, they are pointing to the same index, so they must always both skip
        } else {
            // if one of them is unknown, increment error
            if(graph_utils->bit == UTILS_UNKNOWN_NODE || graph_utils->bit == UTILS_UNKNOWN_NODE) {
                errors++;
            // if they are different, increment error
            } else if( graph_utils->bit != copy_utils->bit ) {
                errors++;
            }
            graph_node = graph_node->next;
            copy_node = copy_node->next;
            
        }
    }

    return errors;
}
unsigned long check_symbol_arrs(uint8_t* result, uint8_t* identifier, unsigned long result_len, unsigned long identifier_len) {

    unsigned long errors = 0;
    for(unsigned long i = 0; i < identifier_len && i < result_len; i++) {
        if( result[i] != identifier[i] || result[i] == UTILS_UNKNOWN_NODE || identifier[i]== UTILS_UNKNOWN_NODE ) errors++;
    }

    if( identifier_len > result_len ) errors += identifier_len - result_len;
    return errors;
}

unsigned long _test_with_removed_connections(
        void* identifier,
        unsigned long identifier_len,
        unsigned long* worst_case,
        GRAPH* graph,
        _CONN_LIST* conns,
        uint8_t rs) {

    graph_load_copy(graph);

    for(_CONN_LIST* l = conns; l; l = l->next) {
        GRAPH* copy_node_from = graph_get_info(l->conn->parent);
        GRAPH* copy_node_to = graph_get_info(l->conn->node);

        graph_oriented_disconnect(copy_node_from, copy_node_to);
    }
    GRAPH* copy = graph_get_info(graph);

    graph_unload_all_info(graph);

    unsigned long num_bytes = identifier_len;
    unsigned long errors = 0;
    if(rs) {

        // turn result to string of ascii numbers
        // 'result' = bit arr of encoded ascii numbers
        //uint8_t* result = watermark2017_decode_analysis_with_rs(copy, &num_bytes, num_bytes+1);
        uint8_t* result = watermark_check_analysis_with_rs(copy, identifier, &num_bytes, num_bytes+1);
        //unsigned long result_len = num_bytes;
        //decoder_graph_to_utils_nodes(copy);
        //checker_graph_to_utils_nodes(copy);

        // prepare identifier (currently it is an encoded ascii array)
        identifier = decode_numeric_string(identifier, &identifier_len);

        // 'bin_result' = encoded ascii numbers
        uint8_t* bin_result = bit_arr_to_bin(result, &num_bytes);
        //unsigned long bin_result_len = num_bytes;

        // 'final_result' = decoded ascii numbers
        uint8_t* final_result = decode_numeric_string(bin_result, &num_bytes);
        errors = check_symbol_arrs(final_result, identifier, num_bytes, identifier_len);

        /*if(errors >= MAX_N_SYMBOLS) {
            printf("errors: %lu\n", errors);
            printf("identifier (decoded ascii numbers): %lu\n\t", identifier_len);
            for(unsigned long i = 0; i < identifier_len; i++) printf("%hhu('%c')[%lu] ", ((uint8_t*)identifier)[i], ((uint8_t*)identifier)[i], i);
            printf("\n");

            printf("result (bit arr of encoded ascii numbers): %lu\n\t", result_len);
            for(unsigned long i = 0; i < result_len; i++) printf("%hhu[%lu] ", result[i], i);
            printf("\n");

            printf("bin result (encoded ascii numbers): %lu\n\t", bin_result_len);
            for(unsigned long i = 0; i < bin_result_len; i++) printf("%hhu[%lu] ", bin_result[i], i);
            printf("\n");
            
            printf("final result (decoded ascii numbers): %lu\n\t", num_bytes);
            for(unsigned long i = 0; i < num_bytes; i++) printf("%hhu('%c')[%lu] ", final_result[i],
                    final_result[i] == 'x' || ( final_result[i] <= '9' && final_result[i] >= '0') ? final_result[i] : 'm'
                    ,i);
            printf("\n\n");
            graph_print(graph, utils_print_node);
            graph_print(copy, utils_print_node);
            exit(0);
        }*/
        free(result);
        free(bin_result);
        free(final_result);
        free(identifier);
    } else {

        free(watermark_check_analysis(copy, identifier, &num_bytes));
        //free(watermark2017_decode_analysis(copy, &num_bytes));
        checker_graph_to_utils_nodes(copy);
        //decoder_graph_to_utils_nodes(copy);
        errors = _check(graph, copy);
        /*if(errors > 0) {
            printf("errors: %lu\n", errors);
            graph_print(graph, utils_print_node);
            graph_print(copy, utils_print_node);
        }*/
    }
    graph_free(copy);

    if( errors > *worst_case ) *worst_case = errors;

    return errors;
}

void _multiple_removal_test(
        unsigned long n_removals,
        unsigned long* total,
        unsigned long* errors,
        unsigned long* worst_case,
        void* identifier,
        unsigned long identifier_len,
        GRAPH* root,
        GRAPH* node,
        CONNECTION* conn,
        _CONN_LIST* l,
        uint8_t rs) {

    if(!n_removals) {

        (*total)++;
        (*errors) += _test_with_removed_connections(identifier, identifier_len, worst_case, root, l, rs);
 
    } else {
        GRAPH* initial_node = node;

        for(; node; node = node->next) {

            if(node->connections) {

                CONNECTION* c = ( node->connections->node == node->next ? node->connections->next : node->connections );
                if(node == initial_node) c = (conn ? conn : c);
                 
                for(; c; c = c->next) {

                    _CONN_LIST* list = _conn_list_create(l, c);

                    _multiple_removal_test(n_removals-1, total, errors, worst_case, identifier, identifier_len, root, node, c->next, list, rs);

                    _conn_list_free(list);
                }
            }

        }
    }
}

unsigned long get_higher_bound(unsigned long n_bits) {

    return 1 << (n_bits);
}

unsigned long get_lower_bound(unsigned long n_bits) {

    return 1 << (n_bits-1);
}

unsigned long get_higher_bound_for_symbol(unsigned long n_symbols) {

    unsigned long higher_bound = 1;
    for(unsigned long i=0; i < n_symbols; i++) higher_bound *= 10;
    return higher_bound;
}

unsigned long get_lower_bound_for_symbol(unsigned long n_symbols) {

    return get_higher_bound_for_symbol(n_symbols-1);
}

void removal_attack() {

    for(unsigned long n_removals = 1; n_removals < 2; n_removals++) {

        char filename[50]={0};
        sprintf(filename, "tests/report_rm_%lu.plt", n_removals);
        FILE* file = fopen(filename, "w");

        printf("number of removals: %lu\n", n_removals);

        unsigned long matrix[MAX_N_BITS][MAX_N_BITS] = {{0}};

        for(unsigned long n_bits = 1; n_bits < MAX_N_BITS; n_bits++) {

            printf("\tnumber of bits: %lu\n", n_bits);

            unsigned long lower_bound = get_lower_bound(n_bits);

            unsigned long higher_bound = get_higher_bound(n_bits);

            double identifiers_evaluated = 0;
            double error_mean_sum = 0;
            double worst_case_mean_sum = 0;

            for(unsigned long i = lower_bound; i < higher_bound; i++) {

                unsigned long total = 0;
                unsigned long errors = 0;
                unsigned long worst_case = 0;

                unsigned long identifier = invert_unsigned_long(i);
                unsigned long identifier_len = sizeof(unsigned long);

                GRAPH* graph = watermark2017_encode(&identifier, identifier_len);
                _multiple_removal_test(n_removals, &total, &errors, &worst_case, &identifier, identifier_len, graph, graph, NULL, NULL, 0);
                graph_free(graph);
                identifiers_evaluated++;

                error_mean_sum += ((double)errors)/total;
                worst_case_mean_sum += worst_case;
                matrix[worst_case][n_bits]++;
            }
            fprintf(file, "%lu %F %F\n", n_bits, error_mean_sum/identifiers_evaluated, worst_case_mean_sum/identifiers_evaluated);
            fflush(file);
        }
        fclose(file);

        FILE* matrix_file = fopen("tests/report_matrix.plt", "wb");
        unsigned long max_n_bits = MAX_N_BITS;
        fwrite(&max_n_bits, sizeof(unsigned long), 1, matrix_file);
        fwrite(&max_n_bits, sizeof(unsigned long), 1, matrix_file);
        fwrite(&matrix, sizeof(unsigned long), MAX_N_BITS * MAX_N_BITS, matrix_file);
        fclose(matrix_file);
    }
}

void removal_attack_with_rs() {

    unsigned long matrix[MAX_N_SYMBOLS][MAX_RS_REMOVALS] = {{0}};

    for(unsigned long n_removals = 1; n_removals < MAX_RS_REMOVALS; n_removals++) {

        char filename[50]={0};
        sprintf(filename, "tests/report_rm_%lu.plt", n_removals);
        FILE* file = fopen(filename, "w");

        printf("number of removals: %lu\n", n_removals);

        for(unsigned long n_symbols = 1; n_symbols < MAX_N_SYMBOLS; n_symbols++) {

            printf("\tnumber of symbols: %lu\n", n_symbols);

            unsigned long lower_bound = get_lower_bound_for_symbol(n_symbols);

            unsigned long higher_bound = get_higher_bound_for_symbol(n_symbols);

            double identifiers_evaluated = 0;
            double error_mean_sum = 0;
            double worst_case_mean_sum = 0;

            for(unsigned long i = lower_bound; i < higher_bound; i++) {

                unsigned long total = 0;
                unsigned long errors = 0;
                unsigned long worst_case = 0;

                char i_str[MAX_N_SYMBOLS]={0};
                sprintf(i_str, "%lu", i);
                unsigned long identifier_len = n_symbols;
                uint8_t* identifier = encode_numeric_string(i_str, &identifier_len);

                GRAPH* graph = watermark2017_encode_with_rs(identifier, identifier_len, identifier_len+1);

                //_multiple_removal_test(n_removals, &total, &errors, &worst_case, i_str, n_symbols, graph, graph, NULL, NULL, 1);
                _multiple_removal_test(n_removals, &total, &errors, &worst_case, identifier, identifier_len, graph, graph, NULL, NULL, 1);
                graph_free(graph);
                free(identifier);
                identifiers_evaluated++;

                error_mean_sum += ((double)errors)/total;
                worst_case_mean_sum += worst_case;
                matrix[worst_case][n_removals]++;
            }
            fprintf(file, "%lu %F %F\n", n_symbols, error_mean_sum/identifiers_evaluated, worst_case_mean_sum/identifiers_evaluated);
            fflush(file);
        }
        fclose(file);

        FILE* matrix_file = fopen("tests/report_matrix.plt", "wb");
        unsigned long max_n_symbols = MAX_N_SYMBOLS;
        unsigned long max_rs_removals = MAX_RS_REMOVALS;
        fwrite(&max_n_symbols, sizeof(unsigned long), 1, matrix_file);
        fwrite(&max_rs_removals, sizeof(unsigned long), 1, matrix_file);
        fwrite(&matrix, sizeof(unsigned long), MAX_N_SYMBOLS * MAX_RS_REMOVALS, matrix_file);
        fclose(matrix_file);
    }
}


void show_report_matrix() {
    
    char filename[MAX_SIZE]={0};

    printf("Insert the filepath of the matrix you want to read: ");
    scanf(" %s", filename);

    FILE* file = fopen(filename, "rb");

    unsigned long y, x;
    fread(&y, sizeof(unsigned long), 1, file);
    fread(&x, sizeof(unsigned long), 1, file);
    unsigned long sums[x];
    memset(sums, 0x00, sizeof(sums));

    printf("        ");
    for(unsigned long j = 0; j < x; j++) {

        printf("%-5lu", j);
        if(j != y-1) printf(" & ");
    }
    printf("\n");

    for(unsigned long i = 0; i < y; i++) {
        printf("%-5lu & ", i);
        for(unsigned long j = 0; j < x; j++) {
            unsigned long n;
            fread(&n, sizeof(unsigned long), 1, file);
            sums[j]+=n;
            printf("%-5lu", n);
            if(j != x-1) printf(" & ");
        }
        printf("\\\\ \\hline\n");
    }
    printf("Total & ");
    for(unsigned long j = 0; j < x; j++) {

        printf("%-5lu", sums[j]);
        if( j != x - 1 ) {
            printf(" & ");
        }
    }
    printf("\\\\ \\hline\n");

    fclose(file);
}

int main() {
    printf("1) encode string\n");
    printf("2) encode number\n");
    printf("3) run removal test\n");
    printf("4) run removal test with reed solomon\n");
    printf("5) show report matrix\n");
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
            removal_attack_with_rs();
            break;
        case 5:
            show_report_matrix();
            break;
    }
}
