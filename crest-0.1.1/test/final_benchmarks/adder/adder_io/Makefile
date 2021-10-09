MPICC=mpicc
GCC=gcc
RUN=mpiexec
DIFF=diff

all: compare1 compare2 end

ms_mpi: adder_ms_mpi_io.c
	$(MPICC) adder_ms_mpi_io.c -std=c11 -o ms_mpix
	$(RUN) -n 6 ./ms_mpix 100000

mpi: adder_mpi_io.c
	$(MPICC) -o mpix adder_mpi_io.c
	$(RUN) -n 6 ./mpix 100000

spec: adder_spec_io.c
	$(GCC) adder_spec_io.c -o specx
	./specx 100000

bad: adder_bad_io.c
	$(MPICC) -o badx adder_bad_io.c
	$(RUN) -n 6 ./badx 100000

compare1: spec ms_mpi
	$(DIFF) parout seqout

compare2: spec mpi
	$(DIFF) parout seqout

end:
	@echo Compare success!

clean: 
	rm -f ms_mpix mpix specx badx parout seqout badout
	rm -f *~

