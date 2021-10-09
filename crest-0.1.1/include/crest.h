/* Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
 *
 * This file is part of CREST, which is distributed under the revised
 * BSD license.  A copy of this license can be found in the file LICENSE.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
 * for details.
 */

#ifndef LIBCREST_CREST_H__
#define LIBCREST_CREST_H__

/*
 * During instrumentation, the following function calls are inserted in the
 * C code under test.
 *
 * These calls (loosely) correspond to an execution of the program
 * under test by a stack machine.  It is intended that these calls be
 * used to symbolically execute the program under test, by maintaining
 * a a symbolic stack (along with a symbolic memory map).  Specifically:
 *
 *  - A C expression (with no side effects) generates a series of Load
 *    and Apply calls corresponding to the "postfix" evaluation of the
 *    expression, using a stack (i.e. a Load indicates that a value is
 *    pushed onto the stack, and unary and binary operations are applied
 *    to one/two values popped off the stack).  For example, the expression
 *    "a*b > 3+c" would generate the instrumentation:
 *        Load(&a, a)
 *        Load(&b, b)
 *        ApplyBinOp(MULTIPLY, a*b)
 *        Load(0, 3)
 *        Load(&c, c)
 *        ApplyBinOp(ADD, 3+c)
 *        ApplyBinOp(GREATER_THAN, a*b > 3+c)
 *    Note that each Load and Apply call includes the concrete value either
 *    loaded or computed.  Also note that constants are treated as having
 *    address "0".
 *
 * - Entering the then- or else-block of an if statement generates a Branch
 *   call indicating which branch was taken.  The branch is on the value
 *   popped from the stack.  For example, "if (a*b > 3+c) ..." generates
 *   the series of Load and Apply calls above, plus one of:
 *       Branch(true_id,  1)
 *       Branch(false_id, 0)
 *
 * - An assignment statement generates a single Store call, indicating
 *   that a value is popped off the stack and stored in the given address.
 *   For example, "a = 3 + b" generates:
 *        Load(0, 3)
 *        Load(&b, b)
 *        ApplyBinOp(ADD, 3+b)
 *        Store(&a)
 *
 * - The instrumentation for function calls is somewhat complicated,
 *   because we have to handle the case where an instrumented code
 *   calls an un-instrumented function.  (We currently forbid
 *   un-instrumented code from calling back into instrumented code.)
 *
 *   * For all calls, the actual function arguments are pushed onto
 *     the stack.  In the body of the called function (if
 *     instrumented), these values are Stored in the formal
 *     parameters.  (See example below.)
 *
 *   * In the body of the called function, "return e" generates the
 *     instrumentation for expression "e", followed by a call to
 *     Return.  An void "return" statement similary generates a call
 *     to Return.
 *
 *   * If the returned value is assigned to some variable -- e.g.
 *     "z = max(a, 7)" -- then two calls are generated:
 *         HandleReturn([concrete returned value])
 *         Store(&z)
 *     If, instead, the return value is ignored -- e.g. "max(a, 7);"
 *     -- a single call to ClearStack is generated.

 *     [The difficultly here is that, if the called function was not
 *      instrumented, HandleReturn must clean up the stack -- which
 *      will still contain the arguments to the function -- and then
 *      load the concrete returned value onto the stack to then be
 *      stored.  If the called function is instrumented, then HandleReturn
 *      need not modify the stack -- it already contains a single element
 *      (the returned value).]
 *
 *    * Full example:  Consider the function "add(x, y) { return x+y; }".
 *      A call "z = add(a, 7)" generates instrumentation calls:
 *          Load(&a, a)
 *          Load(0, 7)
 *          Call(add)
 *          Store(&y)
 *          Store(&x)
 *          Load(&x, x)
 *          Load(&y, y)
 *          ApplyBinOp(ADD, x+y)
 *          Return()
 *          HandleReturn(z)
 *          Store(&z)
 *
 * - A symbolic input generates a call to create a new symbol (passing
 *   the conrete initial value for that symbol).
 *
 *   [We pass the conrete value and have signed/unsigned versions only
 *   to make it easier to exactly capture/print the concrete inputs to
 *   the program under test.]
 */

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN extern
#endif

/*
 * Type definitions.
 *
 * These macros must be kept in sync with the definitions in base/basic_types.h.
 * We use these obscure MACRO's rather than the definitions in basic_types.h
 * in an attempt to avoid clashing with names in instrumented programs
 * (and also because C does not support namespaces).
 */
#define __CREST_ID int
#define __CREST_BRANCH_ID int
#define __CREST_FUNCTION_ID unsigned int
#define __CREST_VALUE long long int
#define __CREST_ADDR unsigned long int

#define __CREST_OP int
#define __CREST_BOOL unsigned char

/*
 * Constants representing possible C operators.
 *
 * TODO(jburnim): Arithmetic versus bitwise right shift?
 */
typedef enum {
  /* binary arithmetic */
  __CREST_ADD       =  0,
  __CREST_SUBTRACT  =  1,
  __CREST_MULTIPLY  =  2,
  __CREST_DIVIDE    =  3,
  __CREST_MOD       =  4,
  /* binary bitwise operators */
  __CREST_AND       =  5,
  __CREST_OR        =  6,
  __CREST_XOR       =  7,
  __CREST_SHIFT_L   =  8,
  __CREST_SHIFT_R   =  9,
  /* binary logical operators */
  __CREST_L_AND     = 10,
  __CREST_L_OR      = 11,
  /* binary comparison */
  __CREST_EQ        = 12,
  __CREST_NEQ       = 13,
  __CREST_GT        = 14,
  __CREST_LEQ       = 15,
  __CREST_LT        = 16,
  __CREST_GEQ       = 17,
  /* unhandled binary operators */
  __CREST_CONCRETE  = 18,
  /* unary operators */
  __CREST_NEGATE    = 19,
  __CREST_NOT       = 20,
  __CREST_L_NOT     = 21,
};

/*
 * Short-cut to indicate that a function should be skipped during
 * instrumentation.
 */
#define __SKIP __attribute__((crest_skip))

/*
 * Instrumentation functions.
 */
EXTERN void __CrestInit() __SKIP;
EXTERN void __CrestLoad(__CREST_ID, __CREST_ADDR, __CREST_VALUE) __SKIP;
EXTERN void __CrestStore(__CREST_ID, __CREST_ADDR) __SKIP;
EXTERN void __CrestClearStack(__CREST_ID) __SKIP;
EXTERN void __CrestApply1(__CREST_ID, __CREST_OP, __CREST_VALUE) __SKIP;
EXTERN void __CrestApply2(__CREST_ID, __CREST_OP, __CREST_VALUE) __SKIP;
EXTERN void __CrestBranch(__CREST_ID, __CREST_BRANCH_ID, __CREST_BOOL) __SKIP;
EXTERN void __CrestCall(__CREST_ID, __CREST_FUNCTION_ID) __SKIP;
EXTERN void __CrestReturn(__CREST_ID) __SKIP;
EXTERN void __CrestHandleReturn(__CREST_ID, __CREST_VALUE) __SKIP;
EXTERN void __CrestDump(__CREST_ID, __CREST_VALUE) __SKIP;

/*
 * Functions (macros) for obtaining symbolic inputs.
 */
#define CREST_unsigned_char(x) __CrestUChar(&x)
#define CREST_unsigned_short(x) __CrestUShort(&x)
#define CREST_unsigned_int(x) __CrestUInt(&x)
#define CREST_char(x) __CrestChar(&x)
#define CREST_short(x) __CrestShort(&x)
#define CREST_int(x) __CrestInt(&x)
#define CREST_intA(x) __CrestInt(x)

//pkalita
#define  CREST_intArray(x, y) __CrestIntArray(x, y)
EXTERN int __CrestIntArray(int* x, int y) __SKIP;
#define  CREST_charArray(x, y) __CrestCharArray(x, y)
EXTERN int __CrestCharArray(char* x, int y) __SKIP;

#define CREST_unsigned_char_trace(x, c, y) __CrestUCharTrace(&x, c, #y)
#define CREST_unsigned_short_trace(x, c, y) __CrestUShortTrace(&x, c, #y)
#define CREST_unsigned_int_trace(x, c, y) __CrestUIntTrace(&x, c, #y)
#define CREST_char_trace(x, c, y) __CrestCharTrace(&x, c, #y)
#define CREST_short_trace(x, c, y) __CrestShortTrace(&x, c, #y)
#define CREST_int_trace(x, c, y) __CrestIntTrace(&x, c, #y)
#define CREST_int_trace_1(x, c, y) __CrestIntTrace_1(&x, c, #y)

#define CREST_var_map(x,t,q) __CrestVarMap(&x, #x, t, #q)
#define CREST_var_map_1(x,y,t,q) __CrestVarMap_1(x, y, t, q)//aakanksha
#define CREST_var_map_gdb(x,y,t,q) __CrestVarMap_gdb(x, y, t, q)//aakanksha
#define CREST_log_state_1(x) __CrestLogState_1(x)
#define CREST_log_state_gdb(x) __CrestLogState_gdb(x)
#define CREST_log_state(x, r_w, line, name,val,addr) __CrestLogState(x, r_w, line, name, val,addr)//aakanksha
#define CREST_print(x, r_w, line, name,val,addr) __CrestPrint(x, r_w, line, name, val,addr)//aakanksha
#define CREST_get_time_stamp() __CrestGetTimeStamp()//aakanksha
#define CREST_print_input(x,y) __CrestPrintInput(x,y)//aakanksha
#define CREST_log_pc(x) __CrestLogPC(x)
#define CREST_log_pc_on_gdbquery(x) __CrestLogPCOnGdbQuery(x)
#define CREST_log_spec(op,op1,op2) __CrestLogSpec(op,op1,op2)

EXTERN void __CrestUChar(unsigned char* x) __SKIP;
EXTERN void __CrestUShort(unsigned short* x) __SKIP;
EXTERN void __CrestUInt(unsigned int* x) __SKIP;
EXTERN void __CrestChar(char* x) __SKIP;
EXTERN void __CrestShort(short* x) __SKIP;
EXTERN void __CrestInt(int* x) __SKIP;

EXTERN void __CrestUCharTrace(unsigned char* x, unsigned char c, char* iprange) __SKIP;
EXTERN void __CrestUShortTrace(unsigned short* x, unsigned short c, char* iprange) __SKIP;
EXTERN void __CrestUIntTrace(unsigned int* x, unsigned int c, char* iprange) __SKIP;
EXTERN void __CrestCharTrace(char* x, char c, char* iprange) __SKIP;
EXTERN void __CrestShortTrace(short* x, short c, char* iprange) __SKIP;
EXTERN void __CrestIntTrace(int* x, int c, char* iprange) __SKIP;
EXTERN void __CrestIntTrace_1(int* x, int c, char* iprange) __SKIP;

EXTERN void __CrestVarMap(void* addr, char *name, int tp, char* trigger) __SKIP;
EXTERN void __CrestVarMap_1(void* addr, char *name, int tp, char* trigger) __SKIP;
EXTERN void __CrestVarMap_gdb(long unsigned int addr, char *name, int tp, char* trigger) __SKIP;
EXTERN void __CrestLogState(unsigned int x, int r_w, int line, char *varname, int val,int *addr) __SKIP;//aakanksha
EXTERN void __CrestPrint(unsigned int x, int r_w, int line, char *varname, int val, int *addr) __SKIP;//aakanksha
EXTERN void __CrestLogState_1(unsigned int x) __SKIP;
EXTERN void __CrestLogState_gdb(unsigned int x) __SKIP;
EXTERN void __CrestLogPC(unsigned int x) __SKIP;
EXTERN void __CrestLogPCOnGdbQuery(unsigned int x) __SKIP;

EXTERN void __CrestLogSpec(char *op,int *op1,int *op2) __SKIP;
EXTERN int __CrestGetTimeStamp() __SKIP;
EXTERN void __CrestPrintInput(char *name,int val) __SKIP;

//Adding Wrappers for /MPI routines @lavleshm
#include<mpi.h>
//int num_elem_to_log; //default number of elements of send buffer to log

//#define MPI_Init(argc, argv) CR_MPI_Init(&argc, &argv, __LINE__);
#define MPI_Comm_size(comm, size) CR_MPI_Comm_size(comm, size, CRESTK, __LINE__);
#define MPI_Finalize() CR_MPI_Finalize(__LINE__);
#define MPI_Send(buf, count, datatype, dest, tag, comm) CR_MPI_Send(buf, count, datatype, dest, tag, comm, __LINE__);
#define MPI_Ssend(buf, count, datatype, dest, tag, comm) CR_MPI_Ssend(buf, count, datatype, dest, tag, comm, __LINE__);
#define MPI_Bsend(buf, count, datatype, dest, tag, comm) CR_MPI_Bsend(buf, count, datatype, dest, tag, comm, __LINE__);
#define MPI_Rsend(buf, count, datatype, dest, tag, comm) CR_MPI_Rsend(buf, count, datatype, dest, tag, comm, __LINE__);

#define MPI_Isend(buf, count, datatype, dest, tag, comm, request) CR_MPI_Isend(buf, count, datatype, dest, tag, comm, request, __LINE__);
#define MPI_Issend(buf, count, datatype, dest, tag, comm, request) CR_MPI_Issend(buf, count, datatype, dest, tag, comm, request, __LINE__);
#define MPI_Ibsend(buf, count, datatype, dest, tag, comm, request) CR_MPI_Ibsend(buf, count, datatype, dest, tag, comm, request, __LINE__);
#define MPI_Irsend(buf, count, datatype, dest, tag, comm, request) CR_MPI_Irsend(buf, count, datatype, dest, tag, comm, request, __LINE__);

#define MPI_Recv(buf, count, datatype, source, tag, comm, status) CR_MPI_Recv(buf, count, datatype, source, tag, comm, status, __LINE__);
#define MPI_Irecv(buf, count, datatype, source, tag, comm, request) CR_MPI_Irecv(buf, count, datatype, source, tag, comm, request, __LINE__);

#define MPI_Wait(request, status) CR_MPI_Wait(request, status, __LINE__);
#define MPI_Waitany(count, array_of_requests, indx, status) CR_MPI_Waitany(count, array_of_requests, indx, status, __LINE__);
#define MPI_Barrier(comm) CR_MPI_Barrier(comm, __LINE__); 
#define MPI_Bcast(buffer, count, datatype, root, comm) CR_MPI_Bcast(buffer, count, datatype, root, comm, __LINE__);
#define MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm) CR_MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm, __LINE__);
#define MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, __LINE__);
#define MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm) CR_MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm, __LINE__);
#define MPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm) CR_MPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, __LINE__);
#define MPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm) CR_MPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, __LINE__);
#define MPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm) CR_MPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, __LINE__);
#define MPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm) CR_MPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, __LINE__);
//Dump the mpi calls as excuted, last argument in each call-string is the source line number
EXTERN void CR_DumpMpiTrace(char * str) __SKIP;

EXTERN int CR_MPI_Init( int *argc, char ***argv, int line) __SKIP;
EXTERN int CR_MPI_Comm_size(MPI_Comm comm, int * size, int crestK, int line) __SKIP;
EXTERN int CR_MPI_Finalize(int line) __SKIP;
EXTERN int CR_MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int line) __SKIP;
EXTERN int CR_MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int line) __SKIP;
EXTERN int CR_MPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int line) __SKIP;
EXTERN int CR_MPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int line) __SKIP;

EXTERN int CR_MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int line) __SKIP;
EXTERN int CR_MPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int line) __SKIP;
EXTERN int CR_MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int line) __SKIP;
EXTERN int CR_MPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int line) __SKIP;

EXTERN int CR_MPI_Recv(const void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status * status, int line) __SKIP;
EXTERN int CR_MPI_Irecv(const void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request * request, int line) __SKIP;

EXTERN int CR_MPI_Wait(MPI_Request * request, MPI_Status * status, int line) __SKIP;
EXTERN int CR_MPI_Waitany(int count, MPI_Request array_of_requests[], int *indx,  MPI_Status * status, int line) __SKIP;
EXTERN int CR_MPI_Barrier(MPI_Comm comm, int line) __SKIP;
EXTERN int CR_MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, int line) __SKIP;
EXTERN int CR_MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, int line) __SKIP;
EXTERN int CR_MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, int line) __SKIP;
EXTERN int CR_MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, int line) __SKIP;
EXTERN int CR_MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, int line) __SKIP;
EXTERN int CR_MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, int line) __SKIP;
EXTERN int CR_MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, int line) __SKIP;
EXTERN int CR_MPI_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, int line) __SKIP;

//pkalita
EXTERN void symbolicForMPIRead(void*) __SKIP;
EXTERN void symbolicForMPIWrite(void*) __SKIP;

#endif  /* LIBCREST_CREST_H__ */
