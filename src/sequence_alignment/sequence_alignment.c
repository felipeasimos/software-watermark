#include "sequence_alignment/sequence_alignment.h"

typedef enum {
    DOWN=0,
    RIGHT=1,
    DIAGONAL=2,
    END=3
} PATH;

#define MIN(x, y) x < y ? x : y
#define MAX(x, y) x < y ? y : x
#define MAX_SCORE(row, column)\
    if(!row) {\
        if(!column) {\
            scores[0][0] = 0;\
        } else {\
            scores[0][column] = scores[0][column-1] + gap;\
            path[0][column-1] = RIGHT;\
        }\
    } else if(!column){\
        scores[row][0] = scores[row-1][0] + gap;\
        path[row-1][0] = DOWN;\
    } else {\
        long diagonal_score = scores[row-1][column-1] + (seq1[row-1] == seq2[column-1] ? match : mismatch);\
        uint8_t skipping_seq1 = scores[row-1][column] + gap > scores[row][column-1] + gap;\
        long max_gap = MAX(scores[row-1][column] + gap, scores[row][column-1] + gap);\
        if( max_gap > diagonal_score ) {\
            scores[row][column] = max_gap;\
            if( skipping_seq1 ) {\
                path[row-1][column] = DOWN;\
            } else {\
                path[row][column-1] = RIGHT;\
            }\
        } else {\
            scores[row][column] = diagonal_score;\
            path[row-1][column-1] = DIAGONAL;\
        }\
    }

NW_RESULT watermark_needleman_wunsch(char* function_code, char* watermark_code, long match, long mismatch, long gap) {

    char* seq1 = function_code;
    char* seq2 = watermark_code;

    unsigned long seq1_len = strlen(seq1)+1;
    unsigned long seq2_len = strlen(seq2)+1;

    // +1 because we can start with a gap
    long scores[seq1_len][seq2_len];
    PATH path[seq1_len][seq2_len];
    path[seq1_len-1][seq2_len-1] = END;

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

    // calculate how many gaps in 'function_code' before we have a match/mismatch
    // to do this, just calculate how many initial consecutive 'DOWN's we have (ignore the first one, since it is always 1 it will be a diagonal)
    unsigned long entry_point=0;
    /*unsigned long i = 0;
    unsigned long j = 0;
    while( i != seq1_len-1 || j != seq2_len-1 ) {
        switch(path[i][j]) {
            case DIAGONAL:
                fprintf(stderr, "DIAGONAL\n");
                j++; i++;
                break;
            case DOWN:
                fprintf(stderr, "DOWN\n");
                i++;
                break;
            case RIGHT:
                fprintf(stderr, "RIGHT\n");
                j++;
                break;
            case END:
                i = seq1_len-1;
                j = seq2_len-1;
                fprintf(stderr, "END\n");
                break;
        }
    }*/
    NW_RESULT results = {
        .score = scores[seq1_len-1][seq2_len-1],
        .entry_point = entry_point
    };

    return results;
}
