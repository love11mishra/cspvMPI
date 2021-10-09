#include <pthread.h>
#include <assert.h>
#include <stdio.h>

void observer(int arg_count,...);

int hack = 0;
int flag1 = 0, flag2 = 0; // N integer flags 

int assume(int c)
{
    assert(c);
}

void* thr1(void * arg) {
  while (1) {
    flag1 = 1;
    if(flag2 > 3)
        exit(0); 
    flag1 = 3;
    if (flag2 == 1) {
      flag1 = 2;
      if(flag2 != 4)
          exit(0);
    }
    flag1 = 4;
    int th1=flag2;
    if(flag2 > 2)
        exit(0);
    // begin critical section
    // end critical section
    if(2 < flag2 && flag2 < 3)
        exit(0);
    flag1 = 0;
  }
}

void* thr2(void * arg) {
  while (1) {
    flag2 = 1;
    if(flag1 > 3)
        exit(0);
    flag2 = 3;
    if (flag1 == 1) {
      flag2 = 2;
      if(flag1 == 4)
          exit(0);
    }
    flag2 = 4;
    int th2=flag1;
    if(flag1 > 2)
        exit(0);
    // begin critical section
    // end critical section
    if(2 < flag1 && flag1 < 3)
        exit(0);
    flag2 = 0;
  }
}

int main()
{
    hack = 0;
    pthread_t t_id1, t_id2;
    input("flag1",&flag1,0,0); // For making x symbolic,(0,0) ->(timestamp,concrete value)
    input("flag2",&flag2,1,0);
  //__CPROVER_ASYNC_1: thr1(0);
    pthread_create(&t_id1,NULL,thr1,NULL);
    pthread_create(&t_id1,NULL,thr2,NULL);
  //thr2(0);
   pthread_join(t_id1,NULL);
    pthread_join(t_id2,NULL);
}
