#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

double f(double x) {
  return x * x - 9;
}

double f_slope(double x) {
  return 2 * x;
} 

void newton_raphson() {

  FILE* file = fopen("nr_before.txt", "w");
  double epsilon = 0.0005;
  double x = 0.5;
  double y = epsilon+1;
  unsigned long max_iter = 1000;
  unsigned long i = 0;
  while(i < max_iter && fabs(y) > epsilon) {
    y = f(x);
    double slope = f_slope(x); 
    fprintf(
        file,
        "%F %F %F\n",
        x,
        y,
        slope);
    x -= y/slope;
    i++;
  }
  fclose(file);
}

int main() {
  newton_raphson();
}
