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
/* FILE: gaussJordan_elimination.c A gaussian-jordan elimination
 * solver that converts a given matrix to a reduce row echelon form
 * matrix
 * RUN : mpicc gaussJordan_elimination.c; mpiexec -n 4 ./a.out numRow numCol m[0][0], m[0][1] ...
 * VERIFY : civl verify gaussianJordan_elimination.c
 */
#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global parameters */
int numRow;     // number of rows in the matrix
int numCol;     // number of columns in the matrix, the right-most
		// column is vector B
int localRow;   // number of rows owned by the process
int rank, nprocs;
int first;      // the global index of the first row in original 
                // matrix
/* a Global Row Index -> Current Row Location table maps original
   indices of rows to their current location in current matrix */
int *loc;       
/* a Current Row Location -> Global Row Index table maps current
   locations of current matrix to their original row indices */
int *idx;
/* local matrix of every process */
long double *a;
/* output filename */
FILE * outfile;

void quit() {
  if(!rank) {
    printf("too few arguments:\n"
	   "Usage: ges inputfilename \n");
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

/* Book keeping functions */
/* Return the owner of the row given by the global index of it in
   original matrix */
#define OWNER(index) ((nprocs*(index+1)-1)/numRow)

/* Returns the global index of the first row owned
 * by the process with given rank */
int firstForProc(int rank) {
  return (rank*numRow)/nprocs;      
}

/* Returns the number of rows the given process owns */
int countForProc(int rank) {
  int a = firstForProc(rank);
  int b = firstForProc(rank + 1);

  return b - a;
}

/* Initialize values of numRow, numCol and matrix for each process. */
void initialize(char * infilename) {
  FILE * infile;
  int i, j;

  if(rank == 0) {
    infile = fopen(infilename, "r");
    assert(infile);
    readint(infile, "rows", &numRow);
    assert(numRow > 0);
    readint(infile, "cols", &numCol);
    assert(numCol > 0);
  }
  MPI_Bcast(&numRow, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&numCol, 1, MPI_INT, 0, MPI_COMM_WORLD);
  first = firstForProc(rank);
  localRow = countForProc(rank);
  a = (long double*)malloc(numCol*localRow*sizeof(long double));
  loc = (int*)malloc(numRow*sizeof(int));
  idx = (int*)malloc(numRow*sizeof(int));
  if(rank == 0) {
    for(i = 0; i < nprocs; i++) {
      long double buf[numCol];
      int rows = countForProc(i);
      
      for(j = 0; j < rows; j++) {
	for(int l = 0; l < numCol; l++)
	  if(fscanf(infile, "%Lf", buf + l ) != 1) quit();
	if(i != 0)
	  MPI_Send(buf, numCol, MPI_LONG_DOUBLE, i, 0, MPI_COMM_WORLD);
	else
	  memcpy(&a[numCol * j], buf, numCol * sizeof(long double));
      }   
    }
    fclose(infile);
  } else {
    for(i = 0; i < localRow; i++)
      MPI_Recv(&a[i*numCol], numCol, MPI_LONG_DOUBLE, 0, 0, MPI_COMM_WORLD, 
	       MPI_STATUS_IGNORE);
  }
  for(i = 0; i<numRow; i++){
    loc[i] = i;
    idx[i] = i;
  }
}

/* Locally print a row */
void printRow(long double * row) {
  for(int k=0; k < numCol; k++)
    fprintf(outfile, "%2.6Lf ", row[k]);
  fprintf(outfile, "\n");
}

/* Print the given matrix. Since each process needs to send their data
 * to root process 0, this function is collective.
 * Parameters: 
 * a: the (part of) matrix will be printed.  
 */
void printSystem(char * message) {
  long double recvbuf[numCol];
  
  // Every process follows the order of locations of rows to send their
  // rows to process with rank 0
  for(int i=0; i<numRow; i++) {
    if(OWNER(idx[i]) == rank && rank != 0) {
      MPI_Send(&a[(idx[i]-first)*numCol], numCol, MPI_LONG_DOUBLE, 0, i, MPI_COMM_WORLD);
    }
  }
  
  if(rank == 0) {
    fprintf(outfile, "%s\n", message);
    for(int i=0; i<numRow; i++) {
      if(OWNER(idx[i]) != 0) {
	MPI_Recv(recvbuf, numCol, MPI_LONG_DOUBLE, MPI_ANY_SOURCE, i, 
		 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	printRow(recvbuf);
      }
      else 
	printRow(&a[(idx[i]-first)*numCol]);
    }
    fprintf(outfile, "\n");
  }
}

/* Set row to location loca */
void setLoc(int row, int loca){
  int tmpLoc, tmpIdx;

  tmpLoc = loc[row];
  tmpIdx = idx[loca];
  //swap locations(update index -> location table)
  loc[row] = loca;
  loc[tmpIdx] = tmpLoc;
  //update location -> index table
  idx[loca] = row;
  idx[tmpLoc] = tmpIdx;
}

/* Performs a gaussian elimination on the given matrix, the output
 * matrix will finally be in row echelon form .
 */
void gaussianElimination(long double *a) {
  /* Buffer for the current toppest unprocessed row. */
  long double top[numCol];

  /* For each row of the matrix, it will be processed once. */
  for(int i=0; i < numRow; i++) {
    /* owner of the current unprocessed top row */
    int owner = OWNER(idx[i]); 
    /* the column of the next leading 1, initial value is numCol
     * (which is the max value) because later it will select a minimum
     * one from all unprocessed rows.
     */
    int leadCol = numCol; 
    /* the global index of the row the next leading 1 will be in */
    int rowOfLeadCol = -1;
    int rowOfLeadColOwner;    // the owner of rowOfLeadCol
    /* message buffer: [0]:leadCol ;[1]:rowOfLeadCol */
    int sendbuf[2];
    /* receive buffer: it will contain lead 1 column candidates from
       all processes */
    int recvbuf[2*nprocs];   
    int tmp;

    //step 1: find out the local leftmost nonzero column
    for(int j=i; j < numCol; j++) {
      int k;

      for(k = first; k < first + localRow; k++) {
        // only look at unprocessed rows
        if(loc[k] >= i) {
          if(a[(k-first)*numCol+j] != 0.0) {
            leadCol = j;
            rowOfLeadCol = k; 
            break;
          }
        }
      }
      if(leadCol < numCol)
        break;
    }
    sendbuf[0] = leadCol;
    sendbuf[1] = rowOfLeadCol;
    /* All reduce the smallest column(left-most) of leading 1 to every
       process */
    MPI_Allreduce(sendbuf, recvbuf, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);
    leadCol = recvbuf[0];
    rowOfLeadCol = recvbuf[1];
    /* Now the row containing next leading 1 is decided, findout the
       owner of it. */
    rowOfLeadColOwner = OWNER(rowOfLeadCol);
    /* if leadCol is still initial value, it means there is no
     * avaliable column suitable for next leading 1. Remaining
     * unprocessed rows will be all zeroes. */
    if(leadCol == numCol)
     return;
    // step 2: reducing the leading number to 1
    if(rank == rowOfLeadColOwner) {
      long double denom = a[(rowOfLeadCol - first)*numCol + leadCol];

      if(denom != 0.0)
        for(int j=leadCol; j < numCol; j++)
          a[(rowOfLeadCol - first)*numCol + j] = a[(rowOfLeadCol - first)*numCol + j] / denom;
      memcpy(top, &a[(rowOfLeadCol - first)*numCol
                     ], numCol*sizeof(long double));
    }
    MPI_Bcast(top, numCol, MPI_LONG_DOUBLE, rowOfLeadColOwner, MPI_COMM_WORLD);
    /* swap the row containing next leading 1 to the top location of
       current submatrix */
    if(loc[rowOfLeadCol] != i)
      setLoc(rowOfLeadCol, i);
    /* step 3: add a suitable value to all unprocessed rows to make
       all numbers at the same column as leading 1 zeros. */
    for(int j=0; j < localRow; j++) {
      if(loc[j+first] > i){
        long double factor = -a[j*numCol + leadCol];

        for(int k=leadCol; k < numCol; k++) {
          a[j*numCol + k] += factor * top[k];
        }
      }
    }
  }
}

/* Perform a backward reduction on the given matrix which transforms a
   row echelon form to a reduced row echelon form */
void backwardReduce(long double *a) {
  int leadCol;
  int owner;
  int i;
  long double top[numCol];

  i = numRow - 1;
  for(; i>=1; i--) {
    leadCol = -1;
    owner = OWNER(idx[i]);
    if(rank == owner)
      memcpy(top, &a[(idx[i] - first)*numCol + i], (numCol-i)*sizeof(long double));
    MPI_Bcast(top, (numCol-i), MPI_LONG_DOUBLE, owner, MPI_COMM_WORLD);
    //find out the leading 1 column
    for(int j=0; j<(numCol-i); j++){
      if(top[j] != 0.0){
	leadCol = j+i;
	break;
      }
    }
    if(leadCol == -1)
      continue;
    else {
      for(int j=first; j<first+localRow; j++){
	if(loc[j] < i){
	  long double factor = -a[(j-first)*numCol + leadCol];

	  for(int k=leadCol; k<numCol; k++)
	    a[(j-first)*numCol + k] += factor*top[k-i];
	}
      }
    }
  }
}

int main(int argc, char *argv[]) {
  char infilename[101];

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  if(argc < 2) 
    quit();
  sscanf(argv[1], "%s", infilename);
  initialize(infilename);
  outfile = fopen("striperowsout", "w");
  printSystem("Original matrix:");
  gaussianElimination(a);
  backwardReduce(a);
  printSystem("Reduced row-echelon form:");
  MPI_Finalize();
  free(loc);
  free(idx);
  free(a);
  fclose(outfile);
  return 0;
}

