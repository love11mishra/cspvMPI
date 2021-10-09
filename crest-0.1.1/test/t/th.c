#include <stdio.h>
#include <crest.h>
#include <stdlib.h>
#include <pthread.h>
int hack = 0;
// A normal C function that is executed as a thread 
// when its name is specified in pthread_create() 
void *myThreadFun(void *vargp)
{
        int val = (int*)vargp;
        if(val == 1)
                
                printf(" Thread 1: %s\n", val);
        else
                printf(" Thread 2: %s\n", val);
        return NULL;
}

int main()
{
        pthread_t thread_id1, thread_id2;
        int in1, in2;
        CREST_int(in1);
        CREST_int(in2);
        printf("Before Thread\n");
        pthread_create(&thread_id1, NULL, myThreadFun, (void*)in1);
        pthread_join(thread_id1, NULL);
        pthread_create(&thread_id2, NULL, myThreadFun, (void*)in2);
        pthread_join(thread_id2, NULL);
        printf("After Thread\n");
        exit(0);
}
