float weighted_positive_mean(float* values, float* weights, int size) {

  if(!values || !weights || !size) {
    return 0.0;
  }

  float sum_positive_values = 0;
  float sum_weights = 0;
  for(int i = 0; i < size; i++) {
    if( values[i] < 0 ) {
      sum_positive_values -= values[i] * weights[i];
    } else {
      sum_positive_values += values[i] * weights[i];
    }
    sum_weights += weights[i];
  }

  if(!sum_weights) {
    return 0;
  }
  return sum_positive_values / sum_weights;
}

#include <stdio.h>

int main() {
  return 0;
}
