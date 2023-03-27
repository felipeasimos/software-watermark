// 45 -> 2969
#include <math.h>
#include <stdio.h>

// perform K means algorithm with 2 clusters and return the mean variance
float linear_kmeans2(float* points, float* clusters, unsigned long points_len, unsigned long num_iterations) {
  unsigned long step = 0;
  int ownership[points_len];
  float distance0[points_len], distance1[points_len];
  // compute Kmeans K times
  for(unsigned long step = 0; step < num_iterations; step++) {
    float next_cluster0 = 0, next_cluster1 = 0;
    // iterate over points, find closest cluster and prepare to update it
    for(unsigned long i = 0; i < points_len; i++) {
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
    clusters[1] = next_cluster1/points_len;
    clusters[0] = next_cluster0/points_len;
  }
  // find variance
  float sum0 = 0, sum1 = 0;
  unsigned long num_cluster1_points = 0;
  for(unsigned long i = 0; i < points_len; i++) {
    if(ownership[i]) {
      sum1 += powf(clusters[1] - points[i], 2);
      num_cluster1_points++;
    } else {
      sum0 += powf(clusters[0] - points[i], 2);
    }
  }
  unsigned long num_cluster0_points = points_len - num_cluster1_points;
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
  unsigned long points_len = 2;
  float clusters[2] = { 0, 0.7 };
  float points[points_len];
  for(unsigned long i = 0; i < points_len; i++) points[i] = (float)rand() / (float)RAND_MAX;
  points[0] = 0.5;
  points[1] = 1;
  float mean_variance = linear_kmeans2(points, clusters, points_len, 1);
  printf("mean variance: %f\n", mean_variance);

  return 0;
}
