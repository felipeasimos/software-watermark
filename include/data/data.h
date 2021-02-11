#ifndef DS_DATA_H
#define DS_DATA_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct DATA {

	void* ptr;
	unsigned long len;
} DATA;

DATA* data_empty();
void data_free(DATA* data);

DATA* data_set(DATA* data, void* ptr, unsigned long len);
DATA* data_init(void* ptr, unsigned long len);

int data_equal(DATA* data1, DATA* data2);

#endif
