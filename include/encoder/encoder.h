#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include <time.h>
#include "graph/graph.h"
#include "rs_api/rs.h"

GRAPH* watermark2014_encode(void* data, unsigned long data_len);

GRAPH* watermark2014_encode_with_rs(void* data, unsigned long data_len, unsigned long num_rs_bytes);

GRAPH* watermark2017_encode(void* data, unsigned long data_len);

GRAPH* watermark2017_encode_with_rs(void* data, unsigned long data_len, unsigned long num_rs_bytes);

#endif
