/* FEVS: A Functional Equivalence Verification Suite for High-Performance
 * Scientific Computing
 *
 * Copyright (C) 2007-2010, Andrew R. Siegel, Stephen F. Siegel,
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

#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<string.h>

long nx, ny;
int nsteps, wstep;
double constTemp;    // value of constant boundaries for test
double initTemp;     // value of initial temperature for test
double k;
double **u_curr, **u_next;

void initData() {
  for (int i=0; i < ny+2; i++)
    for (int j=0; j < nx+2; j++)
      if (i == 0 || j == 0 || i == ny+1 || j == nx+1)
        u_next[i][j] = u_curr[i][j] = constTemp;
      else
        u_curr[i][j] = initTemp;
}

void initialization() {
  nsteps = 150;
  wstep = 10;
  nx = 8;
  ny = 8;
  constTemp = 0.0;
  initTemp = 100.0;
  k = 0.13;
  printf("Diffusion2d with k=%f, nx=%ld, ny=%ld, nsteps=%d, wstep=%d\n",
	 k, nx, ny, nsteps, wstep);
  u_curr = (double **)malloc((ny+2) * sizeof(double *));
  u_next = (double **)malloc((ny+2) * sizeof(double *));
  for(int i=0; i < ny+2; i++) {
    u_curr[i] = (double *)malloc((nx+2) * sizeof(double));
    u_next[i] = (double *)malloc((nx+2) * sizeof(double));
  }
}

void update() {
  double ** tmp;

  for(int i=1; i < ny+1; i++)
    for(int j=1; j < nx+1; j++) 
      u_next[i][j] = u_curr[i][j] +
        k*(u_curr[i+1][j] + u_curr[i-1][j] + 
           u_curr[i][j+1] + u_curr[i][j-1] - 4*u_curr[i][j]);
  // swap two pointers
  tmp = u_curr;
  u_curr = u_next;
  u_next = tmp;
}

void write_plain(int time) {
  printf("\n-------------------- time step:%d --------------------\n", 
	 time);
  for(int i = 1; i <= ny; i++) {
    for(int j = 1; j <= nx; j++)
      printf("%6.2f ", u_curr[i][j]);
    printf("\n");
  }
}

int main(int argc, char * agrv[]) {
  int i,j;

  initialization();
  initData();
  for (i=0; i<=nsteps; i++) {
    if (i%wstep == 0)
      write_plain(i);
    update();
  }
  for (i=0; i<ny; i++) {
    free(u_curr[i]);
    free(u_next[i]);
  }
  free(u_curr);
  free(u_next);
  return 0;
}
