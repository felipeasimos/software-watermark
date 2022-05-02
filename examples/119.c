#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

typedef struct NR_ITERATION {
  double x;
  double y;
  double slope;
} NR_ITERATION;

double f(double x) {
  return x * x - 9;
}

double f_slope(double x) {
  return 2 * x;
} 

void newton_raphson() {

  FILE* file = fopen("nr.txt", "w");
  double initial_x = 0.5;
  double epsilon = 0.0005;
  int max_iter = 1000;
  NR_ITERATION iters[max_iter];
  int i = 0;
  do {
    iters[i].x = 0;
    iters[i].y = 0;
    iters[i].slope = DBL_MAX;
  } while( ++i < max_iter );
  i = 0;
  iters[0].x = initial_x;
  do {
    iters[i].y = f(iters[i].x);
    iters[i].slope = f_slope(iters[i].x);
    if( i + 1 < max_iter ) {
      double diff = iters[i].y/iters[i].slope;
      iters[i+1].x = iters[i].x - diff; 
    }
    i++;
  } while( i < max_iter &&
      fabs(iters[i-1].y) > epsilon );
  i = 0;
  while( i < max_iter &&
      iters[i].slope != DBL_MAX ) {
 
    fprintf(
        file,
        "%F %F %F\n",
        iters[i].x,
        iters[i].y,
        iters[i].slope);
    i++;
  }
  fclose(file);
}

int main() {
  newton_raphson();
}
