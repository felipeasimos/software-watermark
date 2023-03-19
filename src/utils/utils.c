#include "utils/utils.h"

uint8_t is_little_endian_machine() {

	uint16_t x = 1;

	// if it is little endian, 1 should be the first byte
	return ((uint8_t*)(&x))[0];
}

unsigned long ceil_power_of_2(unsigned long n) {

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    return n;
}

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
    if(!queue->n) return ULONG_MAX;
    unsigned long res = queue->queue[0];
    memmove(&queue->queue[0], &queue->queue[1], --queue->n);
    return res;
}
unsigned long queue_get(QUEUE* queue) {
    return queue->n ? queue->queue[0] : ULONG_MAX;
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
    if(value) {
        data[idx] |= (1 << byte_idx);
    } else {
        data[idx] &= ~(1 << byte_idx);
    }
}

void invert_binary_sequence(uint8_t* data, unsigned long size) {

    unsigned long total_n_bits = size * 8;
    unsigned long half_n_bits = total_n_bits/2;
    for(unsigned long i = 0; i < half_n_bits; i++) {

        uint8_t bit1 = get_bit(data, i);
        uint8_t bit2 = get_bit(data, total_n_bits-i-1);
        set_bit(data, i, bit2);
        set_bit(data, total_n_bits-i-1, bit1);
    }
}

void invert_byte_sequence(uint8_t* data, unsigned long size) {

    unsigned long half_size = size/2;
    for(unsigned long i = 0; i < half_size; i++) {

        uint8_t byte1 = data[i];
        uint8_t byte2 = data[size - i - 1];
        data[size - i - 1] = byte1;
        data[i] = byte2;
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

    uint8_t n_zeros_on_the_left = n_bits%8 ? 8 - (n_bits % 8) : 0;
    *num_bytes = n_bits/8 + !!(n_bits%8);
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

void* encode_numeric_string(char* string, unsigned long* data_len) {

    if( !string ) return NULL;

    unsigned long size = strlen(string);

    unsigned long offset = size % 4;
    *data_len = 3 * (size/4) + offset;
    uint8_t* data = malloc(*data_len);
    memset(data, 0x00, *data_len);

    unsigned long data_idx = offset == 1 ? 2 : 4;
    if(!offset) data_idx = 0;

    for(unsigned long i = 0; i < size; i++) {

        if( string[i] ) {
            for(unsigned long j = 2; j < 8; j++) {

                set_bit(data, data_idx++, get_bit((uint8_t*)string, i*8+j));
            }
        }
    }
    return data;
}

unsigned long get_number_of_left_zeros(uint8_t* data, unsigned long data_len) {
  unsigned long byte_idx = 0;
  while(!data[byte_idx] && byte_idx < data_len) byte_idx++;
  // check if its all zeros
  if(byte_idx == data_len - 1 && !data[byte_idx]) return data_len * 8;
  uint8_t bit_idx = 0;
  while(!get_bit(&data[byte_idx], bit_idx) && bit_idx < 8) bit_idx++;
  return byte_idx * 8 + bit_idx;
}

unsigned long get_number_of_right_zeros(uint8_t* data, unsigned long data_len) {
  unsigned long byte_idx = 0;
  while(!data[data_len - byte_idx - 1] && byte_idx < data_len) byte_idx++;
  byte_idx = data_len - byte_idx - 1;
  // check if its all zeros
  if(byte_idx == 0 && !data[byte_idx]) return data_len * 8;
  uint8_t bit_idx = 0;
  while(!get_bit(&data[byte_idx], 7 - bit_idx) && bit_idx < 8) bit_idx++;
  byte_idx = data_len - byte_idx - 1;
  return byte_idx * 8 + bit_idx;
}

void remove_left_zeros(uint8_t* data, unsigned long* data_len) {
  unsigned long n_left_zeros = get_number_of_left_zeros(data, *data_len);

  unsigned long n_bits = (*data_len) * 8;

  // iterate over non-left-zero part of the sequence, throwing its values
  // back to the beginning
  unsigned long cursor = 0;
  for(unsigned long i = n_left_zeros; i < n_bits; i++) {
    set_bit(data, cursor, get_bit(data, i));
    cursor++;
  }
  // calculate new data_len
  *data_len = (n_bits - n_left_zeros) / 8 + !!n_left_zeros;
}

void add_left_zeros(uint8_t** data, unsigned long* data_len, unsigned long num_zeros) {
  unsigned long old_n_bits = (*data_len) * 8;
  unsigned long new_n_bits = num_zeros + old_n_bits;
  unsigned long new_n_bytes = new_n_bits / 8 + !!(new_n_bits % 8);
  new_n_bits = new_n_bytes * 8;

  uint8_t* new_data = malloc(new_n_bytes);
  memset(new_data, 0x00, new_n_bytes);

  for(unsigned long i = 0; i < old_n_bits; i++) {
    set_bit(new_data, num_zeros + i, get_bit(*data, i));
  }
  free(*data);
  *data = new_data;
  *data_len = new_n_bytes;
}

void merge_arr(void* data, unsigned long* data_len, unsigned long element_size, unsigned long symbol_size) {

  unsigned long num_elements = (*data_len)/element_size;
  unsigned long next_bit = 0;
  unsigned long offset = element_size * 8 - symbol_size;
  for(unsigned long i = 0; i < num_elements; i++) {
    void* element = ((uint8_t*)data)+(element_size * i);
    // iterate over element bits
    for(unsigned long j = 0; j < symbol_size; j++) {
      // shift element's bit to where next_bit is pointing to
      set_bit(data, next_bit, get_bit(element, offset + j));
      next_bit++;
    } 
  }
  // update 'data_len' with size of the new sequence
  *data_len = next_bit / 8 + !!(next_bit % 8);
  // set any bits left to zero
  unsigned long n_bits = *data_len * 8;
  for(; next_bit < n_bits; next_bit++) {
    set_bit(data, next_bit, 0);
  }

}

void unmerge_arr(void* data, unsigned long num_symbols, unsigned long element_size, unsigned long symbol_size, void** res) {

  // iterate over 'new_data' elements
  unsigned long symbol_bytes = (symbol_size / 8) + !!(symbol_size % 8);
  unsigned long res_size = (num_symbols) * symbol_bytes;
  unsigned long next_bit = 0;
  if(!*res) {
    *res = malloc(res_size);
  }
  memset(*res, 0x00, res_size);
  unsigned long offset = element_size * 8 - symbol_size;
  for(unsigned long i = 0; i < num_symbols; i ++) {
    void* element = ((uint8_t*)*res)+(element_size * i);
    // iterate over symbol bits and populate element with symbol bits
    for(unsigned long j = 0; j < symbol_size; j++) {
      set_bit(element, offset + j, get_bit(data, next_bit));
      next_bit++;
    }
  }
}

void* decode_numeric_string(void* data, unsigned long* data_len) {

    unsigned long num_bits = *data_len * 8;
    uint8_t offset = get_first_positive_bit_index(data, *data_len);
    unsigned long str_size = 4 * ((*data_len - offset/8)/3) + ((*data_len - offset/8)% 3);

    if( !(str_size % 4) && offset ) str_size--;
    uint8_t* str = malloc(str_size);
    memset(str, 0x00, str_size);

    unsigned long str_idx = 2;

    for(unsigned long i = offset; i < num_bits && str_idx < str_size * 8; i++) {

        set_bit((uint8_t*)str, str_idx++, get_bit(data, i));

        if( !(str_idx % 8) ) str_idx+=2;
    }
    *data_len = str_size;
    return str;
}

uint8_t has_possible_backedge(STACK* possible_backedges, GRAPH* graph, unsigned long current_idx) {

    // new change to condition (ii) for backedges:
    // old: there can't be a backedge [w <- v] if [w <- v-1] exists
    // new: there can't be a backedge [w <- v] if [w' <- v-1] exists
    // watermarks like these shouldn't be valid:
    // 0b1011 = 11
    //   .---------.
    //  /   .----.  \
    // v    v    |   \
    // 0 -> 1 -> 2 -> 3 -> 4 -> 5
    // since in the dijkstra graphs article it is pretty clear that
    // the middle node of a repeat block is not expandable:
    // .----.
    // v    |
    // X -> R -> X
    // However, in the example above, node 3 clearly part of the inner
    // repeat block. Changing [w <- v] to [w' <- v-1] corrects this.
    // while blocks are not affected since they always have the same static structure
    // which doesn't allow for inner backedges:
    // .----.
    // v    |
    // R -> X   
    //  `------> X
    if(graph_get_backedge(graph->nodes[current_idx-1])) return 0;

    return possible_backedges->n && !( possible_backedges->n == 1 &&
            graph_get_connection(graph->nodes[current_idx-1], graph->nodes[possible_backedges->stack[0]]));
}

unsigned long get_backedge_index(STACK* possible_backedges, GRAPH* graph, unsigned long current_idx) {

    CONNECTION* last_node_backedge = graph_get_backedge(graph->nodes[current_idx-1]);
    unsigned long backedge_idx = rand() % possible_backedges->n;
    if( last_node_backedge && backedge_idx == last_node_backedge->node->graph_idx ) {
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
