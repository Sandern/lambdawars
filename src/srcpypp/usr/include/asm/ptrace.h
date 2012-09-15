#ifndef _ASM_X86_PTRACE_H
#define _ASM_X86_PTRACE_H

	/* For */
#include <asm/ptrace-abi.h>
#include <asm/processor-flags.h>


#ifndef __ASSEMBLY__

#ifdef __i386__
/* this struct defines the way the registers are stored on the
   stack during a system call. */


struct pt_regs {
	long ebx;
	long ecx;
	long edx;
	long esi;
	long edi;
	long ebp;
	long eax;
	int  xds;
	int  xes;
	int  xfs;
	/* int  gs; */
	long orig_eax;
	long eip;
	int  xcs;
	long eflags;
	long esp;
	int  xss;
};


#else /* __i386__ */


struct pt_regs {
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long rbp;
	unsigned long rbx;
/* arguments: non interrupts/non tracing syscalls only save upto here*/
	unsigned long r11;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long rax;
	unsigned long rcx;
	unsigned long rdx;
	unsigned long rsi;
	unsigned long rdi;
	unsigned long orig_rax;
/* end of arguments */
/* cpu exception frame or undefined */
	unsigned long rip;
	unsigned long cs;
	unsigned long eflags;
	unsigned long rsp;
	unsigned long ss;
/* top of stack page */
};

#endif /* !__i386__ */


#ifdef CONFIG_X86_PTRACE_BTS
/* a branch trace record entry
 *
 * In order to unify the interface between various processor versions,
 * we use the below data structure for all processors.
 */
enum bts_qualifier {
	BTS_INVALID = 0,
	BTS_BRANCH,
	BTS_TASK_ARRIVES,
	BTS_TASK_DEPARTS
};

struct bts_struct {
	__u64 qualifier;
	union {
		/* BTS_BRANCH */
		struct {
			__u64 from_ip;
			__u64 to_ip;
		} lbr;
		/* BTS_TASK_ARRIVES or
		   BTS_TASK_DEPARTS */
		__u64 jiffies;
	} variant;
};
#endif /* CONFIG_X86_PTRACE_BTS */


#endif /* !__ASSEMBLY__ */

#endif /* _ASM_X86_PTRACE_H */
