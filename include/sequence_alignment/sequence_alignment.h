#ifndef SEQUENCE_ALIGNMENT_H
#define SEQUENCE_ALIGNMENT_H

#include <string.h>

unsigned long sequence_alignment_score_needleman_wunsch(char* seq1, char* seq2, unsigned long mismatch, unsigned long gap, unsigned long match);

#endif
