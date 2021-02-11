#include "data/data.h"

DATA* data_empty() {

	DATA* data = (DATA*) malloc(sizeof(DATA));
	memset(data, 0x00, sizeof(DATA));
	return data;
}

void data_free(DATA* data) {

	if( !data ) return;

	free(data->ptr);
	free(data);
}

DATA* data_set(DATA* data, void* ptr, unsigned long len) {

	if( !data || ( !(uintptr_t)!ptr ^ (uintptr_t)!!len ) ) return NULL;

	free(data->ptr);

	data->ptr = ptr ? memcpy( malloc(len), ptr, len ) : NULL;
	data->len = len;

	return data;
}

DATA* data_init(void* ptr, unsigned long len) {

	if( !(uintptr_t)!ptr ^ (uintptr_t)!!len ) return NULL;

	return data_set( data_empty(), ptr, len );
}

int data_equal(DATA* data1, DATA* data2) {

	if( !(uintptr_t)!data1 ^ !(uintptr_t)!data2 ) return 0;
	if( !data1 && !data2 ) return 1;

	return data1->len == data2->len && ( !data1->len || !memcmp(data1->ptr, data2->ptr, data1->len) );
}
