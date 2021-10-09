RUN = mpiexec
FLAGS = -I/usr/local/include -L/usr/local/lib -lgd -std=c11
GDARGS=-DK=0.1 -DM=100 -DNSTEPS=150 -DWSTEP=10 -DNX=10 -DPWIDTH=2

all: compare end

#################### compile parallel programs ####################
mpi: diffusion2d_mpi.c
	mpicc diffusion2d_mpi.c -o mpix -std=c11

#################### compile sequential programs ####################
spec: diffusion2d_spec.c
	gcc diffusion2d_spec.c -o specx -std=c11

#################### run parallel programs ####################
mpirun: mpi
	mpiexec -n 4 ./mpix 2 2 > OUTMPI

#################### run sequential programs ####################
specrun: spec
	./specx > OUTSPEC

#################### compare ####################
compare: mpirun specrun
	diff OUTSPEC OUTMPI

end:
	@echo Compare success!

clean: 
	rm -f specx mpix OUTMPI OUTSPEC
	rm -f *~