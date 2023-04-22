#include "user/uthread.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

struct uthread  all_threads[MAX_UTHREADS];
struct uthread* curr_thread;

int num_of_threads = 0;

int find_free_entry(){
    for (int i = 0; i < MAX_UTHREADS; i++)
    {
        if (all_threads[i].state == FREE){
            return i;
        }
    }
    return -1;
}


int uthread_create(void (*start_func)(), enum sched_priority priority){

    int entry = find_free_entry();

    if (entry == -1)
        return -1;
    
    struct uthread* thread = &all_threads[entry];
    memset(&thread->context, 0, sizeof(thread->context));
    thread->context.ra = (uint64)start_func;
    thread->context.sp = (uint64)&(thread->ustack) + STACK_SIZE;
    thread->priority = priority;
    thread->state = RUNNABLE;
    num_of_threads++;

    return 0;
}

void usched()
{
    struct uthread* t;
    struct uthread* newT = 0;
    enum sched_priority maxP = LOW;
    for (t = all_threads;t < &all_threads[MAX_UTHREADS];t++)
    {
        if (t->state == RUNNABLE && t->priority >= maxP ){
            maxP = t->priority;
            newT = t;
        }
    }

    t = curr_thread;
    curr_thread = newT;

    curr_thread->state = RUNNING;
    uswtch(&t->context,&newT->context);
}

void uthread_yield()
{
    curr_thread->state = RUNNABLE;
    usched();
}

void uthread_exit()
{
    num_of_threads--;

    curr_thread->priority = 0;
    curr_thread->state  = FREE;
    if (num_of_threads == 0)
        exit(0);
    else
        usched();
}

enum sched_priority uthread_set_priority(enum sched_priority priority){
    enum sched_priority oldP = curr_thread->priority;
    curr_thread->priority = priority;
    return oldP;
}

enum sched_priority uthread_get_priority(){
    return curr_thread->priority;
}

int uthread_start_all(){

    static int flag = 1;

    if (flag)
    {
        struct uthread* t;
        struct uthread* newT = 0;
        enum sched_priority maxP = LOW;
        for (t = all_threads;t < &all_threads[MAX_UTHREADS];t++)
        {
            if (t->state == RUNNABLE && t->priority >= maxP ){
                maxP = t->priority;
                newT = t;
            }
        }
        curr_thread = newT;
        newT->state = RUNNING;
        void (*fptr)() = (void (*)())(curr_thread->context.ra);
        (*fptr)();
        exit(0);
    }
    else{
        return -1;
    }
}

struct uthread* uthread_self(){
    return curr_thread;
}