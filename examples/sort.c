#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void* get_element(void* arr, unsigned long element_size, unsigned long idx) {
  return ((char*)arr)+(element_size * idx);
}

void swap(void* a, void* b, unsigned long element_size) {
  if(a == b) return;
  char tmp[element_size];
  memcpy(tmp, a, element_size);
  memcpy(a, b, element_size);
  memcpy(b, tmp, element_size);
}

void quicksort(void* arr, unsigned long element_size, unsigned long num_elements, int (*cmp_func)(void*, void*)) {

  if(num_elements < 2) return;

  unsigned long subsequence_idxs[65];
  unsigned long subsequence_sizes[65];
  unsigned long num_levels = 0;

  // first subsequence is the entire array
  subsequence_idxs[num_levels] = 0;
  subsequence_sizes[num_levels++] = num_elements;
  while( num_levels ) {
    unsigned long current_subsequence_idx = subsequence_idxs[num_levels-1];
    void* current_subsequence = get_element(arr, element_size, current_subsequence_idx);
    unsigned long current_subsequence_num_elements = subsequence_sizes[num_levels-1];

    unsigned long lower_index = 1;
    unsigned long i = 1;
    while( i < current_subsequence_num_elements ) {
      if( cmp_func(current_subsequence, get_element(current_subsequence, element_size, i)) == -1 ) {
        swap(get_element(current_subsequence, element_size, lower_index++), get_element(current_subsequence, element_size, i), element_size);
      }
      i++;
    }
    swap(current_subsequence, get_element(current_subsequence, element_size, lower_index-1), element_size);

    // here we would add recursive calls like these:
    // * quicksort(arr, element_size, lower_index, cmp_func);
    // * quicksort(get_element(arr, element_size, lower_index), element_size, num_elements - lower_index, cmp_func);
    // instead, we will pop the current arguments for size and index from the 'stacks' and
    // add the indices and size of these partitions to the arrays:
    num_levels--; // pop current subsequence
    // add lower partition
    if( lower_index - 1 > 1 ) { // only add if size > 1

      // we don't change 'subsequence_idxs[num_levels]', since the beginning of the
      // current subsequence is already the beginning of this one
      subsequence_sizes[num_levels++] = lower_index - 1;
    }
    // add upper partition
    if( current_subsequence_num_elements - lower_index > 1 ) { // only add if size > 1
  
      subsequence_idxs[num_levels] = lower_index + current_subsequence_idx;
      subsequence_sizes[num_levels++] = current_subsequence_num_elements - lower_index; 
    }
  }
}

void merge(void* arr, unsigned long element_size, unsigned long first_block_size, unsigned long num_elements, int (*cmp_func)(void*, void*)) {

  char merge_buf[num_elements * element_size];
  unsigned long lower_pointer = 0;
  unsigned long upper_pointer = 0;
  unsigned long half = first_block_size;
  unsigned long half_upper = num_elements - half;

  // merge lower and upper half of this subsequence
  unsigned long element_id = 0;
  while( element_id < num_elements ) {
  
    if( upper_pointer == half_upper ) {
      // copy lower pointer
      memcpy(get_element(merge_buf, element_size, element_id), get_element(arr, element_size, lower_pointer++), element_size);
    } else if( lower_pointer == half ) {
      // copy upper pointer
      memcpy(get_element(merge_buf, element_size, element_id), get_element(arr, element_size, half + upper_pointer++), element_size);
    } else if( cmp_func(get_element(arr, element_size, lower_pointer), get_element(arr, element_size, half + upper_pointer)) == 1) {
      // copy lower pointer
      memcpy(get_element(merge_buf, element_size, element_id), get_element(arr, element_size, lower_pointer++), element_size);
    } else {
      // copy upper pointer
      memcpy(get_element(merge_buf, element_size, element_id), get_element(arr, element_size, half + upper_pointer++), element_size);
    }
    element_id++;
  }
  // copy merge buffer
  memcpy(arr, merge_buf, element_size * num_elements);
}

void mergesort(void* arr, unsigned long element_size, unsigned long num_elements, int (*cmp_func)(void*, void*)) {

  if(num_elements < 2) return;

  // iterate over subsequence size
  unsigned long subsequence_size = 2;
  while(subsequence_size < num_elements) {

    unsigned long num_subsequences = num_elements / subsequence_size;

    // iterate over subsequences
    unsigned long subsequence_id = 0;
    while( subsequence_id < num_subsequences) {
      unsigned long subsequence_offset = subsequence_id * subsequence_size;

      merge(get_element(arr, element_size, subsequence_offset), element_size, subsequence_size >> 1, subsequence_size, cmp_func);
      subsequence_id++;
    }
    // add any elements left and merge them to the last subsequence
    unsigned long rest = num_elements % subsequence_size;
    if( rest ) {
      merge(get_element(arr, element_size, (num_subsequences - 1) * subsequence_size), element_size, subsequence_size, subsequence_size + rest, cmp_func);
    }
    subsequence_size <<= 1;
  }
}

void bubblesort(void* arr, unsigned long element_size, unsigned long num_elements, int (*cmp_func)(void*, void*)) {

  int swap_happened = 1;
  while(swap_happened) {
    swap_happened = 0;
    unsigned long i = 1;
    while( i < num_elements ) {
      if( cmp_func(get_element(arr, element_size, i-1), get_element(arr, element_size, i)) == -1) {
        swap(get_element(arr, element_size, i-1), get_element(arr, element_size, i), element_size);
        swap_happened = 1;
      }
      i++;
    }
  }
}

int cmp_int(void* a, void* b) {
  if(*(int*)a < *(int*)b) {
    return 1;
  } else if(*(int*)a == *(int*)b) {
    return 0;
  } else {
    return -1;
  }
}

int main() {

  int arr[] = {6, 5, 0, 8, 7, 1, 3, 4, 9, 2};
  // int arr[] = {0, 4, 2, 1, 3};

  unsigned long element_size = sizeof(arr[0]);
  unsigned long num_elements = sizeof(arr)/sizeof(arr[0]);

  for(unsigned long i = 0; i < num_elements; i++) {
    printf("%d ", arr[i]);
  }
  printf("\n");
  quicksort(arr, element_size, num_elements, cmp_int);

  for(unsigned long i = 0; i < num_elements; i++) {
    printf("%d ", arr[i]);
  }
  printf("\n");
}
