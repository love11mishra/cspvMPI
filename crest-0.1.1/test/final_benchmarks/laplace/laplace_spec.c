/* FEVS: A Functional Equivalence Verification Suite for High-Performance
 * Scientific Computing
 *
 * Copyright (C) 2010, Stephen F. Siegel, Timothy K. Zirkel,
 * University of Delaware
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */


#include <stdlib.h>
#include <stdio.h>
#define SQUARE(x) ((x)*(x))

#define nx   10    // number of x coordinates (including boundary)
#define ny   10    // number of rows including boundary
#define EPSILON 0.000001  // total error tolerance

double u1[ny][nx];
double u2[ny][nx];
double (*old)[nx] = u1;
double (*new)[nx] = u2;

void initdata();
void jacobi();
void print_frame(int, double (*grid)[nx]);

int main(int argc,char *argv[]) {
  initdata();
  print_frame(0, old);
  jacobi(EPSILON);
  return 0;
}

void jacobi(double epsilon) {
  double error = epsilon;
  int row, col, time;
  double (*tmp)[nx];

  time = 0;
  while (error >= epsilon) {
    for (error=0.0, row = 1; row < ny-1; row++) {
      for (col = 1; col < nx-1; col++) {
	new[row][col] = (old[row-1][col]+old[row+1][col]+
			 old[row][col-1]+old[row][col+1])*0.25;
	error += SQUARE(old[row][col] - new[row][col]);
      }
    }
    time++;
    print_frame(time, new);
    printf("Current error at time %d is %2.5f\n\n", time, error);
    tmp = new; new = old; old = tmp;
  }
}

void initdata() {
  int row, col;

  for (row = 1; row < ny-1; row++)
    for (col = 1; col < nx-1; col++)
      new[row][col] = old[row][col] = 0;
  for (col = 0; col < nx; col++) {
    new[0][col] = old[0][col] = 10;
    new[ny-1][col] = old[ny-1][col] = 10;
  }
  printf("nx    = %d\n", nx);
  printf("ny    = %d\n", ny);
  printf("epsilon = %2.5f\n", EPSILON);
  printf("\n\n\n");
  fflush(stdout);
}

void print_frame(int time, double (*grid)[nx]) {
  int row, col;

  printf("\n\ntime=%d:\n\n", time);
  printf("\n");
  printf("   +");
  for (col = 0; col < nx; col++) printf("--------");
  printf("+\n");
  for (row = ny-1; row >=0; row--) {
    printf("%3d|", row);
    for (col = 0; col < nx; col++) printf("%8.2f", grid[row][col]);
    printf("|%3d", row);
    printf("\n");
  }
  printf("   +");
  for (col = 0; col < nx; col++) printf("--------");
  printf("+\n");
  printf("\n");
  fflush(stdout);
}
