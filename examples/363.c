float weighted_positive_mean(float* values, float* weights, int size) {

  if(!values || !weights || !size) {
    return 0.0;
  }

  float sum_positive_values = 0;
  int i = 0;
  do {
    if(values[i] < 0) {
      sum_positive_values -= values[i] * weights[i];
      continue;
    }
    sum_positive_values += values[i] * weights[i];
  } while(++i < size);

  float sum_weights = i = 0;
  do {
    sum_weights += weights[i];
  } while(++i < size);

  if(!sum_weights) {
    return 0;
  }
  return sum_positive_values / sum_weights;
}

#include <stdio.h>

int main() {
  return 0;
}
