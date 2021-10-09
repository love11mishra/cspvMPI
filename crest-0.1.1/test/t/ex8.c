//No deadlock
//Send1-Send2 Irecv2-Irecv1-Wait2-Wait1
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
            MPI_Send(&buffer_sent, 1, MPI_INT, 1, 11, MPI_COMM_WORLD);
            printf("MPI process %d sends value %d.\n", my_rank, buffer_sent);
            MPI_Send(&buffer_sent1, 1, MPI_INT, 1, 99, MPI_COMM_WORLD);
            printf("MPI process %d sends value %d.\n", my_rank, buffer_sent1);
            break;
        }
        case RECEIVER:
        {
            int buffer_recv;
			int buffer_recv1;
			MPI_Request request1, request2;
            MPI_Irecv(&buffer_recv, 1, MPI_INT, 0, 99, MPI_COMM_WORLD, &request1);
            MPI_Irecv(&buffer_recv1, 1, MPI_INT, 0, 11, MPI_COMM_WORLD, &request2);
			MPI_Wait(&request1, MPI_STATUS_IGNORE);
			MPI_Wait(&request2, MPI_STATUS_IGNORE);
            printf("MPI process %d received value: %d.\n", my_rank, buffer_recv);
            printf("MPI process %d received value: %d.\n", my_rank, buffer_recv1);
            break;
        }
    }
 
    MPI_Finalize();
 
    return EXIT_SUCCESS;
}

