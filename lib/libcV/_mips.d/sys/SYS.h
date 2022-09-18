/*	@(#)SYS.h	4.1	ULTRIX	7/3/90	*/
/*
 * SYSCALLV -- System V system call sequence
 * The kernel expects arguments to be passed with the normal C calling
 * sequence.  v0 should contain the system call number.  On return from
 * the kernel mode, a3 will be 0 to indicate no error and non-zero to
 * indicate an error; if an error occurred v0 will contain an errno.
 */
#define	SYSCALLV(x)					\
LEAF(x);						\
	li	v0,SYS/**/x;				\
	syscall;					\
	beq	a3,zero,9f;				\
	j	_cerror;				\
9:
