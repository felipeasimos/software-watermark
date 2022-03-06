#include "decoder/decoder.h"
#include "encoder/encoder.h"
#include "checker/checker.h"
#include "set/set.h"
#include "code_generation/code_generation.h"
#include "sequence_alignment/sequence_alignment.h"
#include "dijkstra/dijkstra.h"
#include <dirent.h>

#define MAX_SIZE 4095
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define MAX_SIZE_STR STRINGIFY(MAX_SIZE)

#define MAX_N_BITS 12
#define MAX_N_SYMBOLS 5
#define MAX_RS_REMOVALS 4
#define SIZE_PERCENTAGE 0.8

#define MATCH 1
#define MISMATCH -100
#define GAP -1

#define debug fprintf(stderr, "%s: %d\n", __FILE__, __LINE__)

typedef struct CONN_LIST {

struct CONN_LIST* next;
    CONNECTION* conn;
} CONN_LIST;

CONN_LIST* conn_list_copy(CONN_LIST* l) {

    if(!l) return NULL;
    CONN_LIST* list = malloc(sizeof(CONN_LIST));
    list->conn = l->conn;
    list->next = conn_list_copy(l->next);
    return list;
}

CONN_LIST* conn_list_create(CONN_LIST* l, CONNECTION* new_conn) {

    CONN_LIST* list = malloc(sizeof(CONN_LIST));
    list->conn = new_conn;
    list->next = conn_list_copy(l);
    return list;
}

void conn_list_free(CONN_LIST* list) {

    if(!list) return;
    conn_list_free(list->next);
    free(list);
}

typedef struct STATISTICS {
    unsigned long total;
    unsigned long errors;
    unsigned long worst_case;
} STATISTICS;

unsigned long get_upper_bound(unsigned long n_bits) {

    return 1 << (n_bits);
}

unsigned long get_lower_bound(unsigned long n_bits) {

    return 1 << (n_bits-1);
}

unsigned long get_upper_bound_for_symbol(unsigned long n_symbols) {

    unsigned long upper_bound = 1;
    for(unsigned long i=0; i < n_symbols; i++) upper_bound *= 10;
    return upper_bound;
}

unsigned long get_lower_bound_for_symbol(unsigned long n_symbols) {

    return get_upper_bound_for_symbol(n_symbols-1);
}

unsigned long invert_unsigned_long(unsigned long n) {

    invert_byte_sequence((uint8_t*)&n, sizeof(n));
    return n;
}

void show_report_matrix() {

    FILE* file = fopen("tests/report_matrix.plt", "rb");
    unsigned long n_rows, n_columns;
    fread(&n_rows, sizeof(unsigned long), 1, file);
    fread(&n_columns, sizeof(unsigned long), 1, file);

    unsigned long totals[n_rows];
    memset(totals, 0x00, sizeof(totals));

    printf("        ");
    for(unsigned long i = 0; i < n_columns; i++) {
        printf("%-5lu", i);
        if( i != n_columns-1) printf(" & ");
    }

    printf("\n");
    for(unsigned long i = 0; i < n_rows; i++) {

        printf("%-5lu & ", i);
        for(unsigned long j = 0; j < n_columns; j++) {

            unsigned long n;
            fread(&n, sizeof(unsigned long), 1, file);
            totals[j]+=n;
            printf("%-5lu", n);
            if(j != n_columns-1) printf(" & ");
        }
        printf("\\\\ \\hline\n");
    }
    printf("Total & ");
    for(unsigned long j = 0; j < n_columns; j++) {

        printf("%-5lu", totals[j]);
        if( j != n_columns - 1 ) {
            printf(" & ");
        }
    }
    printf("\\\\ \\hline\n");
    fclose(file);
}

void write_to_report_matrix(unsigned long* matrix, unsigned long n_rows, unsigned long n_columns) {

    FILE* file = fopen("tests/report_matrix.plt", "wb");
    fwrite(&n_rows, sizeof(unsigned long), 1, file);
    fwrite(&n_columns, sizeof(unsigned long), 1, file);
    fwrite(matrix, sizeof(unsigned long), n_rows * n_columns, file);
    fclose(file);
}

unsigned long _check(GRAPH* graph, GRAPH* copy) {

    NODE* graph_node = graph->nodes[0];
    NODE* copy_node = copy->nodes[0];

    unsigned long errors=0;
    while(graph_node && copy_node && graph_node->data && copy_node->data) {

#ifdef DEBUG
        graph_write_hamiltonian_dot(graph, "dot.dot", NULL);
        graph_write_hamiltonian_dot(copy, "dot.dot", NULL);
        fprintf(stderr, "graph idx: %lu, copy idx: %lu\n", graph_node->graph_idx, copy_node->graph_idx);
#endif

        UTILS_NODE* graph_utils = node_get_info(graph_node);
        UTILS_NODE* copy_utils = node_get_info(copy_node);

        // if one of them if mute, skip it
        if(graph_utils->checker_bit == BIT_MUTE) {
            graph_node = graph_get(graph, graph_node->graph_idx+1);
        } else if(copy_utils->checker_bit == BIT_MUTE) {
            copy_node = graph_get(copy, copy_node->graph_idx+1);
        // make sure they point to the same bit index
        } else if( copy_utils->bit_idx < graph_utils->bit_idx ) {
            copy_node = graph_get(copy_node->graph, copy_node->graph_idx+1);
        } else if( graph_utils->bit_idx < copy_utils->bit_idx ) {
            graph_node = graph_get(graph_node->graph, graph_node->graph_idx+1);
        // at this point on, they are pointing to the same index, so they must always both skip
        } else {
            // if one of them is unknown, increment error
            if(graph_utils->checker_bit == BIT_UNKNOWN || graph_utils->checker_bit == BIT_UNKNOWN) {
                errors++;
            // if they are different, increment error
            } else if( graph_utils->checker_bit != copy_utils->checker_bit ) {
                errors++;
            }
            graph_node = graph_get(graph_node->graph, graph_node->graph_idx+1);
            copy_node = graph_get(copy_node->graph, copy_node->graph_idx+1);
            
        }
    }

    return errors;
}

unsigned long _test_with_removed_connections(
        STATISTICS* statistics,
        void* identifier,
        unsigned long identifier_len,
        GRAPH* graph,
        CONN_LIST* conns) {

    // get copy and remove some of its connections
    GRAPH* copy = graph_copy(graph);
    for(CONN_LIST* l = conns; l; l = l->next) {
        graph_oriented_disconnect(copy->nodes[l->conn->parent->graph_idx], copy->nodes[l->conn->node->graph_idx]);
    }

    unsigned long num_bytes = identifier_len;
    unsigned long errors = 0;
    
    free(watermark_check_analysis(copy, identifier, &num_bytes));
    num_bytes = identifier_len;
    free(watermark_check_analysis(graph, identifier, &num_bytes));
    //free(watermark2017_decode_analysis(copy, &num_bytes));
    //decoder_graph_to_utils_nodes(copy);
    errors = _check(graph, copy);
    /*if(errors > 0) {
        printf("errors: %lu\n", errors);
        graph_print(graph, utils_print_node);
        graph_print(copy, utils_print_node);
    }*/
    graph_free_info(copy);
    graph_free(copy);

    if( errors > statistics->worst_case ) statistics->worst_case = errors;

    return errors;
}

void _multiple_removal_test(
    unsigned long n_removals,
    STATISTICS* statistics,
    void* identifier,
    unsigned long identifier_len,
    CONN_LIST* non_hamiltonian_edges,
    GRAPH* graph,
    NODE* node,
    CONNECTION* conn) {

    if(!n_removals) {

        statistics->total++;
        statistics->errors += _test_with_removed_connections(statistics, identifier, identifier_len, graph, non_hamiltonian_edges);
    } else {

        NODE* initial_node = node;

        for(; node; node = graph_get(graph, node->graph_idx+1)) {

            if(node->out) {

                CONNECTION* c = ( node->out->node == graph_get(graph, node->graph_idx+1) ? node->out->next : node->out );
                if(node == initial_node) c = (conn ? conn : c);
                 
                for(; c; c = c->next) {

                    CONN_LIST* list = conn_list_create(non_hamiltonian_edges, conn);
                    _multiple_removal_test(
                        n_removals-1,
                        statistics,
                        identifier,
                        identifier_len,
                        non_hamiltonian_edges,
                        graph,
                        node,
                        c->next);

                    conn_list_free(list);
                }
            }

        }
    }
}

void multiple_removal_test(unsigned long n_removals, GRAPH* graph, STATISTICS* statistics, void* identifier, unsigned long identifier_len) {

    _multiple_removal_test(n_removals, statistics, identifier, identifier_len, NULL, graph, graph->nodes[0], NULL);
}

void attack(unsigned long n_removals, unsigned long n_symbols) {

    for(unsigned long n_removal = 0; n_removal < n_removals; n_removal++) {

        unsigned long matrix[n_symbols][n_symbols];
        memset(matrix, 0x00, sizeof(matrix));
        for(unsigned long n_bits = 0; n_bits < n_symbols; n_bits++) {

            printf("number of bits: %lu\n", n_bits);
            unsigned long lower_bound = get_lower_bound(n_bits);
            unsigned long upper_bound = get_upper_bound(n_bits);

            double identifiers_evaluated = 0;

            for(unsigned long i = lower_bound; i < upper_bound; i++) {

                STATISTICS statistics = {0};

                unsigned long identifier = invert_unsigned_long(i);
                unsigned long identifier_len = sizeof(unsigned long);

                GRAPH* graph = watermark_encode(&identifier, identifier_len);

                multiple_removal_test(n_removal, graph, &statistics, &identifier, identifier_len);

                graph_free(graph);
                identifiers_evaluated++;

                matrix[statistics.worst_case][n_bits]++;
            }
        }
        write_to_report_matrix((unsigned long*)&matrix, n_removals, n_symbols);
    }
}

void removal_attack_for_bits(unsigned long n_removals, unsigned long n_symbols) {

    attack(n_removals, n_symbols);
    show_report_matrix();
}

void removal_attack_with_rs_for_bits(unsigned long n_removals, unsigned long n_symbols) {
    n_removals = n_symbols;
    show_report_matrix();
}

void removal_attack_for_symbols(unsigned long n_removals, unsigned long n_symbols) {

    n_removals = n_symbols;
    show_report_matrix();
}

void removal_attack_with_rs_for_symbols(unsigned long n_removals, unsigned long n_symbols) {

    n_removals = n_symbols;
    show_report_matrix();
}

int ask_for_comparison(char* dijkstra_code) {

    printf("would you like to find a function that best fits this watermark?[y/n] ");
    char choice;
    scanf(" %c", &choice);

    if(choice!='y' && choice!='Y') return 0;

    char str[MAX_SIZE];
    printf("\npath to the C file to be analyzed: ");
    scanf(" %s", str);

    char str2[strlen("./cfg.sh    2>/dev/null") + strlen(str) + 1];
    str2[0]='\0';
    strcat(str2, "./cfg.sh ");
    strcat(str2, str);
    strcat(str2, " 2>/dev/null");
    system(str2);

    long highest_score = LONG_MIN;
    long highest_score_entry_point = 0;
    char highest_score_func[MAX_SIZE]={0};
    char highest_score_code[MAX_SIZE]={0};
    uint8_t file_has_at_least_one_dijkstra_graph = 0;
    DIR *dir;
    struct dirent *ent;
    if((dir = opendir(".")) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            // starts with '.' and ends with '.dot'
            unsigned long len = strlen(ent->d_name);
            if( len > 4 && ent->d_name[0] == '.' && ent->d_name[len-4] == '.' && ent->d_name[len-3] == 'd' && ent->d_name[len-2] == 'o' && ent->d_name[len-1] == 't') {

                FILE* f;
                if( (f = fopen(ent->d_name, "r")) != NULL ) {
                    GRAPH* g = graph_create_from_dot(f);
                    graph_topological_sort(g);
                    if( dijkstra_check(g) ) {
                        file_has_at_least_one_dijkstra_graph = 1;
                        char* current_dijkstra_code = dijkstra_get_code(g);
                        // only continues if the watermark can fit in the function
                        if( (float)strlen(current_dijkstra_code) > SIZE_PERCENTAGE * (float)strlen(dijkstra_code) ) {
                            NW_RESULT nw_result = watermark_needleman_wunsch(current_dijkstra_code, dijkstra_code, MATCH, MISMATCH, GAP);
                            long score = nw_result.score;
                            long entry_point = nw_result.entry_point;

                            fprintf(stderr, "%s:\n\tcode: %s\n\tscore: %ld\n\tentry_point: %ld\n", ent->d_name, current_dijkstra_code, score, entry_point);
                            if( score > highest_score ) {
                                highest_score = score;
                                highest_score_entry_point = entry_point;
                                memset(highest_score_func, 0x00, MAX_SIZE);
                                strncpy(highest_score_func, &ent->d_name[1], len-5);
                                strcpy(highest_score_code, current_dijkstra_code);
                            }
                        }
                        free(current_dijkstra_code);
                    }
                    graph_free(g);
                    fclose(f);
                }
                remove(ent->d_name);
            }
        }
        closedir (dir);
    }
    if(highest_score_func[0] != '\0') {

        printf("\nwatermark dijkstra code: %s\n", dijkstra_code);
        printf("best fit function is: '%s'\n", highest_score_func);
        printf("\tscore: %ld\n", highest_score);
        printf("\tentry_point: %ld\n", highest_score_entry_point);
        printf("\tdijkstra code: %s\n", highest_score_code);

        char* code = watermark_generate_code(dijkstra_code);
        printf("\n\nwatermark example code:\n%s", code);
        free(code);
        code = watermark_generate_code(highest_score_code);
        printf("\n\n'%s' code example:\n%s", highest_score_func, code);
        free(code);
    } else {
        printf("\nNo valid candidate function was found. Reason:\n");
        if(file_has_at_least_one_dijkstra_graph) {
            printf("\tthe Dijkstra codes of the functions in the file were too small compared to the watermark dijkstra code\n");
        } else {
            printf("\tnone of the functions inside the file has a Dijkstra graph as CFG\n");
        }
        return 1;
    }
    return 0;
}

uint8_t get_uint8_t(const char* msg) {
    uint8_t n;
    printf("%s", msg);
    scanf("%hhu", &n);
    return n;
}

unsigned long get_ulong(const char* msg) {

    unsigned long n;
    printf("%s", msg);
    scanf("%lu", &n);
    return n;
}

char* get_string(const char* msg) {

    char* s = calloc(MAX_SIZE + 1, sizeof(char));
    printf("%s", msg);
    scanf(" %" MAX_SIZE_STR "s", s);
    return s;
}

int main() {

    printf("1) encode string\n");
    printf("2) encode number\n");
    printf("3) generate graph from dijkstra code\n");
    printf("4) run removal test for bits\n");
    printf("5) run removal test with reed solomon for bits\n");
    printf("6) run removal test for numeric strings\n");
    printf("7) run removal test with reeed solomon for numeric strings\n");
    printf("8) show report matrix\n");
    printf("else) exit\n");
    switch(get_uint8_t("input an option: ")) {
        case 1: {
            char* s = get_string("Input numberic string to encode: ");
            unsigned long data_len=strlen(s);
            void* data = encode_numeric_string(s, &data_len);

            GRAPH* g = watermark_encode(data, data_len);
            char* dijkstra_code = dijkstra_get_code(g);
            graph_write_hamiltonian_dot(g, "dot.dot", dijkstra_code);
            graph_print(g, NULL);
            uint8_t result = ask_for_comparison(dijkstra_code);
            
            free(dijkstra_code);
            graph_free(g);
            free(s);
            free(data);
            return result;
        }
        case 2: {
            unsigned long n = get_ulong("Input number to encode: ");
            invert_byte_sequence((uint8_t*)&n, sizeof(n));
            GRAPH* g = watermark_encode(&n, sizeof(n));
            char* dijkstra_code = dijkstra_get_code(g);
            graph_write_hamiltonian_dot(g, "dot.dot", dijkstra_code);
            graph_print(g, NULL);
            uint8_t result = ask_for_comparison(dijkstra_code);
            
            free(dijkstra_code);
            graph_free(g);

            return result;
        }
        case 3: {
            char* s = get_string("Input dijkstra code: ");
            GRAPH* g = dijkstra_generate(s);
            if(!g) {
                printf("This isn't a valid dijkstra code!\n");
                free(s);
                break;
            }
            graph_print(g, NULL);
            graph_write_dot(g, "dot.dot", s);
            ask_for_comparison(s);
            graph_free(g);
            free(s);
            break;
        }/*
        case 4: {
            printf("input maximum number of removals: ");
            unsigned long n_removals;
            scanf("%lu", &n_removals);
            printf("input maximum number of bits: ");
            unsigned long n_symbols;
            scanf("%lu", &n_symbols);
            removal_attack_for_bits(n_removals, n_symbols);
            break;
        }
        case 5: {
            printf("input maximum number of removals: ");
            unsigned long n_removals;
            scanf("%lu", &n_removals);
            printf("input maximum number of bits: ");
            unsigned long n_symbols;
            scanf("%lu", &n_symbols);
            removal_attack_with_rs_for_bits(n_removals, n_symbols);
            break;
        }
        case 6: {
            printf("input maximum number of removals: ");
            unsigned long n_removals;
            scanf("%lu", &n_removals);
            printf("input maximum number of chars: ");
            unsigned long n_symbols;
            scanf("%lu", &n_symbols);
            removal_attack_for_symbols(n_removals, n_symbols);
            break;
        }
        case 7: {
            printf("input maximum number of removals: ");
            unsigned long n_removals;
            scanf("%lu", &n_removals);
            printf("input maximum number of chars: ");
            unsigned long n_symbols;
            scanf("%lu", &n_symbols);
            removal_attack_with_rs_for_symbols(n_removals, n_symbols);
        }
        case 8: {
            show_report_matrix();
        }*/
    }

    return 0;
}
