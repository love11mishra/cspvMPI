#include<mpi.h>
#include<stdio.h>
#include<crest.h>

void main(int argc, char **argv){
		MPI_Init(&argc, &argv);
		int rank, size;
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		MPI_Comm_size(MPI_COMM_WORLD, &size);

		int buf, input, input2;

		CREST_int(input);
		CREST_int(input2);
	//	printf("Before if\n");
		if(input != 2){
				printf("branch0\n");
	//			printf("After if input\n");
				if( rank == 0){
	//					printf("After if rank 0\n");
						MPI_Request req2, req1;
						MPI_Status st1, st2;
						MPI_Isend(&rank, 1, MPI_INT, 1, 99, MPI_COMM_WORLD, &req1);
						MPI_Irecv(&buf, 1, MPI_INT, 1, 98, MPI_COMM_WORLD, &req2);
						MPI_Wait(&req1,&st1);
						MPI_Wait(&req2,&st2);
						printf("recvd rank %d from %d\n", rank, buf);
				}	
				else{
	//					printf("After if rank 1\n");
						MPI_Request req2, req1;
						MPI_Status st1, st2;
						MPI_Isend(&rank, 1, MPI_INT, 0, 98, MPI_COMM_WORLD, &req1);
						MPI_Irecv(&buf, 1, MPI_INT, 0, 99, MPI_COMM_WORLD, &req2);
						MPI_Wait(&req1,&st1);
						MPI_Wait(&req2,&st2);
						printf("recvd rank %d from %d\n", rank, buf);
				}
		}
		if(input != 2){
				int buffer_sent = 12345;
				int buffer_recv;
				int sendto = (rank + 1)%size;
				int recvfrom = (rank - 1 + size)%size;
    			
				if(input2 > 10){
						printf("branch1\n");
						//No deadlock
						if(rank == 0){
								MPI_Request request1;
								MPI_Isend(&buffer_sent, 1, MPI_INT, sendto, 11, MPI_COMM_WORLD, &request1);
								MPI_Recv(&buffer_recv, 1, MPI_INT, recvfrom, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
								MPI_Wait(&request1, MPI_STATUS_IGNORE);
						}
						else{
								MPI_Send(&buffer_sent, 1, MPI_INT, sendto, 11, MPI_COMM_WORLD);
    							printf("MPI process %d sends value %d.\n", rank, buffer_sent);
    							
								MPI_Recv(&buffer_recv, 1, MPI_INT, recvfrom, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    							printf("MPI process %d received value %d.\n", rank, buffer_recv);
						}
				}
				else{//No deadlock
						printf("branch2\n");
						if(rank == 0){
								MPI_Send(&buffer_sent, 1, MPI_INT, sendto, 11, MPI_COMM_WORLD);
    							printf("MPI process %d sends value %d.\n", rank, buffer_sent);
								MPI_Recv(&buffer_recv, 1, MPI_INT, recvfrom, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    							printf("MPI process %d received value %d.\n", rank, buffer_recv);
						}
						else{
								MPI_Recv(&buffer_recv, 1, MPI_INT, recvfrom, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    							printf("MPI process %d received value %d.\n", rank, buffer_recv);
								MPI_Send(&buffer_sent, 1, MPI_INT, sendto, 11, MPI_COMM_WORLD);
    							printf("MPI process %d sends value %d.\n", rank, buffer_sent);
    					
						}
				}
		}
		//__CrestLogPC(3);
		MPI_Finalize();	
}
