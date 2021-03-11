#include "code_generator/code_generator.h"

typedef struct STRING {

	char* str;

	// number of characters, don't include null terminator (\0)
	unsigned long len;
} STRING;

STRING* string_create(char* str, unsigned long str_len) {

	STRING* string = malloc(sizeof(STRING));

	if( str ){

		string->len = str_len ? str_len : strlen(str);

		string->str = malloc(string->len+1);
		strncpy(string->str, str, string->len);
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
	strncpy(str->str, to_clone->str, str->len);
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

	return str;
}

STRING* string_truncate(STRING* str, unsigned long new_len) {

	if( !str ) return NULL;

	str->str = realloc(str->str, new_len+1);
	str->str[new_len] = 0x00;
	str->len = new_len;
	return str;
}

STRING* string_from(STRING* str, unsigned long idx) {

	if( !str || idx > str->len-1 ) return NULL;

	char* str_old = str->str;
	str->len -= idx;
	str->str = malloc(str->len + 1);

	strncpy(str->str, str_old+idx, str->len);
	return str;
}

char* watermark_get_code(GRAPH* graph) {

	GRAPH* node=NULL;
	// get rid of all data stored in the nodes
	for(node = graph; node; node = node->next) {
		free(node->data);
		node->data = NULL;
		node->data_len = 0;
	}

	// make code blocks
	STRING* opening_bracket = string_create("{", 1);
	unsigned long i = 0;
	for(node = graph; node->next; node = node->next) {

		// check if there is a backedge
		if( node->connections && node->connections->next ) {
			if(!node->data) node->data_len = sizeof(STRING);
			if(!node->connections->node->data) node->connections->node->data_len = sizeof(STRING);

			// add } to the beginning of source node
			STRING* tmp = node->data;
			node->data = string_append(string_create("}", 1), node->data);
			string_free(tmp);
			
			// add { to the end of the destination node
			node->connections->node->data = string_append(node->connections->node->data, opening_bracket);
		}
	}
	string_free(opening_bracket);

	graph_print(graph, NULL);

	STRING* final = string_create("",0);
	// join all the code blocks in one string
	for(node = graph; node->next; node = node->next) {
	
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
