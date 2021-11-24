#ifndef CODE_GENERATION_H
#define CODE_GENERATION_H

#include "graph/graph.h"

#define MAX_DYN_VAR_ID_LEN 10

typedef struct CODE {

    char* str;
    unsigned long size;
} CODE;

typedef struct  GEN_TABLE {

    const char** table;
    unsigned long len;
    unsigned long next;
} GEN_TABLE;

#define ARRAY_SIZE(arr) sizeof(arr)/sizeof(arr[0])

#define GEN_TABLE_CREATE(...) {\
    .table = (const char*[]){__VA_ARGS__},\
    .len = ARRAY_SIZE( ( (const char*[]){__VA_ARGS__}) ),\
    .next = 0\
}

typedef struct CODE_GEN {

    GEN_TABLE identifiers;
    GEN_TABLE values;
    GEN_TABLE comparisons;
    GEN_TABLE operations;
    GEN_TABLE datatypes;


} CODE_GEN;

#define CODE_GEN_CREATE {\
    .identifiers = GEN_TABLE_CREATE("x", "y", "ptr", "tmp", "idx", "n", "i", "len", "a", "b", "c", "var"),\
    .values = GEN_TABLE_CREATE("0b1010000", "'a'", "0x00", "1", "0", "4096", "~1", "'z'"),\
    .comparisons = GEN_TABLE_CREATE(" > ", " < ", " == ", " != ", " <= ", " >= ", " ^ ", " & "),\
    .operations = GEN_TABLE_CREATE("*", "/", "+", "-", "|", "&", "^"),\
    .datatypes = GEN_TABLE_CREATE("int ", "unsigned long ", "unsigned int ", "short ", "long ", "char ")\
}

CODE* code_from(char*);
CODE* code_append(CODE*, CODE*);
CODE* code_append_str(CODE*, char*);
CODE* code_prefix_with_tabs(CODE*, unsigned long tabs);
void code_free(CODE*);

CODE* code_gen_new_identifier(CODE_GEN*);
CODE* code_gen_get_identifier(CODE_GEN*);
const char* code_gen_get_value(CODE_GEN*);
const char* code_gen_get_comparison(CODE_GEN*);
const char* code_gen_get_operation(CODE_GEN*);
const char* code_gen_get_datatype(CODE_GEN*);

CODE* code_gen_prefix_with_tabs(const char*, unsigned long tabs);

CODE* code_gen_switch_header(CODE_GEN*, unsigned long tabs);
CODE* code_gen_switch_case_header(unsigned long tabs);
CODE* code_gen_if_header(CODE_GEN*, unsigned long tabs);
CODE* code_gen_while_header(CODE_GEN*, unsigned long tabs);
CODE* code_gen_repeat_header(CODE_GEN*, unsigned long tabs);

CODE* code_declarations(CODE_GEN* code_gen);

char* watermark_generate_code(char* dijkstra_code);

#endif
