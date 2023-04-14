// 45 -> 2969
#include <math.h>
#include <stdio.h>

#define POINTS_LEN 50
#define NUM_ITERATIONS 25

// perform K means algorithm with 2 clusters and return the mean variance
float linear_kmeans2(float* points, float* clusters) {
  unsigned long step = 0;
  int ownership[POINTS_LEN];
  float distance0[POINTS_LEN], distance1[POINTS_LEN];
  // compute Kmeans K times
  for(unsigned long step = 0; step < NUM_ITERATIONS; step++) {
    float next_cluster0 = 0, next_cluster1 = 0;
    // iterate over points, find closest cluster and prepare to update it
    for(unsigned long i = 0; i < POINTS_LEN; i++) {
      distance0[i] = fabs(clusters[0] - points[i]);
      distance1[i] = fabs(clusters[1] - points[i]);
      ownership[i] = distance1[i] < distance0[i];
      if(ownership[i]) {
        next_cluster1 += points[i];
      } else {
        next_cluster0 += points[i];
      }
    }
    // update clusters
    clusters[1] = next_cluster1/POINTS_LEN;
    clusters[0] = next_cluster0/POINTS_LEN;
  }
  // find variance
  float sum0 = 0, sum1 = 0;
  unsigned long num_cluster1_points = 0;
  for(unsigned long i = 0; i < POINTS_LEN; i++) {
    if(ownership[i]) {
      sum1 += powf(clusters[1] - points[i], 2);
      num_cluster1_points++;
    } else {
      sum0 += powf(clusters[0] - points[i], 2);
    }
  }
  unsigned long num_cluster0_points = POINTS_LEN - num_cluster1_points;
  sum0 /= num_cluster0_points;
  sum1 /= num_cluster1_points;
  if(isnanf(sum0)) sum0 = 0;
  if(isnanf(sum1)) sum1 = 0;
  return (sum0 + sum1) / 2;
}

#include <stdlib.h>
#include <stdio.h>

int main() {

  srand(1);
  float clusters[2] = { 0, 0.7 };
  float points[POINTS_LEN];
  for(unsigned long i = 0; i < POINTS_LEN; i++) points[i] = (float)rand() / (float)RAND_MAX;
  points[0] = 0.5;
  points[1] = 1;
  float mean_variance = linear_kmeans2(points, clusters);
  printf("mean variance: %f\n", mean_variance);

  return 0;
}
