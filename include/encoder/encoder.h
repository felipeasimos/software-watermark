#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include <time.h>
#include "graph/graph.h"

GRAPH* watermark_encode(void* data, unsigned long data_len);

#endif
