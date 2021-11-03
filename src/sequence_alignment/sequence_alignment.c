#include "sequence_alignment/sequence_alignment.h"

#define MIN(x, y) x < y ? x : y
#define MAX(x, y) x < y ? y : x
#define MAX_SCORE(row, column)\
    if(!row) {\
        if(!column) {\
            scores[0][0] = 0;\
        } else {\
            scores[0][column] = scores[0][column-1] + gap;\
        }\
    } else if(!column){\
        scores[row][0] = scores[row-1][0] + gap;\
    } else {\
        long diagonal_score = scores[row-1][column-1] + (seq1[row-1] == seq2[column-1] ? match : mismatch);\
        long max_gap = MAX(scores[row-1][column] + gap, scores[row][column-1] + gap);\
        scores[row][column] = MAX(diagonal_score, max_gap);\
    }

long sequence_alignment_score_needleman_wunsch(char* seq1, char* seq2, long match, long mismatch, long gap) {

    unsigned long seq1_len = strlen(seq1)+1;
    unsigned long seq2_len = strlen(seq2)+1;
   
    // +1 because we can start with a gap
    long scores[seq1_len][seq2_len];

    unsigned long max_p = MIN(seq1_len, seq2_len);
    for(unsigned long p = 0; p < max_p; p++) {

        // get pivot value
        MAX_SCORE(p, p)
        // rows
        for(unsigned long i = p+1; i < seq1_len; i++) {
            MAX_SCORE(i, p);
        }
        // columns
        for(unsigned long j = p+1; j < seq2_len; j++) {
            MAX_SCORE(p, j);
        }
    }
    for(unsigned long i =0; i < seq1_len; i++) {

        for(unsigned long j = 0; j < seq2_len; j++) {
            printf("%5ld ", scores[i][j]);
        }
        printf("\n");
    }
    return scores[seq1_len-1][seq2_len-1];
}
