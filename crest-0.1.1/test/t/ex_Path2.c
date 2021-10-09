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
				int val1 = 5, val2 = 15;
				//int val1, val2;
				//CREST_int(val1);
				//CREST_int(val2);
				MPI_Send(&val1, 1, MPI_INT, 1, 99, MPI_COMM_WORLD);
				sleep(1);
				MPI_Send(&val2, 1, MPI_INT, 2, 99, MPI_COMM_WORLD);
				break;
			}
		case P1:
			{
				int x;
				//CREST_int(x);
				MPI_Status status;
				MPI_Recv(&x, 1, MPI_INT, MPI_ANY_SOURCE, 99, MPI_COMM_WORLD, &status);
				printf("P1 received x = %d\n", x);
				if(x == 5){
					int val = 20;
					MPI_Send(&val, 1, MPI_INT, 2, 99, MPI_COMM_WORLD);
				}
				break;
			}
		case P2:
			{
				int y;
				//CREST_int(y);
//				klee_make_symbolic(&y, sizeof(y), "y"); //for mpi-sv
				MPI_Status status1, status2;
				MPI_Recv(&y, 1, MPI_INT, MPI_ANY_SOURCE, 99, MPI_COMM_WORLD, &status1);
				printf("P2 first recv y = %d\n", y);
				if(y == 20){
				MPI_Recv(&y, 1, MPI_INT, MPI_ANY_SOURCE, 99, MPI_COMM_WORLD, &status2);
				printf("P2 second recv y = %d\n", y);
				assert(y == 16);
				printf("assertion reached\n");
				}
				break;
			}
	}
	MPI_Finalize();
	return 0;
}
