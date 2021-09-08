#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "graph/graph.h"
#include "metrics/metrics.h" 
#include <string.h>

// it frees the graph given
int watermark_is_dijkstra(GRAPH* source);

unsigned long* watermark_dijkstra_code(GRAPH*, unsigned long* size);

int watermark_dijkstra_equal(GRAPH*, GRAPH*);

#endif
