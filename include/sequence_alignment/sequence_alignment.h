#ifndef SEQUENCE_ALIGNMENT_H
#define SEQUENCE_ALIGNMENT_H

#include <string.h>
#include <stdio.h>

long sequence_alignment_score_needleman_wunsch(char* seq1, char* seq2, long mismatch, long gap, long match);

#endif
