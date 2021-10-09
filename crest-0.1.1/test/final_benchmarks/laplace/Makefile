# on a Mac:
RUN = mpiexec
FLAGS = -I/opt/local/include -L/opt/local/lib
MPICC = mpicc
CC = cc

all: seq par bad

seq: laplace_spec
	./laplace_spec > seqout

par: laplace_rowdist
	$(RUN) -n 4 ./laplace_rowdist > parout

bad: laplace_bad
	$(RUN) -n 4 ./laplace_bad > badout

laplace_spec: laplace_spec.c Makefile
	$(CC) -o laplace_spec $(FLAGS) laplace_spec.c

laplace_rowdist: laplace_rowdist.c Makefile
	$(MPICC) -o laplace_rowdist $(FLAGS) laplace_rowdist.c

laplace_bad: laplace_bad.c Makefile
	$(MPICC) -o laplace_bad $(FLAGS) laplace_bad.c

clean:
	rm -f *~ laplace_spec laplace_rowdist laplace_bad seqout parout badout
