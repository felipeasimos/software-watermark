#include "utils/utils.h"

PSTACK* pstack_create(unsigned long max_nodes) {

    PSTACK* pstack = malloc(sizeof(PSTACK));
    pstack->stack = calloc(max_nodes, sizeof(struct NODE*));
    pstack->n = 0;
    return pstack;
}

void pstack_push(PSTACK* stack, struct NODE* node) {
    stack->stack[stack->n++] = node;
}
struct NODE* pstack_pop(PSTACK* stack) {
    return stack->n ? stack->stack[--stack->n] : NULL;
}
struct NODE* pstack_get(PSTACK* stack) {
    return stack->n ? stack->stack[stack->n-1] : NULL;
}
void pstack_free(PSTACK* stack) {
    free(stack->stack);
    free(stack);
}

HSTACK* hstack_create(unsigned long max_nodes) {

    HSTACK* hstack = malloc(sizeof(HSTACK));
    hstack->stack = calloc(max_nodes, sizeof(unsigned long));
    hstack->n = 0;
    return hstack;
}

void hstack_push(HSTACK* stack, unsigned long node) {
    stack->stack[stack->n++] = node;
}
unsigned long hstack_pop(HSTACK* stack) {
    return stack->n ? stack->stack[--stack->n] : ULONG_MAX;
}
unsigned long hstack_get(HSTACK* stack) {
    return stack->n ? stack->stack[stack->n-1] : ULONG_MAX;
}
void hstack_free(HSTACK* stack) {
    free(stack->stack);
    free(stack);
}
