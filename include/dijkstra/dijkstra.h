#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <stdint.h>

struct NODE;

typedef enum {
    INVALID,
    TRIVIAL,
    SEQUENCE,
    IF_THEN,
    WHILE,
    REPEAT,
    IF_THEN_ELSE,
    P_CASE,
} STATEMENT_GRAPH;

typedef struct PRIME_SUBGRAPH {

    struct NODE* sink;
    STATEMENT_GRAPH type;
} PRIME_SUBGRAPH;

#include "graph/graph.h"

int dijkstra_check(GRAPH* graph);

char* dijkstra_get_code(GRAPH* watermark);

int dijkstra_is_equal(GRAPH*, GRAPH*);

GRAPH* dijkstra_generate(char* code);

#endif
