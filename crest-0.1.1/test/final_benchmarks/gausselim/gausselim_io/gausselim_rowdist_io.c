/* FEVS: A Functional Equivalence Verification Suite for High-Performance
 * Scientific Computing
 *
 * Copyright (C) 2004-2010, Stephen F. Siegel, Timothy K. Zirkel,
 * University of Delaware, University of Massachusetts
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


/*
 * Name:       gausselim_par.c
 * Date:       21 Jul 2004
 * Revised:    24 Jul 2004
 * Author:     Anastasia V. Mironova <mironova@laser.cs.umass.edu>
 * Author:     Stephen Siegel <siegel@cs.umass.edu>
 * Maintainer: Anastasia V. Mironova <mironova@laser.cs.umass.edu>
 * Reader:
 *
 * Compile:    mpicc gausselim_par.c
 * Run:        mpirun -np m a.out inputfilename
 *  input file must have such format:
 *  rows = <Integer>
 *  cols = <Integer>
 *  <Double> <Double> <Double> ...
 *
 * Description: This is a parallel implementation of the Gauss-Jordan
 * elimination algorithm.  The input is an n x m matrix A of double
 * precision floating point numbers.  At termination, A has been
 * placed in reduced row-echelon form, where the rows are stored on
 * different processes. This implementation is based on the sequential
 * algorithm described in many places; see for example Howard Anton,
 * Elementary Linear Algebra, Wiley, 1977, Section 1.2.In this
 * implementation a modification to this algorithm has been made to
 * perform backward subsitution together with the process of reduction
 * to row-echelon form.
 *
 * The rows of the matrix are distributed among the processes so that
 * the process of rank i has the i-th row. For example, if A is the
 * 2x3 matrix
 * 
 *     A[0,0]   A[0,1]   A[0,2]
 *     A[1,0]   A[1,1]   A[1,2]
 *
 * process 0 has A[0,0], A[0,1], A[0,2], and process 1 has A[1,0],
 * A[1,1], A[1,2].  Naturally, the number of processes must equal the
 * number of rows.
 * 
 * Once the computation has taken place, the matrix is in the reduced
 * row-echelon form and distributed among the processes so that the
 * process with the lowest rank has the row with the left-most pivot
 * entry.
 */

#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* Global variables */
double * matrix;
FILE * outfile;   
int rank, nprocs;

void quit() {
  if(!rank) {
    printf("too few arguments:\n"
	   "Usage: ges [-debug] inputfilename \n");
    printf("Input file must have format:\n\n");
    printf("rows = <INTEGER>\n");
    printf("cols = <INTEGER>\n");
    printf("<DOUBLE> <DOUBLE> ...\n\n");
    printf("where there are (rows * cols) doubles at the end.\n");
    fflush(stdout);
  }
  MPI_Abort(MPI_COMM_WORLD, 0);
}

void readint(FILE *file, char *keyword, int *ptr) {
  char buf[101];
  int value;
  int returnval;

  returnval = fscanf(file, "%100s", buf);
  if (returnval != 1) quit();
  if (strcmp(keyword, buf) != 0) quit();
  returnval = fscanf(file, "%10s", buf);
  if (returnval != 1) quit();
  if (strcmp("=", buf) != 0) quit();
  returnval = fscanf(file, "%d", ptr);
  if (returnval != 1) quit();
}

void readdouble(FILE *file, char *keyword, double *ptr) {
  char buf[101];
  int value;
  int returnval;

  returnval = fscanf(file, "%100s", buf);
  if (returnval != 1) quit();
  if (strcmp(keyword, buf) != 0) quit();
  returnval = fscanf(file, "%10s", buf);
  if (returnval != 1) quit();
  if (strcmp("=", buf) != 0) quit();
  returnval = fscanf(file, "%lf", ptr);
  if (returnval != 1) quit();
}


/* Initialize values of numRow, numCol and matrix for each process. */
void initialize(char * filename, int * numRows, int * numCols) {
  FILE * infile;
  int i, j;

  if(rank == 0) {
    infile = fopen(filename, "r");
    assert(infile);
    readint(infile, "rows", numRows);
    assert((*numRows) > 0);
    if((*numRows) != nprocs){
      printf("Number of processes must equal the number of rows: %d != %d \n",
	     nprocs, *numRows);
      assert((*numRows) == nprocs);
    }
    readint(infile, "cols", numCols);
    assert((*numCols) > 0);
  }
  MPI_Bcast(numRows, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(numCols, 1, MPI_INT, 0, MPI_COMM_WORLD);
  matrix = (double *)malloc((*numCols) * sizeof(double));
  assert(matrix);
  if(rank == 0) {
    double buf[*numCols];

    for(i = 0; i < (*numRows); i++) {
      for(j = 0; j < (*numCols); j++)
	if(fscanf(infile, "%lf", buf + j) != 1) quit();
      if(i != 0)
	MPI_Send(buf, *numCols, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
      else
	memcpy(matrix, buf, (*numCols) * sizeof(double));
    }
    fclose(infile);
  } else 
    MPI_Recv(matrix, *numCols, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

/* Prints the given matrix of doubles to a output file. The parameter
 * numRows gives the number of rows in the matrix, and numCols the
 * number of columns.  This function uses MPI_Send and MPI_Recv to
 * send all the rows to process 0, which receives them in the proper
 * order and prints them out. The string message is printed out by
 * process 0 first.
 */
void writeMatrix(char* message, int numRows, int numCols) {
  int i,j;
  int rank;
  MPI_Status status;
  
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    double* row = (double*)malloc(numCols * sizeof(double));

    assert(outfile);
    fprintf(outfile, "%s",message);
    for (i = 0; i < numRows; i++) {
      if (i == 0) {
	for (j = 0; j < numCols; j++) {
	  row[j] = matrix[j];
	}
      }
      else {
	MPI_Recv(row, numCols, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &status);
      }
      for (j = 0; j < numCols; j++) {
	fprintf(outfile, "%lf ", row[j]);
      }
      fprintf(outfile, "\n");
    }
    fprintf(outfile, "\n");
    free(row);
  }
  else {
    MPI_Send(matrix, numCols, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
  }
}


/* Performs Gaussian elimination on the given matrix of doubles.  The
 * parameter numRows gives the number of rows in the matrix, and
 * numCols the number of columns.  Upon return, the matrix will be in
 * reduced row-echelon form.
 */
int gausselim(int numRows, int numCols, int debug) {
  int top = 0;           // the current top row of the matrix
  int col = 0;           // column index of the current pivot
  int pivotRow = 0;      // row index of current pivot
  double pivot = 0.0;    // the value of the current pivot
  int j = 0;             // loop variable over columns of matrix
  double tmp = 0.0;      // temporary double variable
  MPI_Status status;     // status object needed for receives
  int rank;              // rank of this process
  int nprocs;            // number of processes
  double* toprow = (double*)malloc(numCols * sizeof(double));

  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  for (top=col=0; top<numRows && col< numCols; top++, col++) {

    /* At this point we know that the submatrix consisting of the
     * first top rows of A is in reduced row-echelon form.  We will now
     * consider the submatrix B consisting of the remaining rows.  We
     * know, additionally, that the first col columns of B are
     * all zero.
     */

    if (debug && rank == 0) {
      printf("Top: %d\n", top);
    }

    /* Step 1: Locate the leftmost column of B that does not consist
     * of all zeros, if one exists.  The top nonzero entry of this
     * column is the pivot. */
  
    for (; col < numCols; col++) {
      if (matrix[col] != 0.0 && rank >= top) {
	MPI_Allreduce(&rank, &pivotRow, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
      }
      else {
	MPI_Allreduce(&nprocs, &pivotRow, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
      }
      if (pivotRow < nprocs){
	break;
      }
    }

    if (col >= numCols) {
      break;
    }

    if (debug) {
      if (rank == 0) {
	printf("Step 1 result: col=%d, pivotRow=%d\n\n", col, pivotRow);
      }
    }
      
    /* At this point we are guaranteed that pivot = A[pivotRow,col] is
     * nonzero.  We also know that all the columns of B to the left of
     * col consist entirely of zeros. */

    /* Step 2: Interchange the top row with the pivot row, if
     * necessary, so that the entry at the top of the column found in
     * Step 1 is nonzero. */

    if (pivotRow != top) {
      if (rank == top) {
	MPI_Sendrecv_replace(matrix, numCols, MPI_DOUBLE, pivotRow, 0, 
			     pivotRow, 0, MPI_COMM_WORLD, &status);
      }
      else if (rank == pivotRow) {
	MPI_Sendrecv_replace(matrix, numCols, MPI_DOUBLE, top, 0, 
			     top, 0,  MPI_COMM_WORLD, &status);
      }
    }

    if (rank == top) {
      pivot = matrix[col];
    }
    
    if (debug) {
      writeMatrix("Step 2 result: \n", numRows, numCols);
    }

    /* At this point we are guaranteed that A[top,col] = pivot is
     * nonzero. Also, we know that (i>=top and j<col) implies
     * A[i,j] = 0. */

    /* Step 3: Divide the top row by pivot in order to introduce a
     * leading 1. */

    if (rank == top) {
      for (j = col; j < numCols; j++) {
	matrix[j] /= pivot;
	toprow[j] = matrix[j];
      }
    }

    if (debug) {
      writeMatrix("Step 3 result:\n", numRows, numCols);
    }

    /* At this point we are guaranteed that A[top,col] is 1.0,
     * assuming that floating point arithmetic guarantees that a/a
     * equals 1.0 for any nonzero double a. */

    MPI_Bcast(toprow, numCols, MPI_DOUBLE, top, MPI_COMM_WORLD);

    /* Step 4: Add suitable multiples of the top row to rows below so
     * that all entries below the leading 1 become zero. */

    if (rank != top) {
      tmp = matrix[col];
      for (j = col; j < numCols; j++) {
	matrix[j] -= toprow[j]*tmp;
      }
    }

    if (debug) {
      writeMatrix("Step 4 result: \n", numRows, numCols);
    }
  }
  free(toprow);
}



/*
 * Usage: mpirun -np m a.out debug inputfilename
 *   
 *   debug : debug flag, triggers output to screen at every step if non-zero
 *
 * Computes reduced row-echelon form and prints intermediate steps.
 */
int main(int argc, char** argv) {
  char infile[101];
  int numRows;
  int numCols;
  int debug = 0;

  outfile = fopen("rowdistout", "w");
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (argc >= 2)
    if (!strcmp("-debug", argv[1]))
      debug = 1;
  if(argc < 2 + debug)
    quit();
  sscanf(argv[1+debug], "%s", infile);
  initialize(infile, &numRows, &numCols);
  writeMatrix("Original matrix:\n", numRows, numCols);
  gausselim(numRows, numCols, debug);
  writeMatrix("Reduced row-echelon form:\n", numRows, numCols);
  MPI_Finalize();
  fclose(outfile);
  free(matrix);
  return 0;
}
