#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "graph/graph.h"

int watermark_is_dijkstra(GRAPH* watermark);

unsigned long* watermark_dijkstra_code(NODE* sink, unsigned long* size);

int watermark_dijkstra_equal(NODE*, NODE*);

#endif
