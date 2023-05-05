#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

extern struct proc proc[NPROC];

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&mykthread()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

void kthreadinit(struct proc *p)
{
  struct kthread *kt;
  for (kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    initlock(&kt->lock,"kthread_lock");
    kt->state = UNUSED;
    kt->myproc = p;
    // WARNING: Don't change this line!
    // get the pointer to the kernel stack of the kthread
    kt->kstack = KSTACK((int)((p - proc) * NKT + (kt - p->kthread)));
  }
}

struct kthread *mykthread()
{
  push_off();
  struct cpu *c = mycpu();
  struct kthread *t = c->thread;
  pop_off();
  return t;
}

int
alloctid(struct proc* p)
{
  int tid;
  
  acquire(&p->counter_lock);
  tid = p->tcounter;
  p->tcounter = p->tcounter + 1;
  release(&p->counter_lock);

  return tid;
}

struct kthread*
allockthread(struct proc* p)
{
  struct kthread *kt;

  for(kt = p->kthread; kt < &p->kthread[NKT]; kt++) {
    acquire(&kt->lock);
    if(kt->state == UNUSED) {
      goto found;
    } else {
      release(&kt->lock);
    }
  }
  return 0;

found:
  kt->tid = alloctid(p);
  kt->state = USED;


  // Allocate a trapframe page.
  // struct trapframe* trpframe = get_kthread_trapframe(p,kt);

  if((kt->trapframe = get_kthread_trapframe(p,kt)) == 0){
    freekthread(kt);
    release(&kt->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&kt->context, 0, sizeof(kt->context));
  kt->context.ra = (uint64)forkret;
  // kt->context.sp = kt->kstack + PGSIZE - 1;
  kt->context.sp = kt->kstack + PGSIZE;

  return kt;
}

void
freekthread(struct kthread* kt)
{
  kt->trapframe = 0;
  kt->chan = 0;
  kt->killed = 0;
  kt->myproc = 0;
  kt->tid = 0;
  kt->xstate = 0;
  kt->state = UNUSED;

  memset(&kt->context, 0, sizeof(kt->context));
}

struct trapframe *get_kthread_trapframe(struct proc *p, struct kthread *kt)
{
  return p->base_trapframes + ((int)(kt - p->kthread));
}

int kthread_create( uint64 start_func, uint64 stack, uint stack_size )
{
  int tid;
  struct kthread *nkt;
  struct kthread *kt = mykthread();
  struct proc *p = myproc();

  // Allocate kthread.
  if((nkt = allockthread(p)) == 0){
    return -1;
  }
  tid = nkt->tid;

  // copy saved user registers.
  *(nkt->trapframe) = *(kt->trapframe);

    // Cause fork to return 0 in the child.
  nkt->trapframe->a0 = 0;
  nkt->myproc = p;

  nkt->state = RUNNABLE;

  nkt->trapframe->epc = (uint64)(start_func);      // user program counter
  nkt->trapframe->sp = kt->kstack + stack_size;  // user stack pointer

  release(&nkt->lock);
  return tid;
}

int kthread_kill(int ktid)
{
  struct proc *p = myproc();
  struct kthread *kt;

  for(kt = p->kthread; kt < &p->kthread[NKT]; kt++) {
    acquire(&kt->lock);
    if(kt->tid == ktid)
    {
      kt->killed = 1;
      if (kt->state == SLEEPING){
        kt->state = RUNNABLE;
      }
      release(&kt->lock);
      return 0;
    }
    release(&kt->lock);
  }
  return -1;
}
void kthread_exit(int status)
{
  struct kthread* kt = mykthread();
  acquire(&kt->lock);
  kt->state = ZOMBIE;
  kt->xstate = status;
  release(&kt->lock);
}

int kthread_join(int ktid, uint64 status)
{
  int tid;
  int found = 0;
  struct proc *p = myproc();
  struct kthread *kt;
  

  for(;;){
    // Scan through table looking for exited children.
    for(kt = p->kthread; kt < &p->kthread[NKT]; kt++){
      if(kt->tid == ktid){
        // make sure the child isn't still in exit() or swtch().
        acquire(&kt->lock);

        if(kt->state == ZOMBIE){
          // Found one.

          if(kt->xstate || (status != 0 && copyout(p->pagetable, status, (char *)&kt->xstate,sizeof(kt->xstate)) < 0))
          {
            release(&kt->lock);
            return -1;
          }
          freeproc(kt);
          release(&kt->lock);
          return 0;
        }
        release(&kt->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!found || killed(p)){
      // release(&wait_lock);
      return -1;
    }
    
    // Wait for a child to exit.
    // sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}