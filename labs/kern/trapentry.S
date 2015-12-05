/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* The TRAPHANDLER macro defines a globally-visible function for handling
 * a trap.  It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 */ 
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

// See http://www.read.seas.harvard.edu/~kohler/class/aosref/i386/s09_10.htm
TRAPHANDLER_NOEC(inthandle_divide, 0)
TRAPHANDLER_NOEC(inthandle_debug, 1)
TRAPHANDLER_NOEC(inthandle_nmi, 2)
TRAPHANDLER_NOEC(inthandle_brkpt, 3)
TRAPHANDLER_NOEC(inthandle_oflow, 4)
TRAPHANDLER_NOEC(inthandle_bound, 5)
TRAPHANDLER_NOEC(inthandle_illop, 6)
TRAPHANDLER_NOEC(inthandle_device, 7)
TRAPHANDLER(inthandle_dblflt, 8)

TRAPHANDLER(inthandle_tss, 10)
TRAPHANDLER(inthandle_segnp, 11)
TRAPHANDLER(inthandle_stack, 12)
TRAPHANDLER(inthandle_gpflt, 13)
TRAPHANDLER(inthandle_pgflt, 14)

TRAPHANDLER_NOEC(inthandle_fperr, 16)
TRAPHANDLER_NOEC(inthandle_align, 17)
TRAPHANDLER_NOEC(inthandle_mchk, 18)
TRAPHANDLER_NOEC(inthandle_simderr, 19)

TRAPHANDLER_NOEC(inthandle_syscall, 48)

_alltraps:
  pushl %ds
  pushl %es
  pusha
  mov $GD_KD, %ax
  mov %ax, %ds
  mov %ax, %es
  pushl %esp
  call trap
