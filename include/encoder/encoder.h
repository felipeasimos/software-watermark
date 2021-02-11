#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include <time.h>
#include "graph/graph.h"
#include "list_double/list_double.h"

GRAPH* watermark_encode(void* data, unsigned long data_len);

#endif
