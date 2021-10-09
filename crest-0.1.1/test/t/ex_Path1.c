#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<crest.h>
//#include<mpi.h>
int main(int argc, char *argv[]){
	MPI_Init(&argc, &argv);
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	if(size != 3){
        printf("This application is meant to be run with 3 processes.\n");
		fflush(stdin);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	}
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	enum role_rank {P0, P1, P2};
	
	switch(rank){
		case P0:
			{
				int val1 = 5;
				int val2[] = {15, 19, 3};
				MPI_Send(&val1, 1, MPI_INT, 1, 99, MPI_COMM_WORLD);
				sleep(1);
				MPI_Send(&val2, 3, MPI_INT, 2, 99, MPI_COMM_WORLD);
				break;
			}
		case P1:
			{
				int x = 1;
				MPI_Status status;
			//	if(x == 1) x = 2;
				MPI_Recv(&x, 1, MPI_INT, MPI_ANY_SOURCE, 99, MPI_COMM_WORLD, &status);
				printf("P1 received x = %d\n", x);
				if(x == 5){
					int val[] = {20, 78, 79};
					MPI_Send(&val, 3, MPI_INT, 2, 99, MPI_COMM_WORLD);
				}
				break;
			}
		case P2:
			{
				int y[3];
				MPI_Status status1, status2;
				MPI_Recv(&y, 3, MPI_INT, MPI_ANY_SOURCE, 99, MPI_COMM_WORLD, &status1);
				printf("P2 first recv y = %d\n", y);
				if(y[0] == 20 /*&& y[1] == 78 && y[2] == 79*/){
				MPI_Recv(&y, 3, MPI_INT, MPI_ANY_SOURCE, 99, MPI_COMM_WORLD, &status2);
				printf("P2 second recv y = %d\n", y);
				assert(y[2]  == 3);
				printf("assertion reached\n");
				}
				break;
			}
	}
	MPI_Finalize();
	return 0;
}
