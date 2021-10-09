RUN = mpiexec
FLAGS = -I/opt/local/include -L/opt/local/lib -std=c11
MPICC = mpicc
CC = cc
INPUTS = data

all: compare1 compare2 end

spec: gausselim_spec
	./gausselim_spec $(INPUTS)

rowdist: gausselim_rowdist
	$(RUN) -n 6 ./gausselim_rowdist $(INPUTS)

striperows: gausselim_striperows
	$(RUN) -n 3 ./gausselim_striperows $(INPUTS)

bad: gausselim_bad
	./gausselim_bad $(INPUTS)

gausselim_spec: gausselim_spec_io.c Makefile
	$(CC) -o gausselim_spec $(FLAGS) gausselim_spec_io.c

gausselim_rowdist: gausselim_rowdist_io.c Makefile
	$(MPICC) -o gausselim_rowdist $(FLAGS) gausselim_rowdist_io.c

gausselim_striperows: gausselim_striperows_io.c Makefile
	$(MPICC) -o gausselim_striperows $(FLAGS) gausselim_striperows_io.c

gausselim_bad: gausselim_bad_io.c Makefile
	$(CC) -o gausselim_bad $(FLAGS) gausselim_bad_io.c

compare1: spec rowdist
	diff specout rowdistout

compare2: spec striperows
	diff specout striperowsout

end:
	@echo Compare success!

clean:
	rm -f *~ gausselim_spec gausselim_rowdist gausselim_striperows
	rm -f gausselim_bad specout rowdistout badout striperowsout