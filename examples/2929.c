// 45 -> 2969
#include <math.h>
#include <stdio.h>

#define POINTS_LEN 50
#define NUM_ITERATIONS 20

// perform K means algorithm with 2 clusters and return the mean variance
float linear_kmeans2(float *points, float *clusters) {
  unsigned long step = 0;
  int ownership[POINTS_LEN];
  float distance0[POINTS_LEN], distance1[POINTS_LEN];
  do {
    // iterate over points for first cluster
    unsigned long i = 0;
    do {
      distance0[i] = fabs(clusters[0] - points[i]);
    } while (++i < POINTS_LEN);
    // iterate over points for second cluster
    // assign point to cluster
    i = 0;
    do {
      distance1[i] = fabs(clusters[1] - points[i]);
      ownership[i] = 0;
      if (distance1[i] < distance0[i]) {
        ownership[i] = 1;
      }
    } while (++i < POINTS_LEN);
    // recalculate cluster position
    clusters[0] = clusters[1] = 0;
    for (unsigned long i = 0; i < POINTS_LEN; i++) {
      clusters[ownership[i]] += points[i];
    }
    clusters[0] /= POINTS_LEN;
    clusters[1] /= POINTS_LEN;
  } while (++step < NUM_ITERATIONS);
  // find variance
  float sum0 = 0, sum1 = 0;
  unsigned long i = 0;
  unsigned long num_cluster1_points = 0;
  do {
    sum0 += powf(clusters[0] - (points[i] * !ownership[i]), 2);
    sum1 += powf(clusters[1] - (points[i] * ownership[i]), 2);
    num_cluster1_points += ownership[i];
  } while (++i < POINTS_LEN);
  unsigned long num_cluster0_points = POINTS_LEN - num_cluster1_points;
  sum0 /= num_cluster0_points + !num_cluster0_points;
  sum1 /= num_cluster1_points + !num_cluster1_points;
  return (sum0 + sum1) / 2;
}

#include <stdlib.h>

int main() {

  srand(1);
  float clusters[2] = {0, 0.7};
  float points[POINTS_LEN];
  for (unsigned long i = 0; i < POINTS_LEN; i++) points[i] = (float)rand() / (float)RAND_MAX;
  points[0] = 0.5;
  points[1] = 1;
  float mean_variance = linear_kmeans2(points, clusters);
  printf("mean variance: %f\n", mean_variance);

  return 0;
}
