#include <stdio.h>
#include<unistd.h>
#include<crest.h>
#define comm MPI_COMM_WORLD
int main(int argc, char** argv) {
        int rank;
        int buf;
        const int root=0;
		int size;
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		MPI_Comm_size(MPI_COMM_WORLD, &size);
		buf = rank;
		if(rank == 0){
			char x, y, z;
			MPI_Status status[3];
			MPI_Recv(&x, 1, MPI_CHAR, 1, 11, comm, &status[0]);
			MPI_Recv(&y, 1, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &status[1]);
			MPI_Recv(&z, 1, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG,comm, &status[2]);
			if( z == 'd') MPI_Bcast(&buf, 1, MPI_INT, root, comm);
		}
		if(rank == 1){
			char a = 'a', b = 'b', c = 'c';
			MPI_Request req[2];
			MPI_Status statuses[2];
			MPI_Isend(&a, 1, MPI_CHAR, 0, 11, comm, &req[0]);
			MPI_Isend(&b, 1, MPI_CHAR, 2, 99, comm, &req[1]);
			MPI_Waitall(2, req, statuses);
			MPI_Send(&c, 1, MPI_CHAR, 0, 44, comm);
        	MPI_Bcast(&buf, 1, MPI_INT, root, comm);

		}
		if(rank == 2){
			char d = 'd';
			char w;
			MPI_Status status;
			MPI_Recv(&w, 1, MPI_CHAR, 1, 99, comm, &status);
			sleep(1);
			MPI_Send(&d, 1, MPI_CHAR, 0, 44, comm);
        	MPI_Bcast(&buf, 1, MPI_INT, root, comm);
		}
        MPI_Finalize();
        return 0;
}
