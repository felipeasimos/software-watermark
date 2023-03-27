// 45 -> 2969
#include <math.h>
#include <stdio.h>

// perform K means algorithm with 2 clusters and return the mean variance
float linear_kmeans2(float* points, float* clusters, unsigned long points_len, unsigned long k) {
  unsigned long step = 0;
  float distance0[points_len], distance1[points_len];
  int ownership[points_len];
  do {
    // iterate over points for first cluster
    unsigned long i = 0;
    do {
      distance0[i] = fabs(clusters[0] - points[i]);
    } while(++i < points_len);
    // iterate over points for second cluster
    // assign point to cluster
    i = 0;
    do {
      distance1[i] = fabs(clusters[1] - points[i]);
      ownership[i] = 0;
      if(distance1[i] < distance0[i]) {
        ownership[i] = 1;
      }
    } while(++i < points_len);
    // recalculate cluster position
    clusters[0] = clusters[1] = 0;
    for(unsigned long i = 0; i < points_len; i++) {
      clusters[ownership[i]] += points[i];
    }
    clusters[0] /= points_len;
    clusters[1] /= points_len;
  } while(++step < k);
  // find variance
  float sum0 = 0, sum1 = 0;
  unsigned long i = 0;
  unsigned long num_cluster1_points = 0;
  do {
    sum0 += powf(clusters[0] - (points[i] * !ownership[i]), 2);
    sum1 += powf(clusters[1] - (points[i] * ownership[i]), 2);
    num_cluster1_points += ownership[i];
  } while(++i < points_len);
  unsigned long num_cluster0_points = points_len - num_cluster1_points;
  sum0 /= num_cluster0_points + !num_cluster0_points;
  sum1 /= num_cluster1_points + !num_cluster1_points;
  return (sum0 + sum1) / 2;
}

#include <stdlib.h>

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
