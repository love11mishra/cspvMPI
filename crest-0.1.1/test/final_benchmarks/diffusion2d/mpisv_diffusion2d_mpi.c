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

/* diffusion2d.c: parallel 2d-diffusion equation solver with 
 * constant boundaries slicing matrix as a checker board. 
 * Initial values are assigned by a group of constant values
 *
 * To execute: mpicc diffusion2d.c ; mpiexec -n 4 ./a.out 2 2
 * To verify: civl verify diffusion2d.c
 */
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<string.h>
#include<mpi.h>

/* Message tags */
#define DATAPASS   1
#define comm MPI_COMM_WORLD

/* Global variables */
long nx, ny;
int nsteps, wstep;
int NPROCSX, NPROCSY;
double constTemp;            // value of constant boundaries for test
double initTemp;             // value of initial temperature for test
double k;
int nprocs, rank;
int left, right, top, bottom;// ranks of neighbors
int nxl;                     // local dimension of x
int nyl;                     // local dimension of y
long firstCol;               // index of the first column in global array
long firstRow;               // index of the first row in global array
/* dynamically-allocated 2d array of doubles specifying state of local
 * grid in current time step. Dimensions are u_curr[nyl+2][nxl+2].
 */
double ** u_curr;
/* dynamically-allocated 2d array of doubles specifying state of local
 * grid in next time step. Dimensions are u_next[nyl+2][nxl+2].
 */
double ** u_next;
/* dynamically-allocated 1d temporary buffer. Length is recvbuf[nxl+1] 
 */
double * recvbuf;

/* The processes are arranged geometrically as follows for the case
 * NPROCSX = 3:
 * row 0: 0 1 2
 * row 1: 3 4 5 
 * ...         
 * This function computes the global index of the first column 
 * owned by the process of the given rank. rank must be in the 
 * range [0, _NPROCS-1]. The result will in the range [0, nx-1].
 */
long firstColForProc(int rank) {
  long tmp = (rank - (rank / NPROCSX)*NPROCSX)*nx;

  return tmp/NPROCSX;
}

/* This function computes the global index of the first row owned by
   the process of the given rank. rank must be in the range[0,
   _NPROCS-1]. The result will in the range [0, ny-1]. */
long firstRowForProc(int rank) {
  long tmp = ((rank / NPROCSX)*ny);

  return tmp/NPROCSY;
}

/* Computes the number of columns owned by the process. The result
   will be in the range [0, nx]. */
int countColForProc(int rank) {
  long a = firstColForProc(rank);
  long b;

  if ((rank / NPROCSX) == ((rank+1) / NPROCSX))
    b = firstColForProc(rank+1);
  else
    b = nx;
  return b - a;
}

/* Computes the number of rows owned by the process. The result
   will be in the range [0, ny]. */
int countRowForProc(int rank) {
  long a = firstRowForProc(rank);
  long b = firstRowForProc(rank+NPROCSX);

  return b - a;
}

/* Get the owner process of the given cell */
int OWNER(long col, long row) {
  long tmp = ((NPROCSY * (row+1))-1);
  int procRow = tmp / ny;
  int procCol;

  tmp = ((col + 1) * NPROCSX - 1);
  procCol  = tmp / nx;
  tmp = procRow * NPROCSX;
  return tmp + procCol;
}


/* initialize all data values owned by each process */
void initData() {
  // All boundary cells are set to the same value constTemp.
  // All cells in the interior region are set to the same value 
  // initTemp.
  for (int i=0; i < nyl+2; i++)
    for (int j=0; j < nxl+2; j++)
      if (i == 0 || j == 0 || i == nyl+1 || j == nxl+1)
        u_next[i][j] = u_curr[i][j] = constTemp;
      else
        u_curr[i][j] = initTemp;
}

/* Initialize all global variables, allocate memory spaces for
 * pointers and proc 0 will do a sequential run. The results of the
 * sequential run will be used to compare with parallel run later. */
void initialization(int argc, char * argv[]) {
  nsteps = 5;//150;
  wstep = 5;//10;
  nx = 4;//8;
  ny = 4;//8;
  if (argc != 3) {
    printf("Usage: mpiexec -n NPROCS diffusion2d NPROCSX NPROCSY\n"
           "  NPROCSX: number of processes in x direction\n"
           "  NPROCSY: number of processes in y direction\n"
           "  NPROCS: the product of NPROCSX and NPROCSY\n");
    exit(1);
  }
  NPROCSX = atoi(argv[1]); 
  NPROCSY = atoi(argv[2]);
  assert(NPROCSX * NPROCSY == nprocs);
  constTemp = 0.0;
  initTemp = 100.0;
  k = 0.13;
  if(!rank)
    printf("Diffusion2d with k=%f, nx=%ld, ny=%ld, nsteps=%d, wstep=%d\n",
	   k, nx, ny, nsteps, wstep);
  nxl = countColForProc(rank);
  nyl = countRowForProc(rank);
  if (rank == 0)
    recvbuf = (double *)malloc((nxl + 1) * sizeof(double));
  u_curr = (double **)malloc((nyl + 2) * sizeof(double *));
  assert(u_curr);
  u_next = (double **)malloc((nyl + 2) * sizeof(double *));
  assert(u_next);
  for (int i=0; i < nyl + 2; i++){
    u_curr[i] = (double *)malloc((nxl + 2) * sizeof(double));
    assert(u_curr[i]);
    u_next[i] = (double *)malloc((nxl + 2) * sizeof(double));
    assert(u_next[i]);
  }
  firstCol = firstColForProc(rank);
  firstRow = firstRowForProc(rank);
  // computes neighbors
  if (firstCol != 0)
    left = OWNER(firstCol - 1, firstRow);
  else
    left = MPI_PROC_NULL;
  if (firstRow != 0)
    top = OWNER(firstCol, firstRow - 1);
  else
    top = MPI_PROC_NULL;
  if (firstCol + nxl < nx)
    right = OWNER(firstCol + nxl, firstRow);
  else
    right = MPI_PROC_NULL;
  if (firstRow + nyl < ny)
    bottom = OWNER(firstCol, firstRow + nyl);
  else
    bottom = MPI_PROC_NULL;
}

/* Update local cells owned by process */
void update() {
  double **tmp;

  for (int i = 1; i < nyl + 1; i++)
    for (int j = 1; j < nxl + 1; j++) {
      u_next[i][j] = u_curr[i][j] +
        k*(u_curr[i+1][j] + u_curr[i-1][j] + 
           u_curr[i][j+1] + u_curr[i][j-1] - 4*u_curr[i][j]);
    }
  // swap two pointers
  tmp = u_curr;
  u_curr = u_next;
  u_next = tmp;
}

/* Exchange ghost cells between proceeses */
void exchange() {
  double sendbuf[nyl];
  double recvbuf[nyl];

  // sends top border row, receives into bottom ghost cell row
  MPI_Sendrecv(&u_curr[1][1], nxl, MPI_DOUBLE, top, 0, &u_curr[nyl+1][1], nxl, 
               MPI_DOUBLE, bottom, 0, comm, MPI_STATUS_IGNORE);
  // sends bottom border row, receives into top ghost cell row
  MPI_Sendrecv(&u_curr[nyl][1], nxl, MPI_DOUBLE, bottom, 0, &u_curr[0][1], nxl, 
               MPI_DOUBLE, top, 0, comm, MPI_STATUS_IGNORE);
  // sends left border column, receives into temporary buffer
  for (int i = 0; i < nyl; i++) sendbuf[i] = u_curr[i+1][1];
  MPI_Sendrecv(sendbuf, nyl, MPI_DOUBLE, left, 0, recvbuf, nyl, 
               MPI_DOUBLE, right, 0, comm, MPI_STATUS_IGNORE);
  // copies temporary buffer into right ghost cell column
  if (right != MPI_PROC_NULL)
    for (int i = 0; i < nyl; i++) u_curr[i+1][nxl+1] = recvbuf[i];
  // sends right border column, receives into temporary buffer
  for (int i = 0; i < nyl; i++) sendbuf[i] = u_curr[i+1][nxl];
  MPI_Sendrecv(sendbuf, nyl, MPI_DOUBLE, right, 0, recvbuf, nyl, 
               MPI_DOUBLE, left, 0, comm, MPI_STATUS_IGNORE);
  // copies temporary buffer into left ghost cell column
  if (left != MPI_PROC_NULL)
    for (int i = 0; i < nyl; i++) u_curr[i+1][0] = recvbuf[i];
}

/* Helper function for write_frame(int). In CIVL mode, it takes the
   index of the first column, the number of columns owned by the
   process and the index of current row to locate the corresponding
   cell in oracle and compare it with the given cell's value computed
   in parallel */
void printData(int time, int firstCol, int nxl, int currRow, double * buf) {
  for (int i=0; i<nxl; i++)
    printf("%6.2f ", *(buf + i));
}

/* Print the computed matrix at the given time step all processes
 * should send their local data to process rank 0 which is responsible
 * for printing */
void write_frame(int time) {
  // sends data row by row
  if (rank != 0) {
    for (int i=0; i<nyl; i++)
      MPI_Send(&u_curr[i+1][1], nxl, MPI_DOUBLE, 0, DATAPASS, comm);
  } else {
    printf("\n-------------------- time step:%d --------------------\n", time);
    for (int i=0; i < NPROCSY; i++) {
      int numRows = countRowForProc(i*NPROCSX);

      for (int j=0; j < numRows; j++) {
        for (int k=0; k < NPROCSX; k++) {
          int curr_rank = i*NPROCSX + k;

          if (curr_rank!=0) {
            int senderx = firstColForProc(curr_rank);
            int sendery = firstRowForProc(curr_rank);
            int senderNxl = countColForProc(curr_rank);

            MPI_Recv(recvbuf, senderNxl, MPI_DOUBLE, curr_rank, DATAPASS, comm, MPI_STATUS_IGNORE);
            printData(time, senderx, senderNxl, sendery+j, recvbuf);
          } else 
            printData(time, firstCol, nxl, firstRow+j, &u_curr[j+1][1]);
        }
        printf("\n");
      }
    }
  } 
}

int main(int argc, char * argv[]) {
  int i,j;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &nprocs);
  initialization(argc, argv);
  initData();
  for (i=0; i<=nsteps; i++) {
    if (i%wstep == 0)
      write_frame(i);
    if (nxl != 0 && nyl != 0) {
      exchange();
      update();
    }
  }
  for (i=0; i<nyl+2; i++) {
    free(u_curr[i]);
    free(u_next[i]);
  }
  free(u_curr);
  free(u_next);
  if (rank == 0)
    free(recvbuf);
  return 0;
}
