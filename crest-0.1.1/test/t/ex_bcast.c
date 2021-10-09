#include <stdio.h>
#include<crest.h>

int main(int argc, char** argv) {
        int rank;
        int buf;
        const int root=0;
		int size;
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		MPI_Comm_size(MPI_COMM_WORLD, &size);
	    int buffer_sent = 12345;
		int buffer_recv;
		int sendto = (rank + 1)%size;
		int recvfrom = (rank - 1 + size)%size;
    
		MPI_Send(&buffer_sent, 1, MPI_INT, sendto, 11, MPI_COMM_WORLD);
    	printf("MPI process %d sends value %d.\n", rank, buffer_sent);
    
		MPI_Recv(&buffer_recv, 1, MPI_INT, recvfrom, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    	printf("MPI process %d received value %d.\n", rank, buffer_recv);

        if(rank == root) {
           buf = 777;
        }

        printf("[%d]: Before Bcast, buf is %d\n", rank, buf);

        /* everyone calls bcast, data is taken from root and ends up in everyone's buf */
        MPI_Bcast(&buf, 1, MPI_INT, root, MPI_COMM_WORLD);

        printf("[%d]: After Bcast, buf is %d\n", rank, buf);

        MPI_Finalize();
        return 0;
}
