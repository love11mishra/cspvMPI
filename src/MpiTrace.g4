/* This is simple grammar representing the trace of mpi communication calls */

grammar MpiTrace;

/*trace			:	(row '\r'?'\n')*	
			;*/

trace
        :	routine ('\r'?'\n' routine | '\r'?'\n')*	
        ;

routine
        : 	ID '(' argumentList ')'	
		;

argumentList
        :	argument (',' argument)*
		;

argument
		:	string
		| 	number
		;

string
        : (Nondigit | Digit)* (Nondigit) (Digit| Nondigit)*
		;

number
        : Digit+
		;

ID
        :	'MPI_Send'
		|	'MPI_Isend'
		| 	'MPI_Ssend'
		| 	'MPI_Issend'
		| 	'MPI_Bsend'
		| 	'MPI_Ibsend'
		| 	'MPI_Rsend'
		| 	'MPI_Irsend'
		| 	'MPI_Recv'
		| 	'MPI_Irecv'
		| 	'MPI_Wait'
		|	'MPI_Waitany'
		|	'MPI_Waitall'
		|	'MPI_Test'
		|	'MPI_Testany'
		|	'MPI_Testall'
		| 	'MPI_Barrier'
        |   'MPI_Finalize'
		|	'MPI_Comm_size'
		|	'MPI_Bcast'
		|	'MPI_Reduce'
		|	'MPI_Gather'
		|	'MPI_Scatter'
		|	'MPI_Allreduce'
		|	'MPI_Allgather'
		|	'MPI_Alltoall'
		|	'MPI_Alltoallv'
		|	'branch'
        ;


Digit
        :   	[0-9]
    	;

Nondigit
        :   [a-zA-Z_+*/%=<>]
        |	'-'
		|   '['
        |   ']'
        ;
WS
        : 	[ \t\r\n]+ -> skip
		;	


