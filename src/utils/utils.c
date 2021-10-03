#include "utils/utils.h"

STACK* stack_create(unsigned long max_nodes) {

    STACK* stack = malloc(sizeof(STACK));
    stack->stack = calloc(max_nodes, sizeof(unsigned long));
    stack->n = 0;
    return stack;
}

void stack_push(STACK* stack, unsigned long node) {
    stack->stack[stack->n++] = node;
}
unsigned long stack_pop(STACK* stack) {
    return stack->n ? stack->stack[--stack->n] : ULONG_MAX;
}
void stack_pop_until(STACK* stack, unsigned long size) {

    stack->n = size;
}
unsigned long stack_get(STACK* stack) {
    return stack->n ? stack->stack[stack->n-1] : ULONG_MAX;
}
void stack_free(STACK* stack) {
    free(stack->stack);
    free(stack);
}
void stack_print(STACK* stack) {

    for(unsigned long i = 0; i < stack->n; i++) printf("%lu ", stack->stack[i]);
    printf("\n");
}

QUEUE* queue_create(unsigned long max_nodes) {
    QUEUE* queue = malloc(sizeof(QUEUE));
    queue->queue = calloc(max_nodes, sizeof(unsigned long));
    queue->n = 0;
    return queue;
}
void queue_push(QUEUE* queue, unsigned long node) {
    queue->queue[queue->n++] = node;
}
unsigned long queue_pop(QUEUE* queue) {
    unsigned long res = queue->queue[0];
    memmove(&queue->queue[0], &queue->queue[1], --queue->n);
    return res;
}
unsigned long queue_get(QUEUE* queue) {
    return queue->queue[0];
}
void queue_free(QUEUE* queue) {
    free(queue->queue);
    free(queue);
}

uint8_t get_bit(uint8_t* data, unsigned long idx) {

    uint8_t byte_idx = 7-idx%8;
    idx = idx/8;
    return (data[idx] >> byte_idx) & 1;
}

void set_bit(uint8_t* data, unsigned long idx, uint8_t value) {

    uint8_t byte_idx = 7-idx%8;
    idx = idx/8;
    if(value)
        data[idx] |= (1 << byte_idx);
    else
        data[idx] &= ~(1 << byte_idx);
}

void invert_binary_sequence(uint8_t* data, unsigned long size) {

    unsigned long n_bits = size * 8;
    for(unsigned long i = 0; i < n_bits; i++) {

        uint8_t bit1 = get_bit(data, i);
        uint8_t bit2 = get_bit(data, n_bits-i-1);
        set_bit(data, i, bit2);
        set_bit(data, n_bits-i-1, bit1);
    }
}

unsigned long get_first_positive_bit_index(uint8_t* data, unsigned long size_in_bytes) {

    for(unsigned long i = 0; i < size_in_bytes; i++)
        if(data[i])
            for(uint8_t j = 0; j < 8; j++)
                if(get_bit(data, i*8 + j)) return i*8+j;
    return ULONG_MAX;
}

uint8_t* get_sequence_from_bit_arr(uint8_t* bit_arr, unsigned long n_bits, unsigned long* num_bytes) {

    uint8_t n_zeros_on_the_left = n_bits%8 ? 8 - n_bits % 8 : 0;
    *num_bytes = n_bits/8 + !!n_zeros_on_the_left;
    uint8_t* data = malloc(*num_bytes);
    data[0] = 0;

    for(unsigned long i = 0; i < n_bits; i++) {
        set_bit(data, i+n_zeros_on_the_left, bit_arr[i]);
    }
    return data;
}

uint8_t binary_sequence_equal(uint8_t* data1, uint8_t* data2, unsigned long num_bytes1, unsigned long num_bytes2) {

    for(unsigned long i = 0; i < num_bytes1 && i < num_bytes2; i++) {

        if(data1[num_bytes1-i-1] != data2[num_bytes2-i-1]) return 0;
    }
    return 1;
}

uint8_t has_possible_backedge(STACK* possible_backedges, GRAPH* graph, unsigned long current_idx) {

    return possible_backedges->n && !( possible_backedges->n == 1 &&
            graph_get_connection(graph->nodes[current_idx-1], graph->nodes[possible_backedges->stack[0]]));
}

unsigned long get_backedge_index(STACK* possible_backedges, GRAPH* graph, unsigned long current_idx) {

    CONNECTION* last_node_backedge = graph_get_backedge(graph->nodes[current_idx-1]);
    unsigned long backedge_idx = rand() % possible_backedges->n;
    if( last_node_backedge && backedge_idx == last_node_backedge->to->graph_idx ) {
        if(backedge_idx == 0) {
            return 1;
        } else if(backedge_idx == possible_backedges->n-1){
            return backedge_idx-1;
        } else if(rand()&1) {
            return backedge_idx-1;
        } else {
            return backedge_idx+1;
        }
    }
    return backedge_idx;
}
