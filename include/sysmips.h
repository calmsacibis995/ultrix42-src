/*	@(#)sysmips.h	4.2	(ULTRIX)	9/4/90				      */
#include <ansi_compat.h>
#ifdef __mips
/*
 * Defines for MIPS specific system calls.  See ../sys/sysmips.c for more info.
 */

#define MIPS_VECTOR_SIZE 0x200
#define MIPS_VECTOR_DIVIDER 0x100

/* those that are implemented all or in part in the sysmips() routine */
#define MIPS_UNAME	0x000
#define MIPS_FPSIGINTR	0x001
#define MIPS_FPU	0x002
#define MIPS_FIXADE	0x003

/* those that are entirely implemented in a broken out procedure */
#define MIPS_KOPT	0x100
#define MIPS_HWCONF	0x101
#define MIPS_GETRUSAGE	0x102
#define MIPS_WAIT3	0x103
#define MIPS_CACHEFLUSH	0x104
#define MIPS_CACHECTL	0x105
#endif /* __mips */
