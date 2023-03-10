#include "decoder/decoder.h"
#include "encoder/encoder.h"
#include "code_generation/code_generation.h"
#include "sequence_alignment/sequence_alignment.h"
#include "dijkstra/dijkstra.h"
#include <dirent.h>
#if defined(_OPENMP)
  #include <omp.h>
#else
  #include <time.h>
#endif

#define MAX_SIZE 4095
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define MAX_SIZE_STR STRINGIFY(MAX_SIZE)

#define SIZE_PERCENTAGE 0.8

#define MATCH 1
#define MISMATCH -2
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

void show_report_matrix(void) {

    FILE* file = fopen("tests/report_matrix.plt", "rb");
    unsigned long n_rows, n_columns;
    fread(&n_rows, sizeof(unsigned long), 1, file);
    fread(&n_columns, sizeof(unsigned long), 1, file);

    unsigned long totals[n_rows];
    memset(totals, 0x00, sizeof(totals));

    printf("lost bits\\bits in key\n");
    printf("        ");
    for(unsigned long i = 0; i < n_columns; i++) {
        printf("%-5lu", i+1);
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

unsigned long _check(void* result, void* identifier, unsigned long result_len, unsigned long identifier_len) {

  unsigned long bit_idx_identifier = get_first_positive_bit_index(identifier, identifier_len);
  unsigned long bit_idx_result = get_first_positive_bit_index(result, result_len);

  unsigned long total_num_bits_identifier = identifier_len * 8;
  unsigned long total_num_bits_result = result_len * 8;

  unsigned long errors = 0;

  while(bit_idx_identifier < total_num_bits_identifier && bit_idx_result < total_num_bits_result) {

    if( get_bit(identifier, bit_idx_identifier) != get_bit(result, bit_idx_result)) {
      errors++;
    }
    bit_idx_result++;
    bit_idx_identifier++;
  }

  return errors;
}

typedef enum METHOD {
  ORIGINAL,
  IMPROVED,
  IMPROVED_WITH_RS
} METHOD;

typedef struct ATTACK {
  METHOD method;
  void* identifier;
  unsigned long identifier_len;
  GRAPH* graph;
  unsigned long n_removals;
  unsigned long n_bits;
  unsigned long n_parity_symbols;
} ATTACK;

unsigned long _test_with_removed_connections(
        ATTACK* attack,
        CONN_LIST* conns) {

    // get copy and remove some of its connections
    GRAPH* copy = graph_copy(attack->graph);
#ifdef DEBUG
    uint8_t has_forward_removal = 0;
#endif
    for(CONN_LIST* l = conns; l; l = l->next) {
      graph_oriented_disconnect(copy->nodes[l->conn->parent->graph_idx], copy->nodes[l->conn->node->graph_idx]);
#ifdef DEBUG
      if(l->conn->parent->graph_idx > l->conn->node->graph_idx) {
        printf("removed \x1b[31mbackedge\x1b[0m from %lu to %lu\n", l->conn->parent->graph_idx, l->conn->node->graph_idx);
      } else {
        has_forward_removal = 1;
        printf("removed \x1b[92mforward edge\x1b[0m from %lu to %lu\n", l->conn->parent->graph_idx, l->conn->node->graph_idx);
      }
#endif
    }

    unsigned long num_bytes = attack->identifier_len;
    void* result = NULL;
    switch(attack->method) {
      case ORIGINAL:
        result = watermark_decode(copy, &num_bytes);
        break;
      case IMPROVED:
        result = watermark_decode_improved(copy, attack->identifier, &num_bytes);
        break;
      case IMPROVED_WITH_RS:
        result = watermark_rs_decode_improved(attack->graph, attack->identifier, &num_bytes, attack->n_parity_symbols);
        break;
    }

    unsigned long errors = _check(result, attack->identifier, num_bytes, attack->identifier_len);
#ifdef DEBUG
    if(errors > 1 || ( has_forward_removal && errors == 1 )) {
      printf("errors: %lu\n", errors);
      printf("identifier: ");
      for(unsigned long i = get_first_positive_bit_index(attack->identifier, attack->identifier_len); i < attack->identifier_len * 8; i++) {
        printf("%hhu", get_bit(attack->identifier, i));
      }
      printf("\n");
      graph_print(attack->graph, NULL);
      printf("result: ");
      for(unsigned long i = get_first_positive_bit_index(result, num_bytes); i < num_bytes * 8; i++) {
        printf("%hhu", get_bit(result, i));
      }
      printf("\n");
      graph_print(copy, NULL);
    } else {
      for(unsigned long i = get_first_positive_bit_index(result, num_bytes); i < num_bytes * 8; i++) {
        printf("%hhu", get_bit(result, i));
      }
      printf(": %lu errors \x1b[32m✓\x1b[0m\n", errors);
      if(errors) graph_print(copy, NULL);
    }
#endif

    free(result);
    graph_free(copy);

    return errors;
}

void _multiple_removal_test(
    ATTACK* attack,
    STATISTICS* statistics,
    CONN_LIST* non_hamiltonian_edges,
    NODE* node,
    CONNECTION* conn) {

    if(!attack->n_removals) {

        statistics->total++;
        statistics->errors = _test_with_removed_connections(attack, non_hamiltonian_edges);
        if(statistics->errors > statistics->worst_case) statistics->worst_case = statistics->errors;
    } else {

        NODE* initial_node = node;

        for(; node; node = graph_get(attack->graph, node->graph_idx+1)) {

            if(node->out) {

                // if this is a hamiltonian edge, skip it
                CONNECTION* c = ( node->out->node == graph_get(attack->graph, node->graph_idx+1) ? node->out->next : node->out );
                // if this is the first node in the loop, start from the connection given by 'conn'
                if(node == initial_node) c = (conn ? conn : c);
                 
                for(; c; c = c->next) {

                    if(c->parent->graph_idx + 1 == c->node->graph_idx ) continue;
                    CONN_LIST* list = conn_list_create(non_hamiltonian_edges, c);
                    ATTACK another_attack;
                    memcpy(&another_attack, attack, sizeof(ATTACK));
                    another_attack.n_removals -= 1;
                    _multiple_removal_test(
                        &another_attack,
                        statistics,
                        list,
                        node,
                        c->next);

                    conn_list_free(list);
                }
            }

        }
    }
}

void multiple_removal_test(ATTACK* attack, STATISTICS* statistics) {

    _multiple_removal_test(attack, statistics, NULL, attack->graph->nodes[0], NULL);
}

void attack(METHOD method, unsigned long n_removal, unsigned long n_bits, unsigned long n_parity_symbols) {

    unsigned long matrix[n_bits][n_bits];
    memset(matrix, 0x00, sizeof(matrix));
  #if defined(_OPENMP)
    omp_set_num_threads(4);
    printf("openmp is being used.\n");
  #else
    printf("openmp is not being used.\n");
  #endif

    for(unsigned long current_n_bits = 1; current_n_bits <= n_bits; current_n_bits++) {

        printf("\tnumber of bits: %lu", current_n_bits);
        #if defined(_OPENMP)
          double start = omp_get_wtime();
        #else
          clock_t start = clock();
        #endif
        unsigned long lower_bound = get_lower_bound(current_n_bits);
        unsigned long upper_bound = get_upper_bound(current_n_bits);

        #if defined(_OPENMP)
          #pragma omp parallel for schedule(dynamic)
        #endif
        for(unsigned long i = lower_bound; i < upper_bound; i++) {

            STATISTICS statistics = {0};

            unsigned long identifier = invert_unsigned_long(i);
            unsigned long identifier_len = sizeof(unsigned long);

            GRAPH* graph = NULL;
            if(method == IMPROVED_WITH_RS) {
                graph = watermark_rs_encode(&identifier, identifier_len, n_parity_symbols); 
            } else {
              graph = watermark_encode(&identifier, identifier_len);
            }

            ATTACK attack = {
              .n_removals = n_removal,
              .n_parity_symbols = n_parity_symbols,
              .n_bits = current_n_bits,
              .graph = graph,
              .identifier = &identifier,
              .identifier_len = identifier_len,
              .method = method,
            };

            multiple_removal_test(&attack, &statistics);

            graph_free(graph);

            #pragma omp critical
            { 
              matrix[statistics.worst_case][current_n_bits-1]++;
            }
        }
        #if defined(_OPENMP)
          double duration = omp_get_wtime() - start;
          printf(" - %F secs\n", duration);
        #else
          clock_t duration = clock() - start;
          printf(" - %F secs\n", duration / (double) CLOCKS_PER_SEC);
        #endif
    }
    write_to_report_matrix((unsigned long*)&matrix, n_bits, n_bits);
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

                            printf("%s:\n\tcode: %s\n\tscore: %ld\n\tentry_point: %ld\n", ent->d_name, current_dijkstra_code, score, entry_point);
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

uint8_t* get_binary_sequence(const char* msg, unsigned long* n_bits, unsigned long* num_bytes) {
    printf("%s", msg);
    char bin_char[MAX_SIZE+1]={0};
    int res = scanf(" %" MAX_SIZE_STR "[01]", bin_char);
    if(res != 1) return NULL;
    *n_bits = strlen(bin_char);
    *num_bytes = (*n_bits / 8) + !!(*n_bits % 8);
    uint8_t* bin_u8 = malloc(*num_bytes);
    memset(bin_u8, 0x00, *num_bytes);
    for(unsigned int i = 0; i < *n_bits; i++) {
      set_bit(bin_u8, i, bin_char[i] - '0');
    }
    return bin_u8;
}

int main(void) {

    printf("1) encode string\n");
    printf("2) encode number\n");
    printf("3) encode number with reed solomon\n");
    printf("4) generate graph from dijkstra code\n");
    printf("5) run removal test\n");
    printf("6) run removal test with improved decoding\n");
    printf("7) run removal test with improved decoding and reed solomon\n");
    printf("8) reed solomon encode\n");
    printf("9) reed solomon decode\n");
    printf("10) show report matrix\n");
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
            unsigned long n = get_ulong("input number to encode: ");
            unsigned long n_parity = get_ulong("number of parity symbols: ");
            invert_byte_sequence((uint8_t*)&n, sizeof(n));
            GRAPH* g = watermark_rs_encode(&n, sizeof(n), n_parity);
            char* dijkstra_code = dijkstra_get_code(g);
            graph_write_hamiltonian_dot(g, "dot.dot", dijkstra_code);
            graph_print(g, NULL);
            uint8_t result = ask_for_comparison(dijkstra_code);
            
            free(dijkstra_code);
            graph_free(g);

            return result;
        }
        case 4: {
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
        }
        case 5: {
            printf("input number of removals: ");
            unsigned long n_removals;
            scanf("%lu", &n_removals);
            printf("input maximum number of bits: ");
            unsigned long n_bits;
            scanf("%lu", &n_bits);
            attack(ORIGINAL, n_removals, n_bits, 0);
            show_report_matrix();
            break;
        }
        case 6: {
            printf("input number of removals: ");
            unsigned long n_removals;
            scanf("%lu", &n_removals);
            printf("input maximum number of bits: ");
            unsigned long n_bits;
            scanf("%lu", &n_bits);
            attack(IMPROVED, n_removals, n_bits, 0);
            show_report_matrix();
            break;
        }
        case 7: {
            printf("input number of removals: ");
            unsigned long n_removals;
            scanf("%lu", &n_removals);
            printf("input maximum number of message symbols (8 bit each): ");
            unsigned long n_symbols;
            scanf("%lu", &n_symbols);
            printf("input number of parity symbols (8 bit each): ");
            unsigned long n_parity;
            scanf("%lu", &n_parity);
            attack(IMPROVED_WITH_RS, n_removals, n_symbols * 8, n_parity);
            show_report_matrix();
            break;           
        }
        case 8: {
          unsigned long n_bits, num_bytes;
          uint8_t* data = get_binary_sequence("input message as binary sequence: ", &n_bits, &num_bytes);

          unsigned long n_parity_symbols;
          printf("input number of parity symbols: ");
          scanf("%lu", &n_parity_symbols);

          int symbol_size;
          printf("input symbol size (1-16): ");
          scanf("%d", &symbol_size);

          unsigned long symbol_bytes = (symbol_size / 8) + !!(symbol_size % 8);
          unsigned long num_data_symbols = (n_bits/symbol_size) + !!(n_bits % symbol_size);
          uint8_t* unmerged_data = NULL;
          printf("num_data_symbols: %lu, symbol_bytes: %lu, symbol_size: %d\n", num_data_symbols, symbol_bytes, symbol_size);
          unmerge_arr(data, &num_data_symbols, symbol_bytes, symbol_size, (void**)&unmerged_data);
          uint16_t parity[n_parity_symbols];
          memset(parity, 0x00, sizeof(uint16_t) * n_parity_symbols);
          rs_encode(unmerged_data, num_data_symbols, parity, n_parity_symbols, symbol_size);
          for(unsigned long i = 0; i < num_data_symbols; i++) {
            for(unsigned long j = symbol_bytes*8 - symbol_size; j < symbol_bytes*8; j++) {
              printf("%hhu", get_bit(&unmerged_data[i], j));
            }
            printf(" ");
          }
          printf("| ");
          for(unsigned long i = 0; i < n_parity_symbols; i++) {
            for(unsigned long j = symbol_bytes*8 - symbol_size; j < symbol_bytes*8; j++) {
              printf("%hhu", get_bit((uint8_t*)&parity[i], j));
            }
          }
          printf("\n");

          printf("\n");
          free(unmerged_data);
          free(data);
          break;
        }
        case 9: {
          
          break;
        }
        case 10: {
            show_report_matrix();
        }
    }

    return 0;
}
