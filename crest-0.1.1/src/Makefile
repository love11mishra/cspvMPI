YICES_DIR=/home/lavleshm/Academic/yices-1.0.40

CC=mpicxx
CFLAGS = -I. -I$(YICES_DIR)/include -I/home/lavleshm/mpich-install/include -Wall -std=c++0x -Wno-narrowing -O0 -g -no-pie
CXXFLAGS = $(CFLAGS)
LDFLAGS = -L$(YICES_DIR)/lib -L/home/lavleshm/mpich-install/lib -O0 -g
LOADLIBES = -lyices -lmpich 

BASE_LIBS = base/basic_types.o base/symbolic_execution.o \
            base/symbolic_interpreter.o base/symbolic_path.o \
            base/symbolic_predicate.o base/symbolic_expression.o \
            base/yices_solver.o


all: libcrest/libcrest.a run_crest/run_crest \
     process_cfg/process_cfg tools/print_execution \
     install

libcrest/libcrest.a:  libcrest/crest.o  $(BASE_LIBS) libcrest/mpiWrappers.o
	$(AR) rsv $@ $^

run_crest/run_crest: run_crest/concolic_search.o $(BASE_LIBS)

tools/print_execution: $(BASE_LIBS)

install:
	cp libcrest/libcrest.a ../lib
	cp run_crest/run_crest ../bin
	cp process_cfg/process_cfg ../bin
	cp tools/print_execution ../bin
	cp libcrest/crest.h ../include

clean:
	rm -f libcrest/libcrest.a run_crest/run_crest
	rm -f process_cfg/process_cfg tools/print_execution
	rm -f */*.o */*~ *~
