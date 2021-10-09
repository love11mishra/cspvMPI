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
        /* everyone calls bcast, data is taken from root and ends up in everyone's buf */
        if(rank != size -1) MPI_Bcast(&buf, 1, MPI_INT, root, MPI_COMM_WORLD);
		if(rank == size -1) MPI_Barrier(MPI_COMM_WORLD);//MPI_Bcast(&buf, 1, MPI_INT, rank, MPI_COMM_WORLD);
        MPI_Finalize();
        return 0;
}
