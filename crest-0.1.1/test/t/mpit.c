// From MPI report

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
// get value of the cvar
int getValue_int_comm(int index, MPI_Comm comm, int *val) {
  int err,count;
  MPI_T_cvar_handle handle;

  /* This example assumes that the variable index */
  /* can be bound to a communicator */

  err=MPI_T_cvar_handle_alloc(index,&comm,&handle,&count);
  if (err!=MPI_SUCCESS) return err;

  /* The following assumes that the variable is */
  /* represented by a single integer */

  err=MPI_T_cvar_read(handle,val);
  if (err!=MPI_SUCCESS) return err;

  err=MPI_T_cvar_handle_free(&handle);
  return err;
}

int main(int argc, char *argv[]) {

  int i, err, num, namelen, bind, verbose, scope;
  int threadsupport;
  int len = 128;
  char name[100];
  MPI_Datatype datatype;

  err=MPI_T_init_thread(MPI_THREAD_SINGLE,&threadsupport);
  if (err!=MPI_SUCCESS)
    return err;

  err=MPI_T_cvar_get_num(&num);
  if (err!=MPI_SUCCESS)
    return err;

  printf ("%d\n", num);
  for (i=0; i<num; i++) {
    namelen=100;
    err=MPI_T_cvar_get_info(i, name, &namelen,
                            &verbose, &datatype, NULL,
                            NULL, NULL, /*no description */
                            &bind, &scope);
    //if (err!=MPI_SUCCESS || err!=MPI_T_ERR_INVALID_INDEX) return err;
    int *val = (int *) malloc (len*sizeof(int));
    getValue_int_comm(i, MPI_COMM_WORLD, val);
    if(strcmp(name, "MPIR_CVAR_CH3_EAGER_MAX_MSG_SIZE")== 0){
			printf("Var %i: %s\n", i, name);
			printf("Var %i: %d\n", i, *val);
	}
  }

  err=MPI_T_finalize();
  if (err!=MPI_SUCCESS)
    return 1;
  else
    return 0;

} 
