#include "libcrest/crest.h"
#include<cstring>
#include<string>
#include<cstdlib>
#include<fstream>
#include<assert.h>
#include<unistd.h>
//Adding wrappers for MPI routines @lavleshm
//using namespace crest;
FILE *cr_fp = NULL;
//std::ofstream outfile;
//void CR_DumpMpiTrace(const char * str, int rank, int action = 0){
//	if(action == 1 and cr_fp == NULL){
//	  std::string fileName = "MpiTrace" + std::to_string(rank) + ".txt";
//	  cr_fp = fopen(fileName.c_str(), "w");
//	  //outfile.open(fileName.c_str(), std::ios_base::app);
//	  //assert(outfile);
//	}
//	assert(cr_fp != NULL);
//	fprintf(cr_fp,"%s\n", str);
//	fflush(cr_fp);
//	 //outfile << str << std::endl;
//	 //outfile.flush();
//	if(action == 2 and cr_fp != NULL){
//	 fclose(cr_fp);
//	  //outfile.close();
//	}
//	//fclose(fp);
//	//}
//	//std::ofstream outfile;
//	//outfile.open(fileName.c_str(), std::ios_base::app);
//	//outfile << str << std::endl;
//	//outfile.flush();
//	//outfile.close();
//}
void CR_DumpMpiTrace(const char * str, int rank, int action = 0){
	  std::string fileName = "MpiTrace" + std::to_string(rank) + ".txt";
	  cr_fp = fopen(fileName.c_str(), "a");
	  fprintf(cr_fp,"%s\n", str);
	  fflush(cr_fp);
	  fclose(cr_fp);
	  fsync(fileno(cr_fp));
}

std::string intArrToStr(const int * arr, int count){
	std::string str = "";
	
	str += "[";
	if(count > 0)
		str += std::to_string(arr[0]);
	for(int i = 1; i < count; i++){
		str += "_" + std::to_string(arr[i]);
	}
	str += "]";
	return str;
}

std::string charArrToStr(const char * arr, int count){
	std::string str = "";
	str += "[";
	if(count > 0)
		str += std::to_string(arr[0]);
	for(int i = 1; i < count; i++){
		str += "_" + std::to_string(arr[i]);
	}
	str += "]";
	return str;
}

int num_elem_to_log = 0;

int CR_MPI_Init(int *argc, char ***argv, int line){
	return PMPI_Init(argc, argv);
}

int CR_MPI_Comm_size(MPI_Comm comm, int * size, int crestK, int line){
  int rank;
  num_elem_to_log = crestK; //First num_elem_to_log elements are logged from send buffers.
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int val = PMPI_Comm_size(comm, size);
  std::string fileName = "MpiTrace" + std::to_string(rank) + ".txt";
 // if(rank == 0){
  	char dumpStr[100];
  	sprintf(dumpStr, "MPI_Comm_size(%d, %d)", *size, line);
  	CR_DumpMpiTrace(dumpStr, rank, 1);
  //}
  return val;
}
int CR_MPI_Finalize(int line){
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  PMPI_Comm_size(MPI_COMM_WORLD, &size);
  char dumpStr[100];
  sprintf(dumpStr, "MPI_Finalize(%d, %d)", rank, line);
  //printf("finalizing %d %s\n",rank, dumpStr);
  //fflush(stdout);
  CR_DumpMpiTrace(dumpStr, rank, 2);
  //if(rank == 0){
  ////Rename the trace file of current iteration @lavleshm
  //std::string allFiles = "MpiTrace0.txt";
  //for(int procid = 1; procid < size; procid++){
  //	allFiles += " MpiTrace" + std::to_string(procid) + ".txt";
  //}

  //std::string traceFileName = "MpiTrace_" + std::to_string(rank) + ".txt";
  //std::string command1 = "cat " + allFiles + " > "+ traceFileName;
  //std::string command2 = "rm " + allFiles;
  //system(command1.c_str());
  ////system(command2.c_str());
  //}
  return PMPI_Finalize();
}
int CR_MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  int elems_to_log = num_elem_to_log < count ? num_elem_to_log : count;
  std::string logged_buf = "";
  if(datatype == MPI_CHAR or datatype == MPI_BYTE)
	logged_buf = charArrToStr((const char *)buf, elems_to_log);
  else //if(datatype == MPI_INT)
  	logged_buf = intArrToStr((const int *)buf, elems_to_log);
  assert(elems_to_log < 900);
  char dumpStr[1000];
  int dtsize;
  MPI_Type_size(datatype, &dtsize);
  sprintf(dumpStr, "MPI_Send(%d, %d, %d, %d, %d, %s, %d)", rank, dest, tag, count, dtsize, logged_buf.c_str(), line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Send(buf, count, datatype, dest, tag, comm);
}

int CR_MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  int elems_to_log = num_elem_to_log < count ? num_elem_to_log : count;
  std::string logged_buf = "";
  if(datatype == MPI_CHAR or datatype == MPI_BYTE)
	logged_buf = charArrToStr((const char *)buf, elems_to_log);
  else //if(datatype == MPI_INT)
  	logged_buf = intArrToStr((const int *)buf, elems_to_log);
  assert(elems_to_log < 900);
  char dumpStr[1000];
  int dtsize;
  MPI_Type_size(datatype, &dtsize);
  sprintf(dumpStr, "MPI_Ssend(%d, %d, %d, %d, %d, %s, %d)", rank, dest, tag, count, dtsize, logged_buf.c_str(), line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Ssend(buf, count, datatype, dest, tag, comm);
}
int CR_MPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  int elems_to_log = num_elem_to_log < count ? num_elem_to_log : count;
  std::string logged_buf = "";
  if(datatype == MPI_CHAR or datatype == MPI_BYTE)
	logged_buf = charArrToStr((const char *)buf, elems_to_log);
  else //if(datatype == MPI_INT)
  	logged_buf = intArrToStr((const int *)buf, elems_to_log);
  assert(elems_to_log < 900);
  char dumpStr[1000];
  int dtsize;
  MPI_Type_size(datatype, &dtsize);
  sprintf(dumpStr, "MPI_Bsend(%d, %d, %d, %d, %d, %s, %d)", rank, dest, tag, count, dtsize, logged_buf.c_str(), line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Bsend(buf, count, datatype, dest, tag, comm);
}
int CR_MPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  int elems_to_log = num_elem_to_log < count ? num_elem_to_log : count;
  std::string logged_buf = "";
  if(datatype == MPI_CHAR or datatype == MPI_BYTE)
	logged_buf = charArrToStr((const char *)buf, elems_to_log);
  else //if(datatype == MPI_INT)
  	logged_buf = intArrToStr((const int *)buf, elems_to_log);
  assert(elems_to_log < 900);
  char dumpStr[1000];
  int dtsize;
  MPI_Type_size(datatype, &dtsize);
  sprintf(dumpStr, "MPI_Rsend(%d, %d, %d, %d, %d, %s, %d)", rank, dest, tag, count, dtsize, logged_buf.c_str(), line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Rsend(buf, count, datatype, dest, tag, comm);
}

int CR_MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  int elems_to_log = num_elem_to_log < count ? num_elem_to_log : count;
  std::string logged_buf = "";
  if(datatype == MPI_CHAR or datatype == MPI_BYTE)
	logged_buf = charArrToStr((const char *)buf, elems_to_log);
  else //if(datatype == MPI_INT)
  	logged_buf = intArrToStr((const int *)buf, elems_to_log);
  assert(elems_to_log < 900);
  char dumpStr[1000];
  int dtsize;
  MPI_Type_size(datatype, &dtsize);
  sprintf(dumpStr, "MPI_Isend(%d, %d, %d, %p, %d, %d, %s, %d)", rank, dest, tag, request, count, dtsize, logged_buf.c_str(), line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
}

int CR_MPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  int elems_to_log = num_elem_to_log < count ? num_elem_to_log : count;
  std::string logged_buf = "";
  if(datatype == MPI_CHAR or datatype == MPI_BYTE)
	logged_buf = charArrToStr((const char *)buf, elems_to_log);
  else //if(datatype == MPI_INT)
  	logged_buf = intArrToStr((const int *)buf, elems_to_log);
  assert(elems_to_log < 900);
  char dumpStr[1000];
  int dtsize;
  MPI_Type_size(datatype, &dtsize);
  sprintf(dumpStr, "MPI_Issend(%d, %d, %d, %p, %d, %d, %s, %d)", rank, dest, tag, request, count, dtsize, logged_buf.c_str(), line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Issend(buf, count, datatype, dest, tag, comm, request);
}

int CR_MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  int elems_to_log = num_elem_to_log < count ? num_elem_to_log : count;
  std::string logged_buf = "";
  if(datatype == MPI_CHAR or datatype == MPI_BYTE)
	logged_buf = charArrToStr((const char *)buf, elems_to_log);
  else //if(datatype == MPI_INT)
  	logged_buf = intArrToStr((const int *)buf, elems_to_log);
  assert(elems_to_log < 900);
  char dumpStr[1000];
  int dtsize;
  MPI_Type_size(datatype, &dtsize);
  sprintf(dumpStr, "MPI_Ibsend(%d, %d, %d, %p, %d, %d, %s, %d)", rank, dest, tag, request, count, dtsize, logged_buf.c_str(), line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Ibsend(buf, count, datatype, dest, tag, comm, request);
}

int CR_MPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  int elems_to_log = num_elem_to_log < count ? num_elem_to_log : count;
  std::string logged_buf = "";
  if(datatype == MPI_CHAR or datatype == MPI_BYTE)
	logged_buf = charArrToStr((const char *)buf, elems_to_log);
  else //if(datatype == MPI_INT)
  	logged_buf = intArrToStr((const int *)buf, elems_to_log);
  assert(elems_to_log < 900);
  char dumpStr[1000];
  int dtsize;
  MPI_Type_size(datatype, &dtsize);
  sprintf(dumpStr, "MPI_Irsend(%d, %d, %d, %p, %d, %d, %s, %d)", rank, dest, tag, request, count, dtsize, logged_buf.c_str(), line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Irsend(buf, count, datatype, dest, tag, comm, request);
}

int CR_MPI_Recv(const void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status * status, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  int elems_to_log = (num_elem_to_log < count) ? num_elem_to_log : count;
  int symVarInt = (datatype == MPI_INT) ? __CrestIntArray((int *)buf, elems_to_log) : __CrestCharArray((char *)buf, elems_to_log); //pkalita @lavleshm
  char dumpStr[100];
  int dtsize;
  MPI_Type_size(datatype, &dtsize);
  sprintf(dumpStr, "MPI_Recv(%d, %d, %d, %d, %d, %d, x%d, %d)", rank, source, tag, count, dtsize, elems_to_log, symVarInt, line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Recv(const_cast<void *>(buf), count, datatype, source, tag, comm, status);
}

int CR_MPI_Irecv(const void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request * request, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  int elems_to_log = (num_elem_to_log < count) ? num_elem_to_log : count;
  int symVarInt = (datatype == MPI_INT) ? __CrestIntArray((int *)buf, elems_to_log) : __CrestCharArray((char *)buf, elems_to_log); //pkalita @lavleshm
  char dumpStr[100];
  int dtsize;
  MPI_Type_size(datatype, &dtsize);
  sprintf(dumpStr, "MPI_Irecv(%d, %d, %d, %p, %d, %d, %d, x%d, %d)", rank, source, tag, request, count, dtsize, elems_to_log, symVarInt, line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Irecv(const_cast<void *>(buf), count, datatype, source, tag, comm, request);
}

int CR_MPI_Wait(MPI_Request * request, MPI_Status * status, int line){
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  char dumpStr[100];
  sprintf(dumpStr, "MPI_Wait(%d, %p, %d)", rank, request, line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Wait(request, status);
}

int CR_MPI_Waitany(int count, MPI_Request array_of_requests[], int *indx,  MPI_Status * status, int line){	
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  char dumpStr[100];
  sprintf(dumpStr, "MPI_Waitany(%d, %p, %d,  %d)", rank, array_of_requests, count, line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Waitany(count, array_of_requests, indx, status);
}

int CR_MPI_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[], int line){
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  char dumpStr[100];
  sprintf(dumpStr, "MPI_Waitall(%d, %p, %d, %d)", rank, array_of_requests, count, line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Waitall(count, array_of_requests, array_of_statuses);
}

int CR_MPI_Barrier(MPI_Comm comm, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  char dumpStr[100];
  sprintf(dumpStr, "MPI_Barrier(%d, %d, %d)", rank, comm, line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Barrier(comm);
}

int CR_MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  char dumpStr[100] = "";
  int dtsize;
  MPI_Type_size(datatype, &dtsize);
  sprintf(dumpStr, "MPI_Bcast(%d, %d, %d, %d, %d)",rank, root, count*dtsize, comm,  line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Bcast(buffer, count, datatype, root, comm);
}

int CR_MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  char dumpStr[100];
  int dtsize;
  MPI_Type_size(datatype, &dtsize);
  sprintf(dumpStr, "MPI_Reduce(%d, %d, %d, %d, %d, %d)", rank, root, count*dtsize, op, comm, line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
}

int CR_MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  char dumpStr[100];
  // Here sendcount*sizeof(sendtype) will suffice to denote the data communicated by each process
  int dtsize;
  MPI_Type_size(sendtype, &dtsize);
  sprintf(dumpStr, "MPI_Gather(%d, %d, %d, %d, %d)", rank, root, sendcount*dtsize, comm, line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

int CR_MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  char dumpStr[100];
  int dtsize;
  MPI_Type_size(datatype, &dtsize);
  sprintf(dumpStr, "MPI_Allreduce(%d, %d, %d, %d, %d)", rank, count*datatype, op, comm, line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
}

int CR_MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  char dumpStr[100];
  int sdtsize;
  MPI_Type_size(sendtype, &sdtsize);
  int rdtsize;
  MPI_Type_size(recvtype, &rdtsize);
  sprintf(dumpStr, "MPI_Allgather(%d, %d, %d, %d, %d)", rank, sendcount*sdtsize, recvcount*rdtsize, comm, line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

int CR_MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  char dumpStr[100];
  // Here recvcount*sizeof(recvtype) will suffice to denote the data communicated by each process
  int dtsize;
  MPI_Type_size(recvtype, &dtsize);
  sprintf(dumpStr, "MPI_Scatter(%d, %d, %d, %d, %d)", rank, root, recvcount*dtsize, comm, line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

int CR_MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  char dumpStr[100];
  int sdtsize;
  MPI_Type_size(sendtype, &sdtsize);
  int rdtsize;
  MPI_Type_size(recvtype, &rdtsize);
  sprintf(dumpStr, "MPI_Allgather(%d, %d, %d, %d, %d)", rank, sendcount*sdtsize, recvcount*rdtsize, comm, line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

int CR_MPI_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, int line){
  int rank;
  MPI_Comm_rank(comm, &rank);
  char dumpStr[100];
  sprintf(dumpStr, "MPI_Alltoallv(%d, %d, %d)", rank, comm, line);
  CR_DumpMpiTrace(dumpStr, rank);
  return PMPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
}
