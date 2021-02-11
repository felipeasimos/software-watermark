#include "list_double/list_double.h"

void _dlist_raw_connect(DLIST* prev, DLIST* next){

	if( prev ){
		if( prev->next ) prev->next->prev = NULL;
		prev->next = next;
	}

	if( next ){
		if( next->prev ) next->prev->next = NULL;
		next->prev = prev;
	}
}

DLIST* dlist_empty(){

	DLIST* dlist = (DLIST*)malloc(sizeof(DLIST));
	memset(dlist, 0x00, sizeof(DLIST));
	return dlist;
}

DLIST* dlist_init(DLIST* prev, DLIST* next, void* data, unsigned long data_len){

	DLIST* dlist = dlist_empty();

	_dlist_raw_connect(dlist, next);
	_dlist_raw_connect(prev, dlist);

	data_set(&dlist->data, data, data_len);

	return dlist;
}

void dlist_node_free(DLIST* node){

	if( !node ) return;

	//free(node->data.ptr);

	if( node->prev ) node->prev->next = NULL;
	if( node->next ) node->next->prev = NULL;

	free(node);
}

void dlist_free(DLIST* dlist){

	if( !dlist ) return;

	dlist_free(dlist->next);
	dlist_node_free(dlist);
}

DLIST* dlist_begin(DLIST* dlist){

	if( !dlist ) return NULL;

	while( dlist->prev ) dlist = dlist->prev;

	return dlist;
}

DLIST* dlist_end(DLIST* dlist){

	if( !dlist ) return NULL;

	while( dlist->next ) dlist = dlist->next;

	return dlist;
}

DLIST* _dlist_search_next(DLIST* list, void* data, unsigned long data_len){

	DATA d = { data, data_len };

	//go to next node until list==NULL or we have a match
	for(; list && !data_equal(&d, &list->data); list = list->next ){}

	return list;
}

DLIST* _dlist_search_prev(DLIST* list, void* data, unsigned long data_len){

	DATA d = { data, data_len };

	//go to previous node until list==NULL or we have a match
	for(; list && !data_equal(&d, &list->data); list = list->prev ){}

	return list;
}

DLIST* dlist_search(DLIST* dlist, void* data, unsigned long data_len){

	DLIST* ret_next = _dlist_search_next(dlist, data, data_len);

	return ret_next ? ret_next : _dlist_search_prev(dlist, data, data_len);
}

DLIST* dlist_node_link(DLIST* prev, DLIST* next){

	if( next ){
		next->prev = prev;
		if( prev ) next->next = prev->next;
	}


	if( prev ){
	
		if( prev->next ) prev->next->prev = next;
		prev->next = next;
	}

	return prev ? prev : next;
}

DLIST* dlist_link(DLIST* list, void* data, unsigned long data_len){

	DLIST* next = ( (uintptr_t)!!data ^ (uintptr_t)!!data_len ) ? NULL : dlist_init(NULL, NULL, data, data_len);

	return dlist_node_link( list, next );
}

DLIST* dlist_node_push(DLIST* list, DLIST* node) {

	if( !list ) return NULL;

	_dlist_raw_connect( dlist_end(list), node);

	return list;
}

DLIST* dlist_push(DLIST* list, void* data, unsigned long data_len) {

	DLIST* next = ( (uintptr_t)!!data ^ (uintptr_t)!!data_len ) ? NULL : dlist_init(NULL, NULL, data, data_len);

	return dlist_node_push( list, next );
}

DLIST* dlist_node_remove(DLIST* list, DLIST* node) {

	if( !list || !node ) return list;

	if( list == node ) list = list->next;

	_dlist_raw_connect( node->prev, node->next );
	dlist_node_free(node);

	return list;
}

DLIST* dlist_remove(DLIST* list, void* data, unsigned long data_len) {

	return dlist_node_remove(list, dlist_search(list, data, data_len));
}
