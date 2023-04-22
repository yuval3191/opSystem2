#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#include "uthread.h"

volatile enum sched_priority x;

void uthread_a_start_func(void){
  printf("started a\n");
  if(x != MEDIUM){
    printf("sched policy failed\n");
    exit(1);
  }
  if(uthread_get_priority() != LOW){
    printf("uthread_get_priority failed\n");
    exit(1);
  }
  for(int i=0; i<10; i++){
    sleep(10); // simulate work
  }
  uthread_exit();
  printf("uthread_exit failed\n");
  exit(1);
}

void uthread_b_start_func(void){
  printf("started b\n");
  for(int i=0; i<10; i++){
    sleep(10); // simulate work
  }
  printf("b get p\n");
  x = uthread_get_priority();
  printf("b after p\n");
  uthread_exit();
  printf("uthread_exit failed\n");
  exit(1);
}

int
main(int argc, char *argv[])
{
  x = HIGH;
  uthread_create(uthread_a_start_func, LOW);
  uthread_create(uthread_b_start_func, MEDIUM);
  uthread_start_all();
  printf("uthread_start_all failed\n");
  exit(1);
}   