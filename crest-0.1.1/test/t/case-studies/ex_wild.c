//Potential deadlock case
//Send-Rev Send-Recv
#include <stdio.h>
#include <stdlib.h>
//#include <mpi.h>
#include<crest.h>
 
/**
 * @brief Illustrates how to send a message in a non-blocking fashion.
 * @details This program is meant to be run with 2 processes: a sender and a
 * receiver.
 **/
int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);
 
    // Get the number of processes and check only 2 processes are used
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
 
    // Get my rank and do the corresponding job
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	int buf = 1;
	if(my_rank == 0){
           	MPI_Recv(&buf, 1, MPI_INT, MPI_ANY_SOURCE, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
           	MPI_Recv(&buf, 1, MPI_INT, 2, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
	}
	else {MPI_Send(&buf, 1, MPI_INT, 0, 99, MPI_COMM_WORLD);}
    MPI_Finalize();
 
    return 0;
}

