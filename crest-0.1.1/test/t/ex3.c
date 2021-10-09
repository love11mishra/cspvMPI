//Potential deadlock case
//cyclic send recv dependency
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include<crest.h>
 
int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);
 
    // Get the number of processes and check only 2 processes are used
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if(size < 2)
    {
        printf("This application is meant to be run with 2 or more processes.\n");
		fflush(stdin);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
 
    // Get my rank and do the corresponding job
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    
	int buffer_sent = 12345;
	int buffer_recv;
	int sendto = (my_rank + 1)%size;
	int recvfrom = (my_rank - 1 + size)%size;
    
	MPI_Send(&buffer_sent, 1, MPI_INT, sendto, 11, MPI_COMM_WORLD);
    printf("MPI process %d sends value %d.\n", my_rank, buffer_sent);
    
	MPI_Recv(&buffer_recv, 1, MPI_INT, recvfrom, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("MPI process %d received value %d.\n", my_rank, buffer_recv);
    
	MPI_Finalize();
 
    return EXIT_SUCCESS;
}

