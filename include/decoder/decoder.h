#ifndef DECODER_H
#define DECODER_H

#include "utils/utils.h"
#include <stdint.h>
#include "rs_api/rs.h"

void* watermark2014_decode(GRAPH* graph, unsigned long* num_bytes);

void* watermark2014_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_bytes);

void* watermark2017_decode(GRAPH* graph, unsigned long* num_bytes);

void* watermark2017_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_bytes);

void* watermark2017_decode_percentage(GRAPH* graph, unsigned long* num_bytes);

void* watermark2017_decode_percentage_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_bytes);

#endif
