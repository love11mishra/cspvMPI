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
			int buffer_recv;
			int *val, count;
			MPI_Comm comm = MPI_COMM_WORLD;
            MPI_Send(&buffer_sent, 1, MPI_INT, 1, 11, MPI_COMM_WORLD);
            printf("MPI process %d sends value %d.\n", my_rank, buffer_sent);
           	MPI_Recv(&buffer_recv, 1, MPI_INT, 1, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
            printf("MPI process %d received value %d.\n", my_rank, buffer_recv);
            break;
        }
        case RECEIVER:
        {
            int buffer_recv;
			int buffer_sent = 54321;
			MPI_Send(&buffer_sent, 1, MPI_INT, 0, 99, MPI_COMM_WORLD);
            printf("MPI process %d sends value %d.\n", my_rank, buffer_sent);
            MPI_Recv(&buffer_recv, 1, MPI_INT, 0, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("MPI process %d received value: %d.\n", my_rank, buffer_recv);
            break;
        }
    }
 
    MPI_Finalize();
 
    return EXIT_SUCCESS;
}

