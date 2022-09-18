
/* 	@(#)debug.h	4.2	(ULTRIX)	9/4/90 	*/

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * Debug macros.
 */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#ifdef __LANGUAGE_C

struct xprbuf {
	char *xp_msg;
	unsigned xp_arg1, xp_arg2, xp_arg3, xp_arg4;
	unsigned xp_timestamp;
	unsigned xp_pid, xp_tlbpid;
};

#ifdef KERNEL

#ifdef ASSERTIONS
#define ASSERT(EX) { if (EX) ; else assfail("EX", __FILE__, __LINE__) }
#else /* ASSERTIONS */
#define ASSERT(EX)
#endif /* ASSERTIONS */

extern struct xprbuf *xprbase, *xprptr;
extern int xprsize, xprinitflag;
extern unsigned xpr_flags;

#ifdef XPRBUG
#define XPRINTF(flags, format, arg1, arg2, arg3, arg4) \
	{ \
		if (xpr_flags & flags) \
			xprintf(format, arg1, arg2, arg3, arg4); \
	}
#else /* XPRBUG */
#define	XPRINTF(flags, format, arg1, arg2, arg3, arg4)
#endif /* XPRBUG */

#endif /* KERNEL */

#endif /* __LANGUAGE_C */

/*
 * flags
 */
#define XPR_CLOCK	0x00000001	/* Clock interrupt handler */
#define XPR_TLB		0x00000002	/* TLB miss handler */
#define XPR_INIT	0x00000004	/* routines called during init */
#define XPR_SCHED	0x00000008	/* Scheduler */
#define XPR_PROCESS	0x00000010	/* newproc/fork */
#define XPR_EXEC	0x00000020	/* Exec */
#define XPR_SYSCALL	0x00000040	/* System calls */
#define XPR_TRAP	0x00000080	/* Trap handler */
#define XPR_NOFAULT	0x00000100	/* Nofault bus error */
#define XPR_VM		0x00000200	/* VM */
#define XPR_SWAP	0x00000400	/* swapin/swapout */
#define XPR_SWTCH	0x00000800	/* swtch, setrq, remrq */
#define	XPR_DISK	0x00001000	/* disk i/o */
#define	XPR_TTY		0x00002000	/* mux i/o */
#define	XPR_TAPE	0x00004000	/* tape i/o */
#define	XPR_BIO		0x00008000	/* blk i/o */
#define	XPR_INTR	0x00010000	/* interrupt handling */
#define	XPR_RMAP	0x00020000	/* resource map handling */
#define	XPR_TEXT	0x00040000	/* shared text stuff */
#define	XPR_CACHE	0x00080000	/* cache handling */
#define	XPR_NFS		0x00100000	/* nfs */
#define	XPR_RPC		0x00200000	/* rpc */
#define	XPR_SIGNAL	0x00400000	/* signal handling */
#define	XPR_FPINTR	0x00800000	/* fp interrupt handling */
#define XPR_SM          0x01000000      /* Shared memory */

/*
 * options for mipskopt system call
 */
#define	KOPT_GET	1		/* get kernel option */
#define	KOPT_SET	2		/* set kernel option */
#define	KOPT_BIS	3		/* or in new option value */
#define	KOPT_BIC	4		/* clear indicated bits */

#ifdef __LANGUAGE_C

/*
 * The following is a table of symbolic names and addresses of kernel
 * variables which can be tuned to alter the performance of the system.
 * They can be modified at boot time as a boot parameter or by the mipskopt
 * system call.  Variables marked as readonly can't be modifed after system
 * boot time (i.e. through the mipskopt call).  "func" is called after the
 * variable is set in case there is processing beyond storing the new value.
 */
struct kernargs {
	char *name;
	int *ptr;
	int readonly;
	int (*func)();
};

/*
 * bit field descriptions for printf %r and %R formats
 */

/*
 * printf("%r %R", val, reg_descp);
 * struct reg_desc *reg_descp;
 *
 * the %r and %R formats allow formatted output of bit fields.
 * reg_descp points to an array of reg_desc structures, each element of the
 * array describes a range of bits within val.  the array should have a
 * final element with all structure elements 0.
 * %r outputs a string of the format "<bit field descriptions>"
 * %R outputs a string of the format "0x%x<bit field descriptions>"
 *
 * The fields in a reg_desc are:
 *	unsigned rd_mask;	An appropriate mask to isolate the bit field
 *				within a word, and'ed with val
 *
 *	int rd_shift;		A shift amount to be done to the isolated
 *				bit field.  done before printing the isolate
 *				bit field with rd_format and before searching
 *				for symbolic value names in rd_values
 *
 *	char *rd_name;		If non-null, a bit field name to label any
 *				out from rd_format or searching rd_values.
 *				if neither rd_format or rd_values is non-null
 *				rd_name is printed only if the isolated
 *				bit field is non-null.
 *
 *	char *rd_format;	If non-null, the shifted bit field value
 *				is printed using this format.
 *
 *	struct reg_values *rd_values;	If non-null, a pointer to a table
 *				matching numeric values with symbolic names.
 *				rd_values are searched and the symbolic
 *				value is printed if a match is found, if no
 *				match is found "???" is printed.
 *				
 */

/*
 * register values
 * map between numeric values and symbolic values
 */
struct reg_values {
	unsigned rv_value;
	char *rv_name;
};

/*
 * register descriptors are used for formatted prints of register values
 * rd_mask and rd_shift must be defined, other entries may be null
 */
struct reg_desc {
	unsigned rd_mask;	/* mask to extract field */
	int rd_shift;		/* shift for extracted value, - >>, + << */
	char *rd_name;		/* field name */
	char *rd_format;	/* format to print field */
	struct reg_values *rd_values;	/* symbolic names of values */
};

#ifdef KERNEL
extern struct reg_values pstat_values[];
extern struct reg_values sig_values[];
extern struct reg_values imask_values[];
extern struct reg_values exc_values[];
extern struct reg_values fileno_values[];
extern struct reg_values prot_values[];
extern struct reg_values syscall_values[];
extern struct reg_desc sr_desc[];
extern struct reg_desc exccode_desc[];
extern struct reg_desc cause_desc[];
extern struct reg_desc tlbhi_desc[];
extern struct reg_desc tlblo_desc[];
extern struct reg_desc tlbinx_desc[];
extern struct reg_desc tlbrand_desc[];
extern struct reg_desc tlbctxt_desc[];
extern struct reg_desc pte_desc[];
#endif /* KERNEL */
#endif /* __LANGUAGE_C */
