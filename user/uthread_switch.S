# user/uthread_switch.S — cooperative user-level context switch (hw3).
#
# void thread_switch(uint64 old_ctx, uint64 new_ctx);
#   a0 = &current_thread->context   (save here)
#   a1 = &next_thread->context      (load from here, then ret into it)
#
# YOUR JOB: save the callee-saved registers of the current thread into
# old_ctx, then load them from new_ctx and ret.
#
# Only callee-saved registers need saving because a thread yield is just
# an ordinary function call: caller-saved registers may already be
# clobbered.
#
# HINTS
#   - The context layout is given by struct thread_context in uthread.c:
#     ra at offset 0, sp at 8, then s0..s11 at 16, 24, ... (8 bytes each).
#   - Use sd/ld with a constant offset from a0 (save) and a1 (load).
#   - The last instruction must be ret.

    .section .text
    .globl thread_switch


thread_switch:
    # Save the current thread's context into old_ctx (a0).
    sd ra,   0(a0)
    sd sp,   8(a0)
    sd s0,  16(a0)
    sd s1,  24(a0)
    sd s2,  32(a0)
    sd s3,  40(a0)
    sd s4,  48(a0)
    sd s5,  56(a0)
    sd s6,  64(a0)
    sd s7,  72(a0)
    sd s8,  80(a0)
    sd s9,  88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)

    # Restore the next thread's context from new_ctx (a1).
    ld ra,   0(a1)
    ld sp,   8(a1)
    ld s0,  16(a1)
    ld s1,  24(a1)
    ld s2,  32(a1)
    ld s3,  40(a1)
    ld s4,  48(a1)
    ld s5,  56(a1)
    ld s6,  64(a1)
    ld s7,  72(a1)
    ld s8,  80(a1)
    ld s9,  88(a1)
    ld s10, 96(a1)
    ld s11, 104(a1)

    # Continue execution using the restored return address.
    ret
