#include "code_generator/code_generator.h"

typedef struct STRING {

	char* str;

	// number of characters, don't include null terminator (\0)
	unsigned long len;
} STRING;

typedef struct GEN_TABLE {

	const char** table;
	unsigned long len;
	unsigned long next;
	unsigned long last;
	unsigned long new;
	unsigned long new_counter;
} GEN_TABLE;

#define ARRAY_SIZE(arr) sizeof(arr)/sizeof(arr[0])

#define GEN_TABLE_CREATE(...) {\
	.table = (const char*[]){__VA_ARGS__},\
	.len = ARRAY_SIZE( ( (const char*[]){__VA_ARGS__} ) ),\
	.next = 0,\
	.last = (unsigned long)-1,\
	.new = 5,\
	.new_counter = 0\
}

STRING* string_create(char* str, unsigned long str_len) {

	STRING* string = malloc(sizeof(STRING));

	if( str ){

		string->len = str_len ? str_len : strlen(str);

		string->str = malloc(string->len+1);
		for(unsigned long i=0; i < string->len; i++) string->str[i] = str[i];
		string->str[string->len]=0x00;
	} else {
		string->str = NULL;
		string->len = 0;
	}

	return string;
}

void string_free(STRING* str) {

	if(!str) return;

	free(str->str);
	free(str);
}

STRING* string_clone(STRING* str) {

	if( !str ) return NULL;

	return string_create(str->str, str->len);
}

STRING* string_copy(STRING* str, STRING* to_clone) {

	if( !str || !to_clone ) return NULL;

	free(str->str);
	str->len = to_clone->len;
	str->str = malloc(str->len+1);
	for(unsigned long i=0; i < str->len; i++) str->str[i] = to_clone->str[i];
	str->str[str->len]=0x00;
	return str;
}

STRING* string_append(STRING* str, STRING* appendix) {

	if( str ) {
		if( !appendix ) return str;
	} else if( appendix ){
		return string_clone(appendix);
	}

	unsigned long old_len = str->len;

	str->len += appendix->len;
	str->str = realloc(str->str, str->len+1);

	// concatenate strings
	for(unsigned long i = old_len; i < str->len; i++) str->str[i] = appendix->str[ i - old_len ];

	str->str[str->len]=0x00;

	return str;
}

STRING* string_truncate(STRING* str, unsigned long new_len) {

	if( !str ) return NULL;

	str->len = new_len;
	str->str = realloc(str->str, str->len+1);
	str->str[str->len] = 0x00;
	return str;
}

void free_data_from_nodes(GRAPH* graph) {

	GRAPH* node=NULL;
	// get rid of all data stored in the nodes
	for(node = graph; node; node = node->next) {
		free(node->data);
		node->data = NULL;
		node->data_len = 0;
	}
}

void generate_code_blocks(GRAPH* graph) {

	// make code blocks
	STRING* opening_bracket = string_create("{", 1);

	for(GRAPH* node = graph; node->next; node = node->next) {

		// mark every node we go through, so we can detected backedges in the future
		node->data_len = UINT_MAX; // backedges will have data_len equal to a non-zero positive number

		// check if there is a backedge (get connection where data != NULL)
		GRAPH* backedge_node = get_backedge(node);
		if( backedge_node ) {
			if(!node->data) node->data_len = sizeof(STRING);
			if(!backedge_node->data) backedge_node->data_len = sizeof(STRING);

			// add } to the beginning of source node
			STRING* tmp = node->data;
			node->data = string_append(string_create("}", 1), node->data);
			string_free(tmp);
			
			// add { to the end of the destination node
			backedge_node->data = string_append(backedge_node->data, opening_bracket);
		}
	}
	string_free(opening_bracket);
}

char* join_strings_from_graph(GRAPH* graph) {

	STRING* final = string_create("",0);
	// join all the code blocks in one string
	for(GRAPH* node = graph; node->next; node = node->next) {
	
		if( node->data ) {
			final = string_append(final, node->data);
			string_free(node->data);
			node->data = NULL;
		}
	}

	char* final_string = final->str;
	free(final);
	return final_string;
}

STRING* generate_non_repeating_variable(GEN_TABLE* variables) {

	unsigned long idx;
	do {
		idx = rand() % variables->next;
	} while( idx == variables->last );

	variables->last = idx;

	if( idx >= variables->len ) {
	
		unsigned long id = idx - variables->len;
		char s[50]={0};
		sprintf(s,"x%lu", id);
		return string_create(s, 0);
	} else {
	
		return string_create( (char*)variables->table[idx], 0 );
	}

}

STRING* generate_variable(GEN_TABLE* variables) {

	// <variable>
	STRING* var = NULL;
	// use old variable?
	if( variables->next > 1 && variables->new > variables->new_counter ) {

		variables->new_counter++;
		return generate_non_repeating_variable(variables);
	// get new variable
	} else {

		variables->new_counter=0;
		variables->new++;
		// do we generate a new one?
		if( variables->next >= variables->len ) {

			unsigned long id = variables->next - variables->len;
			char s[50]={0};
			sprintf(s, "x%lu", id);
			var = string_create(s, 0);
		// get new variable from table
		} else {
			var = string_create( (char*) variables->table[ variables->next ], 0 );
		}
		variables->last = variables->next++;
	}

	return var;
}

STRING* generate_variable_or_value(GEN_TABLE* variables, GEN_TABLE* values) {

	STRING* var = NULL;
	// <variable> | <value>
	if( rand() & 1 ) {

		var = generate_variable(variables);
	} else {
		var = string_create((char*)values->table[ rand() % values->len ], 0);
	}

	return var;
}

STRING* generate_initializer(char* variable, GEN_TABLE* initializers, GEN_TABLE* values) {

	// <initializer> <variable> = ≲value>;\n
	STRING* init = string_create((char*)initializers->table[ rand() % initializers->len ], 0);
	STRING* var1 = string_create(variable, 0);
	STRING* assignment = string_create(" = ", 0);
	STRING* var2 = string_create((char*)values->table[ rand() % values->len ], 0);
	STRING* endline = string_create((char*)";\n", 0);

	STRING* initializer = string_append( init, string_append( var1, string_append( assignment, string_append(var2, endline) ) ) );

	initializer = string_clone( initializer );
	string_free(init);
	string_free(var1);
	string_free(assignment);
	string_free(var2);
	string_free(endline);

	return initializer;
}

char* get_variable_at_index(GEN_TABLE* variables, unsigned long i) {

	char* var = NULL;
	if( i >= variables->len ) {

		unsigned long id = i - variables->len;
		var = malloc(50);
		sprintf(var, "x%lu", id);
	} else {
		var = (char*)variables->table[ i ];
	}
	return var;
}

STRING* generate_initializers(GEN_TABLE* variables, GEN_TABLE* initializers, GEN_TABLE* values) {

	STRING* init=NULL;
	for(unsigned long i=0; i < variables->next; i++) {

		char* var = get_variable_at_index(variables, i);

		STRING* new_initializer = generate_initializer(var, initializers, values);
		init = string_append(init, new_initializer);
		string_free(new_initializer);
	}
	return init ? init : string_create("",0);
}

STRING* generate_expression(GEN_TABLE* variables, GEN_TABLE* values, GEN_TABLE* operations) {

	STRING* exp=NULL;

	STRING* var1 = generate_variable(variables);

	STRING* space1 = string_create(" ", 0);
	STRING* space2 = string_create(" ", 0);
	STRING* space3 = string_create(" ", 0);
	STRING* assignment = string_create((char*)"= ", 0);
	STRING* var2 = generate_variable_or_value(variables, values);
	STRING* op = string_create( (char*)operations->table[ rand() % operations->len ], 0 );

	if( rand() & 1 ) {
		// <variable> = <variable> <operation> ( <value> | <variable> )
		STRING* var3 = generate_variable(variables);

		exp = string_append(var1,
				string_append(space1,
					string_append(assignment,
						string_append( var2,
							string_append(space2,
								string_append(op,
									string_append(space3, var3 ) ))))));
		string_free(var3);
	} else {
		// <variable> <operation>= ( <value> | <variable> )
		exp = string_append(var1, string_append( space1, string_append(op, string_append(assignment, var2))));
	}
	exp = string_clone(exp);
	string_free(var1);
	string_free(assignment);
	string_free(var2);
	string_free(op);
	string_free(space1);
	string_free(space2);
	string_free(space3);

	return exp;
}

STRING* generate_condition(GEN_TABLE* variables, GEN_TABLE* values, GEN_TABLE* comparisons) {

	// <variable> <comparisons> <variable>

	STRING* var1 = generate_variable(variables);
	STRING* comp = string_create( (char*)comparisons->table[ rand() % comparisons->len ], 0 );
	STRING* var2 = generate_variable_or_value(variables, values);

	STRING* condition = string_append( var1, string_append( comp, var2 ) );

	condition = string_clone(condition);
	string_free(var1);
	string_free(comp);
	string_free(var2);

	return condition;
}

STRING* generate_tabs(unsigned long tabs) {

	char s[tabs+1];
	for(unsigned long i = 0; i < tabs; i++) s[i] = '\t';
	STRING* t = string_create(tabs? s : "", tabs);
	return t;
}

STRING*  generate_opening_bracket_snippet(
		unsigned long tabs,
		GEN_TABLE* variables,
		GEN_TABLE* values,
		GEN_TABLE* operations) {

	// <n tabs>do {\n
	// <n+1 tabs><expression>;\n

	STRING* t1 = generate_tabs(tabs++);
	STRING* do_ = string_create("do {\n", 0);

	STRING* t2 = generate_tabs(tabs);
	STRING* exp = generate_expression(variables, values, operations);

	STRING* endline = string_create(";\n", 0);

	STRING* opening = string_append(t1, string_append( do_, string_append( t2, string_append(exp, endline) ) ));

	opening = string_clone(opening);
	string_free( t1 );
	string_free( do_ );
	string_free( t2 );
	string_free( exp );
	string_free( endline );

	return opening;
}

STRING* generate_closing_bracket_snippet(
		unsigned long tabs,
		GEN_TABLE* variables,
		GEN_TABLE* values,
		GEN_TABLE* comparisons) {

	// <n tabs>while( <condition> );\n
	STRING* t = generate_tabs(--tabs);
	STRING* while_ = string_create((char*)"} while( ", 0);
	STRING* condition = generate_condition(variables, values, comparisons);
	STRING*	endline = string_create((char*)");\n", 0);

	STRING* closing = string_append( t, string_append( while_, string_append( condition, endline ) ) );

	closing = string_clone(closing);
	string_free(t);
	string_free(while_);
	string_free(condition);
	string_free(endline);

	return closing;
}

void generate_pseudocode(GRAPH* graph) {

	// tables used to produce code
	GEN_TABLE variables = GEN_TABLE_CREATE("ptr", "idx", "x", "y", "n", "i", "len");
	GEN_TABLE values = GEN_TABLE_CREATE("0b1010000", "'a'", "0x00", "1", "0", "4096", "~1", "'z'");
	GEN_TABLE comparisons = GEN_TABLE_CREATE(" > ", " < ", " == ", " != ", " <= ", " >= ", " ^ ", " & ");
	GEN_TABLE operations = GEN_TABLE_CREATE("*", "/", "+", "-", "|", "&", "^");
	GEN_TABLE initializers = GEN_TABLE_CREATE("int ", "unsigned long ", "unsigned int ", "short ");

	unsigned long tabs = 0;

	for(GRAPH* node=graph; node->next; node = node->next) {

		if( node->data ) {

			STRING* new_str = string_create("", 0);
			STRING* old_str = node->data;
			// iterate through string
			for(unsigned long i = 0; i < old_str->len; i++) {

				if( old_str->str[i] == '{' ) {
					STRING* new_part = generate_opening_bracket_snippet(tabs, &variables, &values, &operations);
					string_append(new_str, new_part);
					string_free(new_part);
					tabs++;
				} else {
					STRING* new_part = generate_closing_bracket_snippet(tabs, &variables, &values, &comparisons);
					string_append(new_str, new_part);
					string_free(new_part);
					tabs--;
				}
			}
			string_copy(node->data, new_str);
			string_free(new_str);
		}

	}

	STRING* tmp = graph->data;
	graph->data = string_append( generate_initializers(&variables, &initializers, &values), graph->data);
	string_free(tmp);
}

char* watermark_get_code2014(GRAPH* graph) {

	free_data_from_nodes(graph);

	generate_code_blocks(graph);

	generate_pseudocode(graph);

	return join_strings_from_graph(graph);
}
