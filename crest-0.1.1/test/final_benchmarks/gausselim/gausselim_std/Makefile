RUN = mpiexec
FLAGS = -I/opt/local/include -L/opt/local/lib -std=c11
MPICC = mpicc
CC = cc
INPUTS = 5 6 1 2 3 0 0 4 5 6 7 0 8 9 3 2 1 -1 -2 -5 0 9 6 -7 5 -4 3 2 3 4 -5 -6

all: compare1 compare2 end

spec: gausselim_spec
	./gausselim_spec $(INPUTS) > specout

rowdist: gausselim_rowdist
	$(RUN) -n 5 ./gausselim_rowdist $(INPUTS) > rowdistout

striperows: gausselim_striperows
	$(RUN) -n 3 ./gausselim_striperows $(INPUTS) > striperowsout

bad: gausselim_bad
	./gausselim_bad $(INPUTS) > badout

gausselim_spec: gausselim_spec.c Makefile
	$(CC) -o gausselim_spec $(FLAGS) gausselim_spec.c

gausselim_rowdist: gausselim_rowdist.c Makefile
	$(MPICC) -o gausselim_rowdist $(FLAGS) gausselim_rowdist.c

gausselim_striperows: gausselim_striperows.c Makefile
	$(MPICC) -o gausselim_striperows $(FLAGS) gausselim_striperows.c

gausselim_bad: gausselim_bad.c Makefile
	$(CC) -o gausselim_bad $(FLAGS) gausselim_bad.c

compare1: spec rowdist
	diff specout rowdistout

compare2: spec striperows
	diff specout striperowsout

end:
	@echo Compare success!

clean:
	rm -f *~ gausselim_spec gausselim_rowdist gausselim_striperows
	rm -f gausselim_bad specout rowdistout badout striperowsout