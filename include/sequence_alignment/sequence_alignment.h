#ifndef SEQUENCE_ALIGNMENT_H
#define SEQUENCE_ALIGNMENT_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {

    long score;
    unsigned long entry_point;
} NW_RESULT;

NW_RESULT watermark_needleman_wunsch(char* function_code, char* watermark_code, long match, long mismatch, long gap);

#endif
