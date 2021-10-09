#   ---------------------------
#   Compiler Flags
#   ---------------------------

#MPICC = mpixlc_r 
#MPICXX = mpicxx
crestdir = /home/lavleshm/Academic/mpi_verify/crest-0.1.1
MPICC = ${crestdir}/cil/bin/cilly --doCrestInstrument --save-temps 
MPICXX = mpicc

KMI_LIB = -L../src -lkmi  -L${crestdir}/lib -lcrest

# HPM_LIB = /soft/perftools/hpctw/lib/libmpihpm.a /bgsys/drivers/ppcfloor/bgpm/lib/libbgpm.a 

MPI_CFLAGS = -I../src -DKMI_CONFIG_BINARY_STRING -DKMI_CONFIG_SORTING -DKMI_CONFIG_INDEXED_ARRAY -DKMI_CONFIG_SAMPLING_ITERATION -DKMI_CONFIG_INDEXED_ARRAY_HASH -DKMI_PROFILING_ALLTOALL \
    -I${DIR}/include \
	-I/home/lavleshm/mpich-install/include \
	-L/home/lavleshm/mpich-install/lib -lmpich \
    -L${DIR}/lib -lcrest -lstdc++	

all: BENCH_QUERY 

KMI_C_OBJ = bench_query.o 

$(KMI_C_OBJ): %.o: %.c
	$(MPICC) $(MPI_CFLAGS) -c $< -o $@

BENCH_QUERY: $(KMI_C_OBJ)
	$(MPICC)  -o BENCH_QUERY $(MPI_CFLAGS) $(KMI_C_OBJ) $(KMI_LIB) 
clean:
	rm -f *.a *.o a.out core* BENCH_QUERY
