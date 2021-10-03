#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "graph/graph.h"

int watermark_is_dijkstra(GRAPH* watermark);

char* watermark_dijkstra_code(GRAPH* watermark);

int watermark_dijkstra_equal(GRAPH*, GRAPH*);

#endif
