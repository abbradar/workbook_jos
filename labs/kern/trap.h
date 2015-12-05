/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_TRAP_H
#define JOS_KERN_TRAP_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/trap.h>
#include <inc/mmu.h>

/* The kernel's interrupt descriptor table */
extern struct Gatedesc idt[];

void idt_init(void);
void print_regs(struct PushRegs *regs);
void print_trapframe(struct Trapframe *tf);
void page_fault_handler(struct Trapframe *);
void backtrace(struct Trapframe *);

/* Interrupt handlers */
void inthandle_divide();
void inthandle_debug();
void inthandle_nmi();
void inthandle_brkpt();
void inthandle_oflow();
void inthandle_bound();
void inthandle_illop();
void inthandle_device();
void inthandle_dblflt();

void inthandle_tss();
void inthandle_segnp();
void inthandle_stack();
void inthandle_gpflt();
void inthandle_pgflt();

void inthandle_fperr();
void inthandle_align();
void inthandle_mchk();
void inthandle_simderr();

void inthandle_syscall();

#endif /* JOS_KERN_TRAP_H */
