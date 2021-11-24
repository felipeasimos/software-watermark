#include "decoder/decoder.h"
#include "encoder/encoder.h"
#include "code_generation/code_generation.h"
#include "sequence_alignment/sequence_alignment.h"
#include "dijkstra/dijkstra.h"
#include <dirent.h>

#define MAX_SIZE 4095
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define MAX_SIZE_STR STRINGIFY(MAX_SIZE)

void ask_for_comparison(char* dijkstra_code) {

    printf("would you like to find a function that best fits this watermark?[y/n] ");
    char choice;
    scanf(" %c", &choice);

    if(choice!='y' && choice!='Y') return;

    char str[MAX_SIZE];
    printf("\npath to the binary to be analyzed: ");
    scanf(" %s", str);

    char str2[strlen(str) + strlen("./cfg.sh ") + 1];
    str2[0]='\0';
    strcat(str2, "./cfg.sh ");
    strcat(str2, str);
    system(str2);

    long highest_score = ~ULONG_MAX;
    char highest_score_func[MAX_SIZE]={0};
    char highest_score_code[MAX_SIZE]={0};
    GRAPH* highest_score_graph = NULL;
    DIR *dir;
    struct dirent *ent;
    if((dir = opendir("/home/felipe/Documents/uff/inmetro/waterwark/watermark")) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            // starts with '.' and ends with '.dot'
            unsigned long len = strlen(ent->d_name);
            if( len > 4 && ent->d_name[0] == '.' && ent->d_name[len-4] == '.' && ent->d_name[len-3] == 'd' && ent->d_name[len-2] == 'o' && ent->d_name[len-1] == 't') {

                FILE* f;
                if( (f = fopen(ent->d_name, "r")) != NULL ) {
                    GRAPH* g = graph_create_from_dot(f);
                    graph_topological_sort(g);
                    if( dijkstra_check(g) ) {
                        char* current_dijkstra_code = dijkstra_get_code(g);
                        long score = sequence_alignment_score_needleman_wunsch(dijkstra_code, current_dijkstra_code, -10, -1, 2);
                        fprintf(stderr, "%s:\n\tcode: %s\n\tscore: %ld\n", ent->d_name, current_dijkstra_code, score);
                        if( score > highest_score ) {
                            highest_score = score;
                            memset(highest_score_func, 0x00, MAX_SIZE);
                            strncpy(highest_score_func, &ent->d_name[1], len-5);
                            strcpy(highest_score_code, current_dijkstra_code);
                            if(highest_score_graph) graph_free(highest_score_graph);
                            highest_score_graph = g;
                        }
                        free(current_dijkstra_code);
                    }
                }
                remove(ent->d_name);
            }
        }
        closedir (dir);
    }
    if(highest_score_func[0] != '\0') {

        printf("\nwatermark dijkstra code: %s\n", dijkstra_code);
        printf("best fit function is: %s\n", highest_score_func);
        printf("\tscore: %ld\n", highest_score);
        printf("\tdijkstra code: %s\n", highest_score_code);

        char* code = watermark_generate_code(dijkstra_code);
        printf("\n\nwatermark example code:\n%s", code);
        free(code);
        code = watermark_generate_code(highest_score_code);
        printf("\n\n%s code example:\n%s", highest_score_func, code);
        free(code);
    }
}

uint8_t get_uint8_t() {
    uint8_t n;
    scanf("%c", &n);
    return n-'0';
}

int main() {

    printf("1) encode string\n");
    printf("2) encode number\n");
    printf("3) generate graph from dijkstra code\n");
    printf("4) run removal test\n");
    printf("5) run removal test with reed solomon\n");
    printf("6) show report matrix\n");
    printf("else) exit\n");
    switch(get_uint8_t()) {
        case 1: {
            printf("Input number to encode: ");
            char s[MAX_SIZE+1];
            scanf(" %" MAX_SIZE_STR "s", s);
            unsigned long data_len=strlen(s);
            void* data = encode_numeric_string(s, &data_len);
            GRAPH* g = watermark_encode(data, data_len);
            char* dijkstra_code_raw = dijkstra_get_code(g);
            char* dijkstra_code = malloc(strlen(dijkstra_code_raw) + strlen(s) + 4);
            dijkstra_code[0]='\0';
            strncpy(dijkstra_code, dijkstra_code_raw, strlen(dijkstra_code_raw) + strlen(s) + 4);
            strcat(dijkstra_code, " - ");
            strcat(dijkstra_code, s);
            graph_write_hamiltonian_dot(g, "dot.dot", dijkstra_code);
            graph_print(g, NULL);
            ask_for_comparison(dijkstra_code_raw);
            free(data);
            free(dijkstra_code);
            graph_free(g);
            break;
        }
        case 2: {
            printf("Input number to encode: ");
            unsigned long n;
            char s[MAX_SIZE+1];
            scanf("%lu", &n);
            invert_byte_sequence((uint8_t*)&n, sizeof(n));
            GRAPH* g = watermark_encode(&n, sizeof(n));
            char* dijkstra_code_raw = dijkstra_get_code(g);
            char* dijkstra_code = malloc(strlen(dijkstra_code_raw) + strlen(s) + 4);
            dijkstra_code[0]='\0';
            strncpy(dijkstra_code, dijkstra_code_raw, strlen(dijkstra_code_raw) + strlen(s) + 4);
            strcat(dijkstra_code, " - ");
            strcat(dijkstra_code, s);
            invert_byte_sequence((uint8_t*)&n, sizeof(n));
            sprintf(s, "%lu", n);
            strcat(dijkstra_code, s);
            graph_write_hamiltonian_dot(g, "dot.dot", dijkstra_code);
            graph_print(g, NULL);
            ask_for_comparison(dijkstra_code_raw);
            free(dijkstra_code);
            graph_free(g);
            break;
        }
        case 3: {
            printf("Input dijkstra code: ");
            char s[MAX_SIZE+1];
            scanf("%s", s);
            GRAPH* g = dijkstra_generate(s);
            graph_print(g, NULL);
            graph_write_dot(g, "dot.dot", s);
            ask_for_comparison(s);
            graph_free(g);
            break;
        }
        case 4: {

            break;
        }
    }

    return 0;
}
