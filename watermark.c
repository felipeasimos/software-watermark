#include "encoder/encoder.h"
#include "decoder/decoder.h"
#include "code_generator/code_generator.h"

#define MAX_SIZE 4095
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define MAX_SIZE_STR STRINGIFY(MAX_SIZE)

uint8_t get_uint8_t() {

	uint8_t n;
	scanf(" %hhu", &n);
	return n;
}

void encode_string() {

	char s[MAX_SIZE+1]={0};
	printf("input string to be encoded (max size is %lu): ", (unsigned long) MAX_SIZE);
	scanf(" %" MAX_SIZE_STR "s", s);

	GRAPH* g = watermark2014_encode(s, strlen(s));

	char* code = watermark_get_code2014(g);

	printf("code generated:\n");
	printf("%s\n", code);

	graph_free(g);
	free(code);
}

uint8_t is_little_endian_machine() {

	uint16_t x = 1;

	// if it is little endian, 1 should be the first byte
	return ((uint8_t*)(&x))[0];
}

void encode_number() {

	unsigned long n;
	printf("input positive number to be encoded: ");
	scanf(" %lu", &n);

	if( is_little_endian_machine() ) {

		// reverse number
		unsigned long tmp;
		for( uint8_t i = 0; i < sizeof(n); i++ ) {

			((uint8_t*)(&tmp))[i] = ((uint8_t*)(&n))[sizeof(n) - i - 1];
		}
		n = tmp;
	}

	GRAPH* g = watermark2014_encode(&n, sizeof(n));

	char* code = watermark_get_code2014(g);

	printf("%s\n", code);

	graph_free(g);
	free(code);
}

int main() {

    printf("do you want to insert a string or a number?\n");
	printf("1) string\n");
	printf("2) number\n");
	printf("else) exit\n");

	uint8_t quit=0;
	while(!quit) {
		switch( get_uint8_t() ) {

			// string
			case 1:
				encode_string();
				break;
			// number
			case 2:
				encode_number();
				break;
			default:
				quit=1;
		}
	}
}
