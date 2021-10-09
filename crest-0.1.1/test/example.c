#include<mpi.h>
#include<stdio.h>

void main(int argc, char **argv){
	MPI_Init(&argc, &argv);
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	int buf;
	if(rank == 0){
		MPI_Request req2, req1;
		MPI_Status st1, st2;
		MPI_Isend(&rank, 1, MPI_INT, 1, 99, MPI_COMM_WORLD, &req1);
		MPI_Irecv(&buf, 1, MPI_INT, 1, 98, MPI_COMM_WORLD, &req2);
		MPI_Wait(&req1,&st1);
		MPI_Wait(&req2,&st2);
		printf("recvd rank %d from %d\n", rank, buf);
	}	
	if(rank == 1){
		MPI_Request req2, req1;
		MPI_Status st1, st2;
		MPI_Isend(&rank, 1, MPI_INT, 0, 98, MPI_COMM_WORLD, &req1);
		MPI_Irecv(&buf, 1, MPI_INT, 0, 99, MPI_COMM_WORLD, &req2);
		MPI_Wait(&req1,&st1);
		MPI_Wait(&req2,&st2);
		printf("recvd rank %d from %d\n", rank, buf);
	}
	MPI_Finalize();	
}
