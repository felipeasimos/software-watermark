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

int watermark_is_dijkstra(GRAPH* watermark);

char* watermark_get_dijkstra_code(GRAPH* watermark);

int watermark_dijkstra_equal(GRAPH*, GRAPH*);

#endif
