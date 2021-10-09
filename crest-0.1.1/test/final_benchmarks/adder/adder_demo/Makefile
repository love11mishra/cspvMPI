MPICC=mpicc
GCC=gcc
RUN=mpiexec
DIFF=diff

all:  compare end

mpi: adder_mpi.c
	$(MPICC) adder_mpi.c -o mpix
	$(RUN) -n 6 ./mpix > OUTMPI

spec: adder_spec.c	
	$(GCC) adder_spec.c -o specx
	./specx > OUTSPEC

compare: mpix specx
	$(DIFF) OUTMPI OUTSPEC

end: 
	@echo Compare success

clean:
	rm -f specx mpix OUTSPEC OUTMPI
	rm -f *~