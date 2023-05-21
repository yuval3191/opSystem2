#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"
#include "uthread.h"

void kthread_start_func(void){
 for(int i=0; i<10; i++){
    sleep(10); // simulate work
  }
  kthread_exit(0);
  printf("kthread_exit failed\n");
  exit(1);
}

int testFunc(){
    return 15;
}

int
main(int argc, char *argv[])
{

    int (*func_pointer)();
    func_pointer = testFunc;
    uint64 stack_a = (uint64)malloc(STACK_SIZE);
    printf("%p\n",kthread_start_func);
    printf("%p\n",func_pointer);
    int kt_a = kthread_create(kthread_start_func, stack_a, STACK_SIZE);
    if(kt_a <= 0){
        printf("kthread_create failed\n");
        exit(1);
    }
  exit(0);
}
