
#pragma once

#include <common_defs.h>
#include <process.h>
#include <arch/generic_x86/x86_utils.h>

#define KERNEL_INITIAL_STACK_ADDR 0xC000FFF0

// Forward-declaring regs
typedef struct regs regs;


/* This defines what the stack looks like after an ISR ran */
struct regs {
   u32 gs, fs, es, ds;      /* pushed the segs last */
   u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
   u32 int_num, err_code;    /* our 'push byte #' and error codes do this */
   u32 eip, cs, eflags, useresp, ss;   /* pushed by the CPU automatically */
};

/*
 * Saves the current state and calls schedule().
 * That after, typically after some time, the scheduler will restore the thread
 * as if kernel_yield() returned and nothing else happened.
 */

void kernel_yield();


static ALWAYS_INLINE void set_return_register(regs *r, u32 value)
{
   r->eax = value;
}


NORETURN void asm_context_switch_x86();
NORETURN void asm_kernel_context_switch_x86();

NORETURN static ALWAYS_INLINE void context_switch(regs *r)
{
   asm_context_switch_x86(
                          // Segment registers
                          r->gs,
                          r->fs,
                          r->es,
                          r->ds,

                          // General purpose registers
                          r->edi,
                          r->esi,
                          r->ebp,
                          /* skipping ESP */
                          r->ebx,
                          r->edx,
                          r->ecx,
                          r->eax,

                          // Registers pop-ed by iret
                          r->eip,
                          r->cs,
                          r->eflags,
                          r->useresp,
                          r->ss);
}



NORETURN static ALWAYS_INLINE void kernel_context_switch(regs *r)
{
   asm_kernel_context_switch_x86(
                                 r->eip,
                                 r->useresp,

                                 // Segment registers
                                 r->gs,
                                 r->fs,
                                 r->es,
                                 r->ds,

                                 // General purpose registers
                                 r->edi,
                                 r->esi,
                                 r->ebp,
                                 /* skipping ESP */
                                 r->ebx,
                                 r->edx,
                                 r->ecx,
                                 r->eax,

                                 // The eflags register
                                 r->eflags,

                                 // The useresp is repeated. See the assembly.
                                 r->useresp);
}

