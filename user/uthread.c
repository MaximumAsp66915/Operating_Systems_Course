// user/uthread.c — cooperative user-level threads (hw3, part آ).
//
// YOUR JOB: implement thread_create(), thread_schedule(), thread_yield()
// (below, marked TODO).
//
// Context switch happens in user/uthread_switch.S (also marked TODO).
//
// Rules:
//   - A context holds only callee-saved registers: ra, sp, s0..s11.
//   - thread_create(func) sets up a fresh thread: ra = func,
//     sp = top of its 8 KiB stack, state = RUNNABLE.
//   - thread_schedule() picks the next RUNNABLE thread in round-robin
//     order starting right after the current one (wrap around).
//     If none is runnable, hand control back to thread 0 (main).
//   - thread_yield() marks the current thread RUNNABLE and reschedules.
//
// Do not change the functions below (worker0/1/2 and main).
//
// Expected output (deterministic):
//
//   uthread: thread 0 tick 0
//   uthread: thread 1 tick 0
//   uthread: thread 2 tick 0
//   uthread: thread 0 tick 1
//   ... (round-robin, ticks 0..2 for each thread)
//   uthread: thread 0 finished
//   uthread: thread 1 finished
//   uthread: thread 2 finished
//   uthread: done

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NTHREAD   4   // slot 0 = main, slots 1..3 = workers
#define STACK_SIZE 8192
#define NTICKS     3

enum { FREE, RUNNING, RUNNABLE };

struct thread_context {
  uint64 ra;
  uint64 sp;
  uint64 s0, s1, s2, s3;
  uint64 s4, s5, s6, s7;
  uint64 s8, s9, s10, s11;
};

struct thread {
  char stack[STACK_SIZE] __attribute__((aligned(16)));
  struct thread_context context;
  int state;
};

struct thread all_thread[NTHREAD];
struct thread *current_thread;

extern void thread_switch(uint64 old_ctx, uint64 new_ctx);
void thread_schedule(void);

void
thread_init(void)
{
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
}

// TODO: create a new thread running func.
// Find a FREE slot, mark it RUNNABLE, set context.ra = func and
// context.sp = the top of that slot's stack. Return 0 on success,
// -1 if no free slot.
int
thread_create(void (*func)(void))
{
  for (int i = 1; i < NTHREAD; i++) {
    if (all_thread[i].state == FREE) {
      struct thread *t = &all_thread[i];

      memset(&t->context, 0, sizeof(t->context));

      t->context.ra = (uint64)func;

      t->context.sp =
        ((uint64)(t->stack + STACK_SIZE)) & ~((uint64)0xF);

      t->state = RUNNABLE;

      return 0;
    }
  }

  return -1;
}

// TODO: mark the current thread RUNNABLE and reschedule.
void
thread_yield(void)
{
  current_thread->state = RUNNABLE;
  thread_schedule();
}

// TODO: switch to the next RUNNABLE thread in round-robin order
// starting just after current_thread (wrapping around).
// If none is RUNNABLE, switch back to thread 0 (main) unless the
// current thread is already main (in which case just return).
void
thread_schedule(void)
{
  struct thread *next_thread = 0;
  int current_index = current_thread - all_thread;

  for (int i = 1; i < NTHREAD; i++) {
    int index = (current_index + i) % NTHREAD;

    if (all_thread[index].state == RUNNABLE) {
      next_thread = &all_thread[index];
      break;
    }
  }

  if (next_thread == 0) {
    if (current_index == 0) {
      return;
    }

    next_thread = &all_thread[0];
  }

  if (next_thread == current_thread) {
    return;
  }

  struct thread *previous_thread = current_thread;

  next_thread->state = RUNNING;
  current_thread = next_thread;

  thread_switch(
    (uint64)&previous_thread->context,
    (uint64)&next_thread->context
  );
}

static void
worker(int id)
{
  for (int tick = 0; tick < NTICKS; tick++) {
    printf("uthread: thread %d tick %d\n", id, tick);
    thread_yield();
  }
  printf("uthread: thread %d finished\n", id);
  current_thread->state = FREE;
  thread_schedule();
}

static void
worker0(void)
{
  worker(0);
}

static void
worker1(void)
{
  worker(1);
}

static void
worker2(void)
{
  worker(2);
}

int
main(void)
{
  thread_init();
  thread_create(worker0);
  thread_create(worker1);
  thread_create(worker2);
  thread_schedule();   // hand control to the worker threads
  printf("uthread: done\n");
  exit(0);
}


