#include "code_generation/code_generation.h"

CODE* code_from(char* str) {

    CODE* code = malloc(sizeof(CODE));
    code->size = strlen(str);
    code->str = strcpy(malloc(code->size + 1), str);
    return code;
}

CODE* code_append(CODE* to, CODE* from) {

    to->str = realloc(to->str, to->size+from->size+1);
    to->str = strncat(to->str, from->str, from->size);
    //for(unsigned long i = 0; i < from->size; i++) {
    //    to->str[to->size+i] = from->str[i];
    //}
    to->size += from->size;
    code_free(from);
    return to;
}

CODE* code_append_str(CODE* to, char* str) {

    return code_append(to, code_from(str));
}

CODE* code_prefix_with_tabs(CODE*  code, unsigned long tabs) {

    unsigned long str_len = code->size;
    code->size += tabs;
    code->str = realloc(code->str, code->size + 1);
    code->str[code->size] = '\0';
    // move string to the right
    memmove(code->str+tabs, code->str, str_len);
    // write tabs
    memset(code->str, '\t', tabs);
    return code;
}

void code_free(CODE* code) {
    free(code->str);
    free(code);
}

const char* code_gen_table_new(GEN_TABLE* table) {
    // if we already consumed all elements on this table, return NULL
    return table->next < table->len ? table->table[table->next++] : NULL;
}

const char* code_gen_repeatable_new(GEN_TABLE* table) {
    // if we already consumed all elements in this table, reset the counter and return the first element
    return table->next < table->len ? table->table[table->next++] : table->table[(table->next=1)-1];
}

const char* code_gen_try_old(GEN_TABLE* table) {
    // if we didn't consume any item from the table before, get the first one
    return table->next ? table->table[rand() % table->next] : table->table[(table->next=1)-1];
}

CODE* code_gen_new_identifier(CODE_GEN* code_gen) {
    char* res = (char*)code_gen_repeatable_new(&code_gen->identifiers);
    if(!res) {
        char id[MAX_DYN_VAR_ID_LEN + 1];
        sprintf(id, "%s%lu", code_gen->identifiers.table[0], code_gen->identifiers.next - code_gen->identifiers.len);
        code_gen->identifiers.next++;
        res = id;
    }
    return code_from(res);
}

CODE* code_gen_get_identifier(CODE_GEN* code_gen) {

    if(rand() & 1) {
        return code_gen_new_identifier(code_gen);
    } else {
        return code_from((char*)code_gen_try_old(&code_gen->identifiers));
    }
}

const char* code_gen_get_value(CODE_GEN* code_gen) {
    return rand() & 1 ? code_gen_try_old(&code_gen->values) : code_gen_repeatable_new(&code_gen->values);
}
const char* code_gen_get_comparison(CODE_GEN* code_gen) {
    return rand() & 1 ? code_gen_try_old(&code_gen->comparisons) : code_gen_repeatable_new(&code_gen->comparisons);
}
const char* code_gen_get_operation(CODE_GEN* code_gen) {
    return rand() & 1 ? code_gen_try_old(&code_gen->operations) : code_gen_repeatable_new(&code_gen->operations);
}
const char* code_gen_get_datatype(CODE_GEN* code_gen) {
    return rand() & 1 ? code_gen_try_old(&code_gen->datatypes) : code_gen_repeatable_new(&code_gen->datatypes);
}

CODE* code_gen_value_or_variable(CODE_GEN* code_gen) {

    if(rand() & 1) {
        return code_from((char*)code_gen_get_value(code_gen));
    } else {
        return code_from((char*)code_gen_try_old(&code_gen->identifiers));
    }
}

CODE* code_gen_condition(CODE_GEN* code_gen) {

    CODE* identifier = code_gen_get_identifier(code_gen);
    code_append_str(identifier, (char*)code_gen_get_comparison(code_gen));
    code_append(identifier, code_gen_value_or_variable(code_gen));
    return identifier;
}

CODE* code_gen_expression(CODE_GEN* code_gen) {

    CODE* identifier = code_gen_get_identifier(code_gen);
    // shorthand operator or not
    if(rand() & 1) {

        code_append_str(identifier, " = ");
        code_append(identifier, code_gen_value_or_variable(code_gen));
    } else {
        code_append_str(identifier, " ");
        code_append_str(identifier, (char*)code_gen_get_operation(code_gen));
        code_append_str(identifier, "= ");
    }
    code_append(identifier, code_gen_value_or_variable(code_gen));
    code_append_str(identifier, ";\n");
    return identifier;
}

CODE* code_gen_prefix_with_tabs(const char* str, unsigned long tabs) {

    unsigned long str_len = strlen(str) + tabs;
    char* s = malloc(str_len + 1);
    strcpy(s+tabs, str);
    memset(s, '\t', tabs);
    CODE* code = malloc(sizeof(CODE));
    code->str = s;
    code->size = str_len;
    return code;
}

CODE* code_gen_switch_header(CODE_GEN* code_gen, unsigned long tabs) {

    CODE* code = code_from("switch( ");
    code_append(code, code_from((char*)code_gen_try_old(&code_gen->identifiers)));
    code_append_str(code, " ) {\n");
    return code_prefix_with_tabs(code, tabs);
}

CODE* code_gen_switch_case_header(unsigned long tabs) {

    CODE* code = code_from("case: {\n");
    return code_prefix_with_tabs(code, tabs);
}

CODE* code_gen_if_header(CODE_GEN* code_gen, unsigned long tabs) {

    CODE* code = code_from("if( ");
    code_append(code, code_gen_condition(code_gen));
    code_append(code, code_from(" ) {\n"));
    return code_prefix_with_tabs(code, tabs);
}

CODE* code_gen_while_header(CODE_GEN* code_gen, unsigned long tabs) {

    CODE* code = code_from("while( ");
    code_append(code, code_gen_condition(code_gen));
    code_append(code, code_from(" ) {\n"));
    return code_prefix_with_tabs(code, tabs);
}

CODE* code_gen_repeat_header(CODE_GEN* code_gen, unsigned long tabs) {

    CODE* code = code_from("} while( ");
    code_append(code, code_gen_condition(code_gen));
    code_append(code, code_from(" );\n"));
    return code_prefix_with_tabs(code, tabs);
}

CODE* code_declarations(CODE_GEN* code_gen) {
    CODE* code = code_from("int main() {\n");
    for(unsigned long i = 0; i < code_gen->identifiers.len && i < code_gen->identifiers.next; i++) {

        code_append_str(code, "\t");
        code_append_str(code, (char*)code_gen_get_datatype(code_gen));
        code_append_str(code, (char*)code_gen->identifiers.table[i]);
        code_append_str(code, " = ");
        code_append_str(code, (char*)code_gen_get_value(code_gen));
        code_append_str(code, ";\n");
    }
     for(unsigned long i = code_gen->identifiers.len; i < code_gen->identifiers.next; i++) {
        char id[MAX_DYN_VAR_ID_LEN + 1];
        sprintf(id, "%s%lu", code_gen->identifiers.table[0], i - code_gen->identifiers.len);

        code_append_str(code, "\t");
        code_append_str(code, (char*)code_gen_get_datatype(code_gen));
        code_append_str(code, id);
        code_append_str(code, " = ");
        code_append_str(code, (char*)code_gen_get_value(code_gen));
        code_append_str(code, ";\n");
    }
    return code;
}

// return updated dijkstra_code pointer, from where the next node should be read from
char* watermark_generate_code_recursive(CODE* code, char* dijkstra_code, unsigned long tabs, CODE_GEN* code_gen) {

    // if first character isn't a '1', there is an error, since this function should
    // be called for every node, and every node code should begin and end with an '1'
    if(!dijkstra_code || dijkstra_code[0] != '1') return NULL;

    // check if this is just a trivial node at the end of the string
    if( dijkstra_code[1] == '\0' ) return ++dijkstra_code;

    // if this isn't a trivial node, get the type from the second character
    STATEMENT_GRAPH type = dijkstra_code[1] - '0';

    if( type >= P_CASE || type == IF_THEN_ELSE ) {
        unsigned long p = type - 4;
        code_append(code, code_gen_switch_header(code_gen, tabs++));
        dijkstra_code+=2;
        // get last connection from source code and start expanding from it
        for(unsigned long i = 0; i < p; i++) {
            code_append(code, code_gen_switch_case_header(tabs));
            dijkstra_code = watermark_generate_code_recursive(code, dijkstra_code, tabs+1, code_gen);
            code_append(code, code_gen_prefix_with_tabs("break;\n", tabs+1));
            code_append(code, code_gen_prefix_with_tabs("}\n", tabs));
        }
        code_append(code, code_gen_prefix_with_tabs("}\n", --tabs));
        return watermark_generate_code_recursive(code, dijkstra_code, tabs, code_gen);
    }
    switch(type) {
        case TRIVIAL:
            code_append(code, code_prefix_with_tabs(code_gen_expression(code_gen), tabs));
            return ++dijkstra_code;
        case SEQUENCE:
            return watermark_generate_code_recursive(code, dijkstra_code+2, tabs, code_gen);
        case IF_THEN: {
            code_append(code, code_gen_if_header(code_gen, tabs));
            //code_append(code, code_prefix_with_tabs(code_gen_expression(code_gen), tabs+1));
            dijkstra_code = watermark_generate_code_recursive(code, dijkstra_code+2, tabs+1, code_gen);
            //code_append(code, code_prefix_with_tabs(code_gen_expression(code_gen), tabs+1));
            code_append(code, code_gen_prefix_with_tabs("}\n", tabs));
            return watermark_generate_code_recursive(code, dijkstra_code, tabs, code_gen);
        }
        case WHILE: {
            code_append(code, code_gen_while_header(code_gen, tabs));
            //code_append(code, code_prefix_with_tabs(code_gen_expression(code_gen), tabs+1));
            dijkstra_code = watermark_generate_code_recursive(code, dijkstra_code+2, tabs+1, code_gen);
            code_append(code, code_prefix_with_tabs(code_gen_expression(code_gen), tabs+1));
            code_append(code, code_gen_prefix_with_tabs("}\n", tabs));
            return watermark_generate_code_recursive(code, dijkstra_code, tabs, code_gen);
        }
        case REPEAT: {
            code_append(code, code_gen_prefix_with_tabs("do {\n", tabs));
            //code_append(code, code_prefix_with_tabs(code_gen_expression(code_gen), tabs+1));
            dijkstra_code = watermark_generate_code_recursive(code, dijkstra_code+2, tabs+1, code_gen);
            //code_append(code, code_prefix_with_tabs(code_gen_expression(code_gen), tabs+1));
            code_append(code, code_gen_repeat_header(code_gen, tabs));
            return watermark_generate_code_recursive(code, dijkstra_code, tabs, code_gen);
        }
        case IF_THEN_ELSE:
        case P_CASE:
        case INVALID:
        default:
            return NULL;
    }
}
char* watermark_generate_code(char* dijkstra_code) {

    CODE* code = code_from("");
    CODE_GEN code_gen = CODE_GEN_CREATE;
    watermark_generate_code_recursive(code, dijkstra_code, 1, &code_gen);
    code_append_str(code, "}\n");
    CODE* declarations = code_declarations(&code_gen);
    code = code_append(declarations, code);
    char* str = code->str;
    free(code);
    return str;
}
