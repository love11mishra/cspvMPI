
#include <stdio.h>
#include <stdlib.h>
//#include<crest.h>
#include <mpi.h>
//#include <string.h>

//#define buf_size 128

int main (int argc, char **argv){

  int nprocs = -1;
  int rank = -1;
  char processor_name[128];
  int namelen = 128;
  int buf_size = 10;
  int buf0[10];
  int buf1[10];
  MPI_Status status;
  MPI_Request req;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  //  MPI_Get_processor_name (processor_name, &namelen);
  // printf ("(%d) is alive on %s\n", rank, processor_name);
  //fflush (stdout);

  //  MPI_Barrier (MPI_COMM_WORLD);

  if (nprocs < 5)
    {
      printf ("not enough tasks\n");
    }
  else if (rank == 0)
    {
      MPI_Recv (&buf0, buf_size, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
      MPI_Ssend (&buf0, buf_size, MPI_INT, 3, 0, MPI_COMM_WORLD);
      MPI_Recv (&buf0, buf_size, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    }
  else if (rank == 1)
    {
      MPI_Ssend (&buf0, buf_size, MPI_INT, 0, 0, MPI_COMM_WORLD);
      MPI_Ssend (&buf0, buf_size, MPI_INT, 3, 0, MPI_COMM_WORLD);
    }
  else if (rank == 2)
    {
      MPI_Recv (&buf0, buf_size, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
      MPI_Ssend (&buf0, buf_size, MPI_INT,0, 0, MPI_COMM_WORLD); 
    }
  else if (rank == 3)
    {
      MPI_Recv (&buf0, buf_size, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
      MPI_Recv (&buf0, buf_size, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    }
  else if (rank == 4)
    {
      MPI_Ssend (&buf0, buf_size, MPI_INT, 2, 0, MPI_COMM_WORLD);
    }

  
  //  MPI_Barrier (MPI_COMM_WORLD);

  MPI_Finalize ();
  //  printf ("(%d) Finished normally\n", rank);
  return 0;
}


