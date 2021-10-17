#include "decoder/decoder.h"
#include "encoder/encoder.h"

#define MAX_SIZE 4095
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define MAX_SIZE_STR STRINGIFY(MAX_SIZE)

uint8_t get_uint8_t() {
    uint8_t n;
    scanf("%c", &n);
    return n-'0';
}

int main() {

    printf("1) encode string\n");
    printf("2) encode number\n");
    printf("3) generate graph from dijkstra code\n");
    printf("3) run removal test\n");
    printf("4) run removal test with reed solomon\n");
    printf("5) show report matrix\n");
    printf("else) exit\n");
    switch(get_uint8_t()) {
        case 1: {
            printf("Input number to encode: ");
            char s[MAX_SIZE+1]={0};
            scanf(" %" MAX_SIZE_STR "s", s);
            unsigned long data_len=strlen(s);
            void* data = encode_numeric_string(s, &data_len);
            GRAPH* g = watermark_encode(data, data_len);
            char* dijkstra_code = dijkstra_get_code(g);
            dijkstra_code = realloc(dijkstra_code, strlen(dijkstra_code) + strlen(s) + 4);
            strcat(dijkstra_code, " - ");
            strcat(dijkstra_code, s);
            graph_write_hamiltonian_dot(g, "dot.dot", dijkstra_code);
            graph_print(g, NULL);
            free(data);
            free(dijkstra_code);
            graph_free(g);
            break;
        }
        case 2: {
            printf("Input number to encode: ");
            unsigned long n;
            char s[MAX_SIZE+1]={0};
            scanf("%lu", &n);
            invert_binary_sequence((uint8_t*)&n, sizeof(n));
            GRAPH* g = watermark_encode(&n, sizeof(n));
            char* dijkstra_code = dijkstra_get_code(g);
            dijkstra_code = realloc(dijkstra_code, strlen(dijkstra_code) + strlen(s) + 4);
            strcat(dijkstra_code, " - ");
            sprintf(s, "%lu", n);
            strcat(dijkstra_code, s);
            graph_write_hamiltonian_dot(g, "dot.dot", dijkstra_code);
            graph_print(g, NULL);
            free(dijkstra_code);
            graph_free(g);
            break;
        }
        case 3: {
            printf("Input dijkstra code: ");
            char s[MAX_SIZE+1]={0};
            scanf("%s", s);
            GRAPH* g = dijkstra_generate(s);
            graph_print(g, NULL);
            graph_write_dot(g, "dot.dot", s);
            graph_free(g);
            break;
        }
        case 4: {

            break;
        }
    }

    return 0;
}
