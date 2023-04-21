#include "user/uthread.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

struct uthread  all_threads[MAX_UTHREADS];
struct uthread* curr_thread;

int find_free_entry(){
    for (int i = 0; i < MAX_UTHREADS; i++)
    {
        if (all_threads[i].state == FREE){
            return i;
        }
    }
    return -1;
}

void combined_function(void (*f)(), void (*g)()) {
    f();
    g();
}

void (*wrapper_function(void (*f)(), void (*g)()))() {
    void (*combined)() = &combined_function;
    void (*result)() = ^{
        combined(f, g);
    };
    return result;
}

int uthread_create(void (*start_func)(), enum sched_priority priority){

    int entry = find_free_entry();

    if (entry == -1)
        return -1;
    
    struct uthread* thread = &all_threads[entry];
    memset(&thread->context, 0, sizeof(thread->context));
    void (*start_f)() = ^()
    {
        start_func();
        uthread_exit();
    };
    thread->context.ra = (uint64)(()->);
    thread->context.sp = thread->ustack + STACK_SIZE;
    thread->priority = priority;
    thread->state = RUNNABLE;

    return 0;
}

void uthread_yield()
{
    struct uthread* t;
    struct uthread* newT;
    enum sched_priority maxP = LOW;
    int i;
    for (t = all_threads;t < &all_threads[MAX_UTHREADS];t++)
    {
        if (t->state == RUNNABLE && t->priority >= maxP ){
            maxP = t->priority;
            newT = t;
        }
    }
    curr_thread->state = RUNNABLE;
    uswtch(&curr_thread->context,&newT->context);
    curr_thread = newT;
}


int
main(int argc, char *argv[]){
    return 1;
}