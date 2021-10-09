//No deadlock
//Isend1-Ssend2-Wait1 Recv1-Recv2
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
    if(size != 2)
    {
        printf("This application is meant to be run with 2 processes.\n");
		fflush(stdin);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
 
    // Get my rank and do the corresponding job
    enum role_ranks { SENDER, RECEIVER };
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    switch(my_rank)
    {
        case SENDER:
        {
            int buffer_sent = 12345;
			int buffer_sent1 = 12111;
			MPI_Request request;
            MPI_Isend(&buffer_sent, 1, MPI_INT, 1, 11, MPI_COMM_WORLD, &request);
            printf("MPI process %d sends value %d.\n", my_rank, buffer_sent);
            MPI_Ssend(&buffer_sent1, 1, MPI_INT, 1, 99, MPI_COMM_WORLD);
            printf("MPI process %d sends value %d.\n", my_rank, buffer_sent1);
			MPI_Wait(&request, MPI_STATUS_IGNORE);
            break;
        }
        case RECEIVER:
        {
            int buffer_recv;
			int buffer_recv1;
            MPI_Recv(&buffer_recv, 1, MPI_INT, 0, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("MPI process %d received value: %d.\n", my_rank, buffer_recv);
            MPI_Recv(&buffer_recv1, 1, MPI_INT, 0, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("MPI process %d received value: %d.\n", my_rank, buffer_recv1);
            break;
        }
    }
 
    MPI_Finalize();
 
    return EXIT_SUCCESS;
}

