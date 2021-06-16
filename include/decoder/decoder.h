#ifndef DECODER_H
#define DECODER_H

#include "utils/utils.h"
#include <stdint.h>
#include "rs_api/rs.h"
#include "time.h"

void* watermark2014_decode(GRAPH* graph, unsigned long* num_bytes);

void* watermark2014_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_uint16s);

void* watermark2017_decode(GRAPH* graph, unsigned long* num_bytes);

void* watermark2017_decode_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_uint16s);

void* watermark2017_decode_analysis(GRAPH* graph, unsigned long* num_bytes);

void* watermark2017_decode_analysis_with_rs(GRAPH* graph, unsigned long* num_bytes, unsigned long num_rs_uint16s);

#endif
