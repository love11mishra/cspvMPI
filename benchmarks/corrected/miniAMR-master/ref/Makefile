crestdir = /home/lavleshm/Academic/mpi_verify/crest-0.1.1
#CC   = ${crestdir}/cil/bin/cilly --save-temps --doCrestInstrument --merge
#CC   = ${crestdir}/cil/bin/cilly --save-temps --doCrestInstrument 
CC	  = mpicc
#LD   = ${crestdir}/cil/bin/cilly
LD   = mpic++ -g
CPPFLAGS = -D_Float128=double -I. -I${crestdir}/include -I/home/lavleshm/mpich-install/include \
		   -L/home/lavleshm/mpich-install/lib -lmpich \
		    -lstdc++
CFLAGS = -O0
LDFLAGS =
LDLIBS = -lm -L${crestdir}/lib -lcrest

EXEC = miniAMR.x

OBJS = block.o check_sum.o comm_block.o comm.o comm_parent.o comm_refine.o \
       comm_util.o driver.o init.o main.o move.o pack.o plot.o profile.o \
       rcb.o refine.o sfc.o stencil.o util.o

$(EXEC): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

clean:
	rm *.o miniAMR.x

# need dependencies

block.o: block.h proto.h comm.h timer.h

check_sum.o: block.h comm.h proto.h timer.h

comm_block.o: block.h comm.h proto.h

comm.o: block.h comm.h proto.h timer.h

comm_parent.o: block.h comm.h proto.h

comm_refine.o: block.h comm.h proto.h

comm_util.o: block.h comm.h proto.h

driver.o: block.h comm.h proto.h timer.h

init.o: block.h proto.h comm.h

main.o: block.h comm.h proto.h param.h timer.h

move.o: block.h comm.h proto.h

pack.o: block.h comm.h proto.h

plot.o: block.h comm.h proto.h

profile.o: block.h comm.h proto.h timer.h

rcb.o: block.h comm.h proto.h timer.h

refine.o: block.h comm.h proto.h timer.h

sfc.o: block.h comm.h proto.h timer.h

stencil.o: block.h comm.h proto.h

util.o: block.h comm.h proto.h timer.h
