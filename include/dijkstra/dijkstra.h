#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <stdint.h>

struct NODE;

typedef enum {
    INVALID,
    TRIVIAL=1,
    SEQUENCE=2,
    IF_THEN=3,
    WHILE=4,
    REPEAT=5,
    IF_THEN_ELSE=6,
    P_CASE=7,
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
