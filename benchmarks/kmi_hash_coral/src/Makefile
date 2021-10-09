#   ---------------------------
#   Compiler Flags
#   ---------------------------

#MPICC = mpixlc_r 
#MPICXX = mpicxx
crestdir =/home/lavleshm/Academic/mpi_verify/crest-0.1.1
MPICC = ${crestdir}/cil/bin/cilly  --doCrestInstrument --save-temps 
MPICXX = ${MPICC}

#LIB_TIMEBASE_PATH = /home/morozov/lib
MPI_CFLAGS = -I. -O0 -g -pg -DKMI_CONFIG_BINARY_STRING -DKMI_CONFIG_SORTING -DKMI_CONFIG_INDEXED_ARRAY -DKMI_CONFIG_SAMPLING_ITERATION -DKMI_CONFIG_INDEXED_ARRAY_HASH -DKMI_PROFILING_ALLTOALL \
    -I${DIR}/include \
	-I/home/lavleshm/mpich-install/include \
	-L/home/lavleshm/mpich-install/lib -lmpich \
    -L${DIR}/lib -lcrest -lstdc++	
Q_AR = ar #${crestdir}/cil/bin/cilly --merge --mode=AR

all: libkmi.a

KMI_C_OBJ = binary.o  db.o  debug.o  hash_table.o  init.o  iquery.o  malloc.o  partition.o  query.o  sorted_array.o  string.o  table.o 

$(KMI_C_OBJ): %.o: %.c
	$(MPICC) $(MPI_CFLAGS) -c $< -o $@

libkmi.a: $(KMI_C_OBJ)
	$(Q_AR) -rcs $@ $(KMI_C_OBJ)

clean:
	rm -f *.a *.o a.out core*
