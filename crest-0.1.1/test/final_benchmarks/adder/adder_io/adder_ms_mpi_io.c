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
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/* MPI message passing tags */
#define MSG_DATA 100
#define MSG_RESULT 101
 
void master (void);
void slave (void);
int n;

void quit() {
  printf("Reading data file failure!\n");
  MPI_Abort(MPI_COMM_WORLD, 0);
}
 
int main (int argc, char **argv) {
  int PID;

  if(argc != 2) {
    printf("The program needs one argument to specify "
	   "the length of the array\n");
    return 0;
  }
  n = atoi(argv[1]);
  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &PID);
  if (!PID)
    master ();
  else
    slave ();
  MPI_Finalize ();
  return 0;
}
 
void master (void) {
  double array[n];
  double sum, tmpsum;
  unsigned long long step, i;
  int NPROCS;
  FILE *fp = fopen("../data","r");
  FILE *out = fopen("parout","w");


  MPI_Comm_size (MPI_COMM_WORLD, &NPROCS);
  //Initialization of the array
  for(i=0; i<n; i++) 
    if(fscanf(fp, "%lf", &array[i]) != 1) quit();
  fclose(fp);
  if(NPROCS != 1)
    step = n / (NPROCS - 1);
  //The array is divided by the number of slaves
  for (i = 0; i < NPROCS - 1; i++)
    MPI_Send (array + i * step, step, MPI_DOUBLE, i + 1, MSG_DATA, MPI_COMM_WORLD);
  //The master completes the work if necessary
  for (i = (NPROCS-1) * step, sum = 0; i < n; i++)
    sum += array[i];
  //The master receives the results in any order
  for (i = 1; i < NPROCS; sum += tmpsum, i++)
    MPI_Recv (&tmpsum, 1, MPI_DOUBLE, MPI_ANY_SOURCE, MSG_RESULT, MPI_COMM_WORLD, 
	      MPI_STATUS_IGNORE);
  fprintf(out, "%8.4lf\n", sum);
  fclose(out);
}

void slave (void) {
  double array[n];
  double localSum;
  unsigned long long i;
  int count;
  MPI_Status status;

  MPI_Recv (array, n, MPI_DOUBLE, 0, MSG_DATA, MPI_COMM_WORLD, &status);
  //The slave finds the size of the array
  MPI_Get_count (&status, MPI_DOUBLE, &count);
  for (i = 0, localSum = 0; i < count; i++)
    localSum += array[i];
  MPI_Send (&localSum, 1, MPI_DOUBLE, 0, MSG_RESULT, MPI_COMM_WORLD);
}
