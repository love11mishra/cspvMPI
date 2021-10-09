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
 * Name:       gausselim_seq.c
 * Date:       21 Jul 2004
 * Revised:    24 Jul 2004
 * Author:     Anastasia V. Mironova <mironova@laser.cs.umass.edu>
 * Author:     Stephen Siegel <siegel@cs.umass.edu>
 * Maintainer: Anastasia V. Mironova <mironova@laser.cs.umass.edu>
 * Reader:
 *
 * Compile:    cc gausselim_seq.c
 * Run:        a.out inputfilename
 *  intput file must have such format:
 *  rows = <Integer>
 *  cols = <Integer>
 *  <Double> <Double> <Double> ...
 *
 * Description: This is a sequential implementation of the
 * Gauss-Jordan elimination algorithm.  The input is an n x m matrix A
 * of double precision floating point numbers.  At termination, A has
 * been placed in reduced row-echelon form. The original algorithm is 
 * described in many places; see for example Howard Anton, Elementary 
 * Linear Algebra, Wiley, 1977, Section 1.2. In this implementation a
 * modification to this algorithm has been made to perform backward 
 * subsitution together with the process of reduction to row-echelon 
 * form.
 * 
 * The entries of the matrix are stored in row-major order, i.e., if
 * A is the 2x3 matrix
 * 
 *     A[0,0]   A[0,1]   A[0,2]
 *     A[1,0]   A[1,1]   A[1,2]
 *
 * then its entries are stored as A[0,0], A[0,1], A[0,2], A[1,0],
 * A[1,1], A[1,2].
 *
 * This has the error of not updating col in the main for loop of gausselim()
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* Global variable */
double * matrix; 
FILE * outfile;

void quit() {
  printf("Too few arguments.\n");
  printf("Usage: ges [-debug] inputfilename \n");
  printf("Input file must have format:\n\n");
  printf("rows = <INTEGER>\n");
  printf("cols = <INTEGER>\n");
  printf("<DOUBLE> <DOUBLE> ...\n\n");
  printf("where there are (rows * cols) doubles at the end.\n");
  fflush(stdout);
  exit(1);
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

void initialize(char * filename, int * numRows, int * numCols) {
  char keyword[101];
  FILE * infile = fopen(filename, "r");
  int i;

  assert(infile);
  readint(infile, "rows", numRows);
  assert((*numRows) > 0);
  readint(infile, "cols", numCols);
  assert((*numCols) > 0);
  matrix = (double *)malloc((*numRows) * (*numCols) * sizeof(double));
  assert(matrix);
  for(i = 0; i < (*numRows) * (*numCols); i++)
    if(fscanf(infile, "%lf", matrix + i) != 1) quit();
  fclose(infile);
}

/* Prints the given matrix of doubles to stdout. The parameter numRows
 * gives the number of rows in the matrix, and numCols the number of
 * columns. The string message is printed before the matrix.
 */
void writeMatrix(char* message, int numRows, int numCols) {
  int k;

  assert(outfile);
  fprintf(outfile, "%s",message);
  for (k = 0; k < numRows*numCols; k++) {
    fprintf(outfile, "%lf ", matrix[k]);
    if ((k+1)%numCols == 0) {
      fprintf(outfile, "\n");
    }
  }
  fprintf(outfile, "\n");
}


/* Performs Gaussian elimination on the given matrix of doubles.  The
 * parameter numRows gives the number of rows in the matrix, and
 * numCols the number of columns.  Upon return, the matrix will be in
 * reduced row-echelon form.
 */
void gausselim(int numRows, int numCols, int debug) {
  int top = 0;           // index of current top row
  int col = 0;           // index of current left column
  int pivotRow = 0;      // index of row containing the pivot
  double pivot = 0.0;    // the value of the pivot
  int i = 0;             // loop variable over rows of matrix
  int j = 0;             // loop variable over columns of matrix
  double tmp = 0.0;      // temporary double variable

  for (top=col=0; top<numRows && col< numCols; top++) {

    /* At this point we know that the submatarix consisting of the
     * first top rows of A is in reduced row-echelon form.  We will
     * now consider the submatrix B consisting of the remaining rows.
     * We know, additionally, that the first col columns of B are all
     * zero.
     */

    if (debug) printf("Top: %d\n\n", top);
	
    /* Step 1: Locate the leftmost column of B that does not consist
     * entirely of zeros, if one exists.  The top nonzero entry of
     * this column is the pivot. */

    pivot = 0.0;
    for (; col < numCols; col++) {
      for (pivotRow = top; pivotRow < numRows; pivotRow++) {
	pivot = matrix[pivotRow*numCols + col];
	if (pivot) break;
      }
      if (pivot) break;
    }

    if (col >= numCols) {
      break;
    }

    /* At this point we are guaranteed that pivot = A[pivotRow,col] is
     * nonzero.  We also know that all the columns of B to the left of
     * col consist entirely of zeros. */

    if (debug) {
      printf("Step 1 result: col=%d, pivotRow=%d, pivot=%lf.\n\n",
	     col, pivotRow, pivot);
    }
	    
    /* Step 2: Interchange the top row with the pivot row, if
     * necessary, so that the entry at the top of the column found in
     * Step 1 is nonzero. */

    if (pivotRow != top) {
      for (j = 0; j < numCols; j++) {
	tmp = matrix[top*numCols + j];
	matrix[top*numCols + j] = matrix[pivotRow*numCols + j];
	matrix[pivotRow*numCols + j] = tmp;
      }
    }

    if (debug) {
      writeMatrix("Step 2 result:\n", numRows, numCols);
    }

    /* At this point we are guaranteed that A[top,col] = pivot is
     * nonzero. Also, we know that (i>=top and j<col) implies
     * A[i,j] = 0. */

    /* Step 3: Divide the top row by pivot in order to introduce a
     * leading 1. */

    for (j = col; j < numCols; j++) {
      matrix[top*numCols + j] /= pivot;
    }

    if (debug) {
      writeMatrix("Step 3 result:\n", numRows, numCols);
    }

    /* At this point we are guaranteed that A[top,col] is 1.0,
     * assuming that floating point arithmetic guarantees that a/a
     * equals 1.0 for any nonzero double a. */

    /* Step 4: Add suitable multiples of the top row to all other rows
     * so that all entries above and below the leading 1 become
     * zero. */
    
    for (i = 0; i < numRows; i++) {
      if (i != top){
	tmp = matrix[i*numCols + col];
	for (j = col; j < numCols; j++) {
	  matrix[i*numCols + j] -= matrix[top*numCols + j]*tmp;
	}
      }
    }

    if (debug) {
      writeMatrix("Step 4 result:\n", numRows, numCols);
    }
  }
}


/*
 * Usage: ges [-debug] n m A[0,0] A[0,1] ... A[n-1,m-1]
 *
 *   n : number of rows in matrix
 *   m : number of columns in matrix
 *   A[0,0] .. A[n-1,m-1] : entries of matrix (doubles)
 *
 * Computes reduced row-echelon form and prints intermediate steps.
 */
int main(int argc, char ** argv) {
  char filename[101];
  int numRows;
  int numCols;
  int k;
  int debug = 0;

  outfile = fopen("badout", "w");
  if (argc >= 2) 
    if (!strcmp("-debug", argv[1])) 
      debug = 1;
  if(argc < 2 + debug)
    quit();
  sscanf(argv[1+debug], "%s", filename);
  initialize(filename, &numRows, &numCols);
  writeMatrix("Original matrix:\n", numRows, numCols);
  gausselim(numRows, numCols, debug);
  writeMatrix("Reduced row-echelon form:\n", numRows, numCols);
  fclose(outfile);
  free(matrix);
  return 0;
}
