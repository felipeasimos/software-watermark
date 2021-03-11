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

	free(str->str);
	free(str);
}

STRING* string_copy(STRING* str) {

	return string_create(str->str, str->len);
}

STRING* string_append(STRING* str, STRING* appendix) {

	str = string_copy(str);
	// update length
	str->len += appendix->len;
	// get more memory using previously calculated length
	str->str = realloc(str->str, str->len+1);
	// concatenate strings
	strncat(str->str, appendix->str, appendix->len);

	return str;
}

STRING* string_truncate(STRING* str, unsigned long new_len) {

	return string_create(str->str, new_len);
}

STRING* string_from(STRING* str, unsigned long idx) {

	if( idx > str->len-1 ) idx = str->len;

	return string_create(str->str + idx, str->len - idx);
}

STRING* string_replace(STRING* str, STRING* replace, unsigned long substr_idx, unsigned long substr_len) {

	// replace = str[:substr_idx] + replace + str[substr_idx:]
	STRING* beginning = string_truncate(str, substr_idx+1);
	STRING* end = string_from(str, substr_idx+substr_len);

	STRING* str1 = string_append(beginning, replace);
	STRING* str2 = string_append(str1, end);

	string_free(beginning);
	string_free(end);
	string_free(str1);

	return str2;
}

STRING* string_find_and_replace(STRING* str, STRING* find, STRING* replace) {

	STRING* tmp;
	for(unsigned long i=0; i < str->len;) {
		// get index of substring
		char* substr_pointer = strstr((const char*)(str->str + i), (const char*)find->str);
		if( !substr_pointer ) {
			return i ? str : NULL;
		}

		unsigned long substr_idx = substr_pointer - str->str;

		tmp = string_replace(str, replace, substr_idx, find->len);
		string_free(str);
		str = tmp;
		i = substr_idx + replace->len;
	}

	return str;
}

void string_add_to_node(GRAPH* node, char* new_code) {

	if( node->data ) {
		STRING* old_str = node->data;
		STRING* tmp = string_create(new_code, 0);
		node->data = string_append(old_str, tmp);
		string_free(tmp);
		string_free(old_str);
	} else {
		node->data = string_create(new_code, 0);
	}
}

char* watermark_get_code(GRAPH* graph) {

	GRAPH* node=NULL;
	// get rid of all data stored in the nodes
	for(node = graph; node; node = node->next) {
		free(node->data);
		node->data = NULL;
	}

	// make code blocks
	for(node = graph; node->next; node = node->next) {

		// check if there is a backedge
		if( node->connections && node->connections->next ) {
			// add "do {\n" to destination, add "} while();\n"
			string_add_to_node(node->connections->node, (char*)"do {\n");
			string_add_to_node(node, (char*)"} while();\n");
		}
	}

	STRING* final = string_create("",0);
	// join all the code blocks in one string
	for(node = graph; node->next; node=node->next) {
	
		if( node->data ) {
			STRING* tmp = final;
			final = string_append(final, node->data);
			string_free(tmp);
			string_free(node->data);
			node->data = NULL;
		}
	}

	char* final_string = final->str;
	free(final);
	return final_string;
}
