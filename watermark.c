#include "decoder/decoder.h"
#include "encoder/encoder.h"
#include "code_generation/code_generation.h"
#include "sequence_alignment/sequence_alignment.h"
#include "dijkstra/dijkstra.h"
#include <dirent.h>
#include <math.h>

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
#define MIN(a, b) a < b ? a : b

#define show_bits(bits,len) fprintf(stderr, "%s:%d:" #bits ":", __FILE__, __LINE__);\
  for(unsigned long i = 0; i < len; i++) {\
    fprintf(stderr, "%hhu", get_bit(bits, i));\
  }\
  fprintf(stderr, "\n");

typedef struct CONN_ARR {
  unsigned long len;
  CONNECTION** arr;
} CONN_ARR;

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

unsigned long _check_rs(void* result, void* identifier, unsigned long n_bits) {

  // unsigned long bit_idx_identifier = get_first_positive_bit_index(identifier, identifier_bits);
  // unsigned long bit_idx_result = get_first_positive_bit_index(result, result_bits);
  long bit_idx_identifier = 0;
  long bit_idx_result = 0;

  unsigned long errors = 0;

  while(bit_idx_identifier < (long)n_bits && bit_idx_result < (long)n_bits) {

    if( get_bit(identifier, bit_idx_identifier) != get_bit(result, bit_idx_result)) {
      errors++;
    }
    bit_idx_result++;
    bit_idx_identifier++;
  }
  show_bits((void*)identifier, n_bits);
  show_bits((void*)result, n_bits);
  return errors + abs((int)n_bits - (int)n_bits);
}

CONN_ARR* conn_arr_create(CONN_ARR* arr, CONNECTION* conn) {

  if(!arr) { 
    arr = malloc(sizeof(CONN_ARR));
    memset(arr, 0x00, sizeof(CONN_ARR));
  }
  arr->len += 1;
  arr->arr = realloc(arr->arr, arr->len * sizeof(CONNECTION*));
  arr->arr[arr->len - 1] = conn;
  return arr;
}

CONN_ARR* conn_arr_copy(CONN_ARR* arr) {
  CONN_ARR* new = malloc(sizeof(CONN_ARR));
  new->len = arr->len;
  new->arr = malloc(arr->len * sizeof(CONNECTION*));
  memcpy(new->arr, arr->arr, sizeof(CONNECTION*) * new->len);
  return new;
}

void conn_arr_free(CONN_ARR* arr) {
  free(arr->arr);
  arr->len = 0;
  free(arr);
}

typedef enum METHOD {
  ORIGINAL,
  IMPROVED,
  IMPROVED_WITH_RS
} METHOD;

typedef struct ATTACK {
  METHOD method;
  void* identifier;
  unsigned long n_removals;
  GRAPH* graph;
  unsigned long identifier_len;
  union {
    struct {
      unsigned long n_bits;
    } no_rs;
    struct { 
      unsigned long n_parity_symbols;
      unsigned long n_data_symbols;
      unsigned long symsize;
    } rs;
  } info;
} ATTACK;

unsigned long _test_with_removed_connections(
        ATTACK* attack,
        CONNECTION** conns) {
    if(!conns) return 0;

#ifdef DEBUG
    uint8_t has_forward_removal = 0;
#endif

    // get copy and remove some of its connections
    GRAPH* copy = graph_copy(attack->graph);
    for(unsigned long i = 0; i < attack->n_removals; i++) {
      if(!graph_oriented_disconnect(copy->nodes[conns[i]->parent->graph_idx], copy->nodes[conns[i]->node->graph_idx])) {
        fprintf(stderr, "TEST ERROR: invalid edge removal requested\n");
        exit(EXIT_FAILURE);
      }

#ifdef DEBUG
      if(conns[i]->parent->graph_idx > conns[i]->node->graph_idx) {
        printf("removed \x1b[31mbackedge\x1b[0m from %lu to %lu\n", conns[i]->parent->graph_idx, conns[i]->node->graph_idx);
      } else if(conns[i]->parent->graph_idx + 2 == conns[i]->node->graph_idx ) {
        has_forward_removal = 1;
        printf("removed \x1b[92mforward edge\x1b[0m from %lu to %lu\n", conns[i]->parent->graph_idx, conns[i]->node->graph_idx);
      } else {
        fprintf(stderr, "TEST ERROR: hamiltonian edge removed\n");
        graph_write_hamiltonian_dot(attack->graph, "original.dot", NULL);
        graph_write_hamiltonian_dot(copy, "copy.dot", NULL);
        exit(EXIT_FAILURE);
      }
#endif
    }

    unsigned long num_bytes = 0;
    void* result = NULL;
    switch(attack->method) {
      case ORIGINAL:
        num_bytes = attack->identifier_len;
        result = watermark_decode(copy, &num_bytes);
        break;
      case IMPROVED:
        num_bytes = attack->identifier_len;
        result = watermark_decode_improved8(copy, attack->identifier, &num_bytes);
        break;
      case IMPROVED_WITH_RS:
        num_bytes = attack->info.rs.n_data_symbols;
        result = watermark_rs_decode_improved(attack->graph, attack->identifier, &num_bytes, attack->info.rs.n_parity_symbols, attack->info.rs.symsize);
        if(!result) {
          free(result);
          graph_free(copy);
          return attack->info.rs.n_data_symbols * attack->info.rs.symsize;
        }
        break;
    }

    unsigned long errors = attack->method == IMPROVED_WITH_RS ? _check_rs(result, attack->identifier, attack->info.rs.symsize * attack->info.rs.n_data_symbols)
    : _check(result, attack->identifier, num_bytes, attack->identifier_len) ;
#ifdef DEBUG
    if(errors > 1 || (( has_forward_removal && errors == 1 )&&0)) {
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

uint8_t is_hamiltonian(CONNECTION* conn) {
  return conn->parent->graph_idx + 1 == conn->node->graph_idx;
}

CONNECTION* conn_next(CONNECTION* conn) {
  if(conn->next) return conn->next;
  // iterate over nodes
  for(NODE* next_node = graph_get(conn->parent->graph, conn->parent->graph_idx + 1); next_node; next_node = graph_get(next_node->graph, next_node->graph_idx + 1)) {
    if(next_node->out) return next_node->out;
  }
  return NULL;
}

CONNECTION* conn_next_non_hamiltonian_edge(CONNECTION* conn) {
  do {
    conn = conn_next(conn);
  } while(conn && is_hamiltonian(conn));
  return conn;
}

CONN_ARR* get_list_of_non_hamiltonian_edges(GRAPH* graph) {
 
  CONN_ARR* arr = NULL;
  CONNECTION* conn = conn_next_non_hamiltonian_edge(graph->nodes[0]->out);
  for(; conn; conn = conn_next_non_hamiltonian_edge(conn)) {
    arr = conn_arr_create(arr, conn);
  }
  if(!arr) return NULL;
#ifdef DEBUG
  for(unsigned long i = 0 ; i < arr->len; i++) {
    CONNECTION* conn = arr->arr[i];
    if( conn->parent->graph_idx + 1 == conn->node->graph_idx) {
      fprintf(stderr, "TEST ERROR: hamiltonian edge added to removal combination array\n");
      exit(EXIT_FAILURE);
    }
  }
#endif
  return arr;
}

void** stack_combinations(void** comb1, unsigned long comb1len, void** comb2, unsigned long comb2len) {

  unsigned long nrows = comb1len + comb2len;
  void** combinations = calloc(nrows, sizeof(void*));
  for(unsigned long i = 0; i < comb1len; i++) {
    combinations[i] = comb1[i];
  }
  for(unsigned long i = comb1len; i < nrows; i++) {
    combinations[i] = comb2[i - comb1len];
  }
  free(comb1);
  free(comb2);
  return combinations;
}

void** get_list_of_combinations(void* arr, unsigned long* len, unsigned long element_size, unsigned long k) {
  if(!arr || *len < k || !k) return NULL;
  // for k = 1 is just every element in a different row
  if(k == 1) {
    void** combinations = malloc(sizeof(void*) * (*len));
    for(unsigned long i = 0; i < *len; i++) {
      combinations[i] = malloc(element_size);
      memcpy(combinations[i], (uint8_t*)arr + (element_size * i), element_size);
    }
    return combinations;
  }
  unsigned long arr_len = *len;
  void** combinations = NULL;
  unsigned long combinations_len = 0;
  for(unsigned long i = 0; i < arr_len; i++) {
    unsigned long n_comb = *len - i - 1;
    void** sub_combinations = get_list_of_combinations(((uint8_t*)arr) + (element_size * (i+1)), &n_comb, element_size, k-1);
    if(!sub_combinations || !n_comb) continue;
    // append current element to the sub_combinations (each row is in reverse order)
    for(unsigned long j = 0; j < n_comb; j++) {
      sub_combinations[i] = realloc(sub_combinations[i], element_size * k);
      memcpy(((uint8_t*)sub_combinations[i]) + (element_size * k), ((uint8_t*)arr) + ( element_size * i), element_size);
    }
    // stack combinations
    combinations = stack_combinations(combinations, combinations_len, sub_combinations, n_comb);
    combinations_len += n_comb;
  }
  *len = combinations_len;
  return combinations;
}

void remove_combinations(void** combinations, unsigned long len) {
  for(unsigned long i = 0; i < len; i++) {
    free(combinations[i]);
  }
  free(combinations);
}

void _multiple_removal_test(
    ATTACK* attack,
    STATISTICS* statistics,
    CONNECTION*** combinations,
    unsigned long len) {

  statistics->total+=len;
  // iterate through combinations and attack from them
  for(unsigned long i = 0; i < len; i++) {
    statistics->errors = _test_with_removed_connections(attack, combinations[i]);
    if(statistics->errors > statistics->worst_case) statistics->worst_case = statistics->errors;
  }
}

unsigned long fac(unsigned long n) {
  if(n == 1 || !n) return 1;
  return n * fac(n-1);
}

void multiple_removal_test(ATTACK* attack, STATISTICS* statistics) {
    CONN_ARR* arr = get_list_of_non_hamiltonian_edges(attack->graph);
    if(!arr) {
      _multiple_removal_test(attack, statistics, NULL, 0);
      return;
    }
    unsigned long combinations_len = arr->len;
    CONNECTION*** combinations = (CONNECTION***)get_list_of_combinations(arr->arr, &combinations_len, sizeof(CONNECTION*), MIN(arr->len, attack->n_removals));
    _multiple_removal_test(attack, statistics, combinations, combinations_len);
    conn_arr_free(arr);
    remove_combinations((void**)combinations, combinations_len);
}

uint8_t key_is_non_zero(unsigned long key, unsigned long n_data_symbols, unsigned long symsize) {
  unsigned long n_bits = n_data_symbols * symsize;
  for(unsigned long i = 0; i < n_bits; i++) {
    if(get_bit((void*)&key, i)) return 1;
  }
  return 0;
}

void get_key_from_k(unsigned long** k, unsigned long symsize, unsigned long num_data_symbols) {
  // for(unsigned long i = 0; i < sizeof(*k); i++) {
  //   invert_binary_sequence(&((uint8_t*)k)[i], 1);
  // }
  **k = invert_unsigned_long(**k);
  // show_bits((void*)*k, sizeof(**k) * 8 );
  unsigned long total_n_bits = sizeof(**k) * 8;
  unsigned long data_bits = num_data_symbols * symsize;
  unsigned long left_zeros = get_number_of_left_zeros((void*)*k, sizeof(**k));
  long zeros_in_symbol = ( data_bits + left_zeros ) - total_n_bits;
  unsigned long num_bytes = sizeof(*k);
  // fprintf(stderr, "zeros_in_symbol: %ld, left_zeros: %lu, total_n_bits: %lu, data_bits: %lu, k: %lu\n", zeros_in_symbol, left_zeros, total_n_bits, data_bits, **k);
  remove_left_zeros((void*)*k, &num_bytes);
  // show_bits((void*)*k, data_bits );
  add_left_zeros((uint8_t**)k, &num_bytes, zeros_in_symbol);
  // show_bits((void*)*k, data_bits );
}

void attack(METHOD method, unsigned long n_removal, unsigned long n_bits, unsigned long n_parity_symbols, unsigned long symsize) {

    unsigned long matrix_size = method == IMPROVED_WITH_RS ? n_bits * symsize + 1 : n_bits + 1;
    unsigned long matrix[matrix_size][matrix_size];
    memset(matrix, 0x00, sizeof(unsigned long) * matrix_size * matrix_size);
  #if defined(_OPENMP)
    omp_set_num_threads(4);
    printf("openmp is being used.\n");
  #else
    printf("openmp is not being used.\n");
  #endif

    unsigned long identifier_len = sizeof(unsigned long);
    for(unsigned long current_n_bits = 1; current_n_bits <= n_bits; current_n_bits++) {

        printf("\tnumber of bits: %lu", current_n_bits);
        #if defined(_OPENMP)
          double start = omp_get_wtime();
        #else
          clock_t start = clock();
        #endif

        unsigned long lower_bound = get_lower_bound(current_n_bits);
        unsigned long upper_bound = get_upper_bound(current_n_bits);

        if(method == IMPROVED_WITH_RS) {
          lower_bound = 1 << ((current_n_bits-1) * symsize);
          upper_bound = 1 << (current_n_bits * symsize);    
        }
        fprintf(stderr,"lower bound: %lu, upper bound: %lu\n", lower_bound, upper_bound);

        #if defined(_OPENMP)
          #pragma omp parallel for schedule(dynamic)
        #endif
        for(unsigned long i = lower_bound; i < upper_bound; i++) {

            STATISTICS statistics = {0};

            unsigned long* identifier = malloc(sizeof(unsigned long));
            memset(identifier, 0x00, sizeof(unsigned long));
            if(method == IMPROVED_WITH_RS) {
              *identifier = i;
              get_key_from_k(&identifier, symsize, current_n_bits);
            } else {
              *identifier = invert_unsigned_long(i);
            }
            // if(!key_is_non_zero(*identifier, current_n_bits, symsize)) {
            //   fprintf(stderr, "i: %lu\n", i);
            //   show_bits((void*)&i, current_n_bits * symsize);
            //   fprintf(stderr, "identifier: %lu\n", *identifier);
            //   show_bits((void*)identifier, current_n_bits * symsize);
            //   exit(EXIT_FAILURE);
            // }

            GRAPH* graph = NULL;
            if(method == IMPROVED_WITH_RS) {
              graph = watermark_rs_encode(identifier, current_n_bits, n_parity_symbols, symsize); 
            } else {
              graph = watermark_encode8(identifier, identifier_len);
            }
            ATTACK attack = {0};
            attack.n_removals = n_removal;
            attack.graph = graph;
            attack.identifier = identifier;
            attack.identifier_len = identifier_len;
            attack.method = method;
            if(method == IMPROVED_WITH_RS) {
              attack.info.rs.n_parity_symbols = n_parity_symbols;
              attack.info.rs.symsize = symsize;
              attack.info.rs.n_data_symbols = current_n_bits;
            } else {
              attack.info.no_rs.n_bits = current_n_bits;
            }

            multiple_removal_test(&attack, &statistics);
            graph_free(graph);
            #if defined(_OPENMP) 
              #pragma omp critical
            #endif
            matrix[statistics.worst_case][current_n_bits-1]++;
            free(identifier);
        }
        #if defined(_OPENMP)
          double duration = omp_get_wtime() - start;
          printf(" - %F secs\n", duration);
        #else
          clock_t duration = clock() - start;
          printf(" - %F secs\n", duration / (double) CLOCKS_PER_SEC);
        #endif
    }
    write_to_report_matrix((unsigned long*)&matrix, matrix_size, matrix_size);
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

uint8_t* get_binary_sequence(const char* msg, unsigned long* n_bits, unsigned long* num_bytes) {
    printf("%s", msg);
    char bin_char[MAX_SIZE+1]={0};
    int res = scanf(" %" MAX_SIZE_STR "[01]", bin_char);
    if(res != 1) return NULL;
    *n_bits = strlen(bin_char);
    uint8_t bin_u8[*n_bits];
    for(unsigned int i = 0; i < *n_bits; i++) {
      bin_u8[i] = bin_char[i] - '0';
    }
    return get_sequence_from_bit_arr(bin_u8, *n_bits, num_bytes);
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
            GRAPH* g = watermark_rs_encode(&n, sizeof(n), n_parity, 0);
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
            attack(ORIGINAL, n_removals, n_bits, 0, 0);
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
            attack(IMPROVED, n_removals, n_bits, 0, 0);
            show_report_matrix();
            break;
        }
        case 7: {
            printf("input number of removals: ");
            unsigned long n_removals;
            scanf("%lu", &n_removals);
            printf("input maximum number of message symbols: ");
            unsigned long n_symbols;
            scanf("%lu", &n_symbols);
            printf("input number of parity symbols: ");
            unsigned long n_parity;
            scanf("%lu", &n_parity);
            printf("input symbol size (1-16): ");
            unsigned long symsize;
            scanf("%lu", &symsize);
            attack(IMPROVED_WITH_RS, n_removals, n_symbols, n_parity, symsize);
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
          unmerge_arr(data, num_data_symbols, symbol_bytes, symbol_size, (void**)&unmerged_data);
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
