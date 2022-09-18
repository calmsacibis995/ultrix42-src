/*
 * @(#)cpu.h	4.3	(ULTRIX)	9/4/90
 */
/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

/*
 * Modification History: cpu.h - cpu specific defines
 *
 * 09-Aug-90	Randall Brown
 *	Added SPLIO define.
 *
 * 29-Mar-90	gmm
 *	Changed spl6 and splhigh to be same as splclock (6)
 *
 * 30-Dec-89	bp
 *	Added field to cpu_archdep structure for determining whether a cpu
 *	is in an interrupt service routine.
 *
 * 28-Dec-89	Robin
 *	Added defines for R2000 cause register bits.
 *
 * 13-Oct-89    gmm
 *	SMP changes - added cpu_archdep and tlb_pid_state structures.
 *
 * 21-sep-89	burns
 *	ISIS pool merge for the following:
 *
 * 	13-May-89 -- kong
 *	changed NF_NENTRIES from 11 to 12, added new #define NF_INTR.
 *	See locore.s for details.
 *
 * 08-Jun-89 -- gmm
 *	Added define for IDLSTAK_OFF
 *
 * 02-May-89 -- jaw, jmartin
 *	fix forkutl to work on mips.
 *
 * 26-Apr-89 -- jmartin
 *	Add IS_Forkmap predicate.
 *
 * 07-Apr-89 -- afd
 *	Fixed CONS/PROM/MEM-PRINT flag values.
 *	Added IPLSIZE for the iplmask array (its the number of interrupt lines)
 *
 * 06-Feb-89 -- kong
 *	Added size of interrupt masks to include those for spl6, splhigh,
 *	spl7, and splextreme.
 *
 * 26-Jan-89 -- kong
 *	Added interrupt masks offsets.
 *
 * 06-Sep-88 -- afd
 *	Added SBE_ADDR, SEG_BITS, SPLblah defines.
 *
 */

#ifndef CPU_HDR
#define CPU_HDR

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

/*
 * Segment base addresses and sizes
 */
#define	K0BASE		0x80000000
#define	K0SIZE		0x20000000
#define	K1BASE		0xA0000000
#define	K1SIZE		0x20000000
#define	K2BASE		0xC0000000
#define	K2SIZE		0x20000000
/*
 * KPTE window is 2 megabytes long and is placed
 * 4 Meg from the top of kseg2, to leave space for upage). 
 * KPTE windown must be on a 2 Meg boundary.
 * (Note: this definition relies upon 2's complement arithmetic!)
 */
#define	KPTEBASE	(-0x400000)
#define	KPTESIZE	0x200000
#define	KUBASE		0
#define	KUSIZE		0x80000000

/*
 * Exception vectors
 */
#define	UT_VEC		K0BASE			/* utlbmiss vector */
#define	E_VEC		(K0BASE+0x80)		/* exception vector */
#define	R_VEC		(K1BASE+0x1fc00000)	/* reset vector */

/*
 * Address conversion macros
 */
#define	K0_TO_K1(x)	((unsigned)(x)|0xA0000000)	/* kseg0 to kseg1 */
#define	K1_TO_K0(x)	((unsigned)(x)&0x9FFFFFFF)	/* kseg1 to kseg0 */
#define	K0_TO_PHYS(x)	((unsigned)(x)&0x1FFFFFFF)	/* kseg0 to physical */
#define	K1_TO_PHYS(x)	((unsigned)(x)&0x1FFFFFFF)	/* kseg1 to physical */
#define	PHYS_TO_K0(x)	((unsigned)(x)|0x80000000)	/* physical to kseg0 */
#define	PHYS_TO_K1(x)	((unsigned)(x)|0xA0000000)	/* physical to kseg1 */

/*
 * Address predicates
 */
#define	IS_KSEG0(x)	((unsigned)(x) >= K0BASE && (unsigned)(x) < K1BASE)
#define	IS_KSEG1(x)	((unsigned)(x) >= K1BASE && (unsigned)(x) < K2BASE)
#define	IS_KSEG2(x)	((unsigned)(x) >= K2BASE && (unsigned)(x) < KPTEBASE)
#define	IS_KPTESEG(x)	((unsigned)(x) >= KPTEBASE)
#define	IS_KUSEG(x)	((unsigned)(x) < K0BASE)
#define IS_Forkmap(x)	((unsigned)(x) >= (unsigned)Forkmap && \
			 (unsigned)(x) < (unsigned)(Forkmap + FORKPAGES))

/*
 * Cache size constants
 */
#define	MINCACHE	(4*1024)
#define	MAXCACHE	(64*1024)

/*
 * TLB size constants
 */
#define	TLBWIREDBASE	0		/* WAG for now */
#define	NWIREDENTRIES	8		/* WAG for now */
#define	TLBRANDOMBASE	NWIREDENTRIES
#define	NRANDOMENTRIES	(NTLBENTRIES-NWIREDENTRIES)
#define	NTLBENTRIES	64		/* WAG for now */

/*
 * tlb entrylo format
 */
#ifndef LOCORE
union tlb_lo {
	unsigned tl_word;		/* efficient access */
	struct {
#ifdef __MIPSEB
		unsigned tls_pfn:20;	/* physical page frame number */
		unsigned tls_n:1;	/* non-cacheable */
		unsigned tls_d:1;	/* dirty (actually writeable) */
		unsigned tls_v:1;	/* valid */
		unsigned tls_g:1;	/* match any pid */
		unsigned :8;
#endif /* __MIPSEB */
#ifdef __MIPSEL
		unsigned :8;
		unsigned tls_g:1;	/* match any pid */
		unsigned tls_v:1;	/* valid */
		unsigned tls_d:1;	/* dirty (actually writeable) */
		unsigned tls_n:1;	/* non-cacheable */
		unsigned tls_pfn:20;	/* physical page frame number */
#endif /* __MIPSEL */
	} tl_struct;
};

#define	tl_pfn		tl_struct.tls_pfn
#define	tl_n		tl_struct.tls_n
#define	tl_d		tl_struct.tls_d
#define	tl_v		tl_struct.tls_v
#define	tl_g		tl_struct.tls_g
#endif /* !LOCORE */

#define	TLBLO_PFNMASK	0xfffff000
#define	TLBLO_PFNSHIFT	12
#define	TLBLO_N		0x800		/* non-cacheable */
#define	TLBLO_D		0x400		/* writeable */
#define	TLBLO_V		0x200		/* valid bit */
#define	TLBLO_G		0x100		/* global access bit */

#define	TLBLO_FMT	"\20\14N\13D\12V\11G"

/*
 * TLB entryhi format
 */
#ifndef LOCORE
union tlb_hi {
	unsigned th_word;		/* efficient access */
	struct {
#ifdef __MIPSEB
		unsigned ths_vpn:20;	/* virtual page number */
		unsigned ths_pid:6;
		unsigned :6;
#endif /* __MIPSEB */
#ifdef __MIPSEL
		unsigned :6;
		unsigned ths_pid:6;
		unsigned ths_vpn:20;	/* virtual page number */
#endif /* __MIPSEL */
	} th_struct;
};

#define	th_vpn		th_struct.ths_vpn
#define	th_pid		th_struct.ths_pid
#endif /* !LOCORE */

#define	TLBHI_VPNMASK	0xfffff000
#define	TLBHI_VPNSHIFT	12
#define	TLBHI_PIDMASK	0xfc0
#define	TLBHI_PIDSHIFT	6
#define	TLBHI_NPID	64

/*
 * TLB index register
 */
#ifndef LOCORE
union tlb_inx {
	unsigned ti_word;
	struct {
#ifdef __MIPSEB
		unsigned tis_probe:1;	/* 1 => probe failure */
		unsigned :17;
		unsigned tis_inx:6;	/* tlb index for TLBWRITEI op */
		unsigned :8;
#endif /* __MIPSEB */
#ifdef __MIPSEL
		unsigned :8;
		unsigned tis_inx:6;	/* tlb index for TLBWRITEI op */
		unsigned :17;
		unsigned tis_probe:1;	/* 1 => probe failure */
#endif /* __MIPSEL */
	} ti_struct;
};

#define	ti_probe	ti_struct.tis_probe
#define	ti_inx		ti_struct.tis_inx
#endif /* !LOCORE */

#define	TLBINX_PROBE		0x80000000
#define	TLBINX_INXMASK		0x00003f00
#define	TLBINX_INXSHIFT		8

/*
 * TLB random register
 */
#ifndef LOCORE
union tlb_rand {
	unsigned tr_word;
	struct {
#ifdef __MIPSEB
		unsigned :18;
		unsigned trs_rand:6;	/* tlb index for TLBWRITER op */
		unsigned :8;
#endif /* __MIPSEB */
#ifdef __MIPSEL
		unsigned :8;
		unsigned trs_rand:6;	/* tlb index for TLBWRITER op */
		unsigned :18;
#endif /* __MIPSEL */
	} tr_struct;
};

#define	tr_rand		ti_struct.tis_rand
#endif /* !LOCORE */

#define	TLBRAND_RANDMASK	0x00003f00
#define	TLBRAND_RANDSHIFT	8

/*
 * TLB context register
 */
#ifndef LOCORE
union tlb_ctxt {
	unsigned tc_word;		/* efficient access */
	struct {
#ifdef __MIPSEB
		unsigned tcs_pteseg:11;	/* bits 21-31 of kernel pte window */
		unsigned tcs_vpn:19;	/* vpn of faulting ref (ro) */
		unsigned :2;
#endif /* __MIPSEB */
#ifdef __MIPSEL
		unsigned :2;
		unsigned tcs_vpn:19;	/* vpn of faulting ref (ro) */
		unsigned tcs_pteseg:11;	/* bits 22-31 of kernel pte window */
#endif /* __MIPSEL */
	} tc_struct;
};

#define	tc_pteseg	tc_struct.tcs_pteseg
#define	tc_vpn		tc_struct.tcs_vpn
#endif /* !LOCORE */

#define	TLBCTXT_BASEMASK	0xffe00000
#define	TLBCTXT_BASESHIFT	21

#define	TLBCTXT_VPNMASK		0x001ffffc
#define	TLBCTXT_VPNSHIFT	2

#if defined(__mips) && defined(__LANGUAGE_C)
struct tlbinfo {
	union tlb_lo	lo;
	union tlb_hi	hi;
};
#endif /* __mips && __LANGUAGE_C */
/*
 * NPAGEMAP is the number of second level page-mappings that can be cached
 * in the tlb wired entries. The u-area and kernel stack take the other 2.
 * This should be defined as (NWIREDENTRIES-UPAGES) but that would cause
 * this file to depend on vmparam.h
 */
#define NPAGEMAP 6

/*
 * PMAX control and status register
 *
 */

#define PM_CSR_ADDR	((volatile short *)PHYS_TO_K1(0x1e000000))
#define PM_CSR_MNFMOD	0x8000
#define PM_CSR_STATUS	0x4000
/* empty */
#define PM_CSR_CRSRTST	0x1000
#define PM_CSR_MONO	0x0800
#define PM_CSR_MEMERR	0x0400
#define PM_CSR_VINT	0x0200
#define PM_CSR_TXDIS	0x0100
/* empty */
#define PM_CSR_VBGTRG	0x0004
#define PM_CSR_VRGTRG	0x0002
#define PM_CSR_VRGTRB	0x0001

/*
 * Status register
 */
#define	SR_CUMASK	0xf0000000	/* coproc usable bits */

#define	SR_CU3		0x80000000	/* Coprocessor 3 usable */
#define	SR_CU2		0x40000000	/* Coprocessor 2 usable */
#define	SR_CU1		0x20000000	/* Coprocessor 1 usable */
#define	SR_CU0		0x10000000	/* Coprocessor 0 usable */

#define	SR_BEV		0x00400000	/* use boot exception vectors */

/* Cache control bits */
#define	SR_TS		0x00200000	/* TLB shutdown */
#define	SR_PE		0x00100000	/* cache parity error */
#define	SR_CM		0x00080000	/* cache miss */
#define	SR_PZ		0x00040000	/* cache parity zero */
#define	SR_SWC		0x00020000	/* swap cache */
#define	SR_ISC		0x00010000	/* Isolate data cache */

/*
 * Interrupt enable bits
 * (NOTE: bits set to 1 enable the corresponding level interrupt)
 */
#define	SR_IMASK	0x0000ff00	/* Interrupt mask */
#define	SR_IMASK8	0x00000000	/* mask level 8 (FPU) */
#define	SR_IMASK7	0x00008000	/* mask level 7 (MEM) */
#define	SR_IMASK6	0x0000c000	/* mask level 6 (CLOCK) */
#define	SR_IMASK5	0x0000e000	/* mask level 5 (TTY) */
#define	SR_IMASK4	0x0000f000	/* mask level 4 (IMP) */
#define	SR_IMASK3	0x0000f800	/* mask level 3 (BIO) */
#define	SR_IMASK2	0x0000fc00	/* mask level 2 (software - NET) */
#define	SR_IMASK1	0x0000fe00	/* mask level 1 (software - SOFTCLOCK)*/
#define	SR_IMASK0	0x0000ff00	/* mask level 0 (NONE) */

#define	SR_IBIT8	0x00008000	/* bit level 8 */
#define	SR_IBIT7	0x00004000	/* bit level 7 */
#define	SR_IBIT6	0x00002000	/* bit level 6 */
#define	SR_IBIT5	0x00001000	/* bit level 5 */
#define	SR_IBIT4	0x00000800	/* bit level 4 */
#define	SR_IBIT3	0x00000400	/* bit level 3 */
#define	SR_IBIT2	0x00000200	/* bit level 2 */
#define	SR_IBIT1	0x00000100	/* bit level 1 */

#define	IPLSIZE		8		/* number of interrupt lines */

/*
 * Interrupt levels (for software use). These defines are used as indexes
 * into the splm array to obtain proper interrupt mask for the system.
 * The masks vary greatly on the various mips platforms. These defines
 * are also used by whatspl (in trap.c) to give us a platform independent
 * way of checking ipl levels.
 */
#define SPLFPU		8		/* block everything */
#define SPLEXTREME 	8		/* block everything */
#define SPL7		8		/* block everything */
#define SPLMEM		7		/* block hard error intrrupts */
#define SPL6		6		/* block clock */
#define SPLHIGH		6		/* block clock */
#define SPLCLOCK 	6		/* block clock */
#define SPLIO		5		/* block I/O devices */
#define SPLDEVHIGH 	5		/* block I/O devices */
#define SPLTTY		5		/* block I/O devices */
#define SPLCONS		5		/* block I/O devices */
#define SPLIMP		5		/* block I/O devices */
#define SPLBIO		5		/* block I/O devices */
                                        /* Note: 4 is not used */
                                        /* Note: 3 is not used */
#define SPLNET		2		/* block softnet interrupts */
#define SPLSOFTC 	1		/* block softclock interrupts */
#define SPLNONE		0		/* no interrupts blocked */
#define	SPLMSIZE	(9)

#define	SR_KUO		0x00000020	/* old kernel/user, 0 => k, 1 => u */
#define	SR_IEO		0x00000010	/* old interrupt enable, 1 => enable */
#define	SR_KUP		0x00000008	/* prev kernel/user, 0 => k, 1 => u */
#define	SR_IEP		0x00000004	/* prev interrupt enable, 1 => enable */
#define	SR_KUC		0x00000002	/* cur kernel/user, 0 => k, 1 => u */
#define	SR_IEC		0x00000001	/* cur interrupt enable, 1 => enable */

#define	SR_IMASKSHIFT	8

#define	SR_FMT		"\20\40BD\26TS\25PE\24CM\23PZ\22SwC\21IsC\20IM7\17IM6\16IM5\15IM4\14IM3\13IM2\12IM1\11IM0\6KUo\5IEo\4KUp\3IEp\2KUc\1IEc"

/*
 * Cause Register
 */
#define	CAUSE_BD	0x80000000	/* Branch delay slot */
#define	CAUSE_CEMASK	0x30000000	/* coprocessor error */
#define	CAUSE_CESHIFT	28

/* Interrupt pending bits */
#define	CAUSE_IP8	0x00008000	/* External level 8 pending */
#define	CAUSE_IP7	0x00004000	/* External level 7 pending */
#define	CAUSE_IP6	0x00002000	/* External level 6 pending */
#define	CAUSE_IP5	0x00001000	/* External level 5 pending */
#define	CAUSE_IP4	0x00000800	/* External level 4 pending */
#define	CAUSE_IP3	0x00000400	/* External level 3 pending */
#define	CAUSE_SW2	0x00000200	/* Software level 2 pending */
#define	CAUSE_SW1	0x00000100	/* Software level 1 pending */

#define	CAUSE_IPMASK	0x0000FF00	/* Pending interrupt mask */
#define	CAUSE_IPSHIFT	8

#define	CAUSE_EXCMASK	0x0000003C	/* Cause code bits */
#define	CAUSE_EXCSHIFT	2

#define	CAUSE_FMT	"\20\40BD\36CE1\35CE0\20IP8\17IP7\16IP6\15IP5\14IP4\13IP3\12SW2\11SW1\1INT"

/* Cause register exception codes */

#define	EXC_CODE(x)	((x)<<2)

/* Hardware exception codes */
#define	EXC_INT		EXC_CODE(0)	/* interrupt */
#define	EXC_MOD		EXC_CODE(1)	/* TLB mod */
#define	EXC_RMISS	EXC_CODE(2)	/* Read TLB Miss */
#define	EXC_WMISS	EXC_CODE(3)	/* Write TLB Miss */
#define	EXC_RADE	EXC_CODE(4)	/* Read Address Error */
#define	EXC_WADE	EXC_CODE(5)	/* Write Address Error */
#define	EXC_IBE		EXC_CODE(6)	/* Instruction Bus Error */
#define	EXC_DBE		EXC_CODE(7)	/* Data Bus Error */
#define	EXC_SYSCALL	EXC_CODE(8)	/* SYSCALL */
#define	EXC_BREAK	EXC_CODE(9)	/* BREAKpoint */
#define	EXC_II		EXC_CODE(10)	/* Illegal Instruction */
#define	EXC_CPU		EXC_CODE(11)	/* CoProcessor Unusable */
#define	EXC_OV		EXC_CODE(12)	/* OVerflow */

/* software exception codes */
#define	SEXC_SEGV	EXC_CODE(16)	/* Software detected seg viol */
#define	SEXC_RESCHED	EXC_CODE(17)	/* resched request */
#define	SEXC_PAGEIN	EXC_CODE(18)	/* page-in request */
#define	SEXC_CPU	EXC_CODE(19)	/* coprocessor unusable */


/*
 * Coprocessor 0 registers
 */
#define	C0_INX		$0		/* tlb index */
#define	C0_RAND		$1		/* tlb random */
#define	C0_TLBLO	$2		/* tlb entry low */

#define	C0_CTXT		$4		/* tlb context */

#define	C0_BADVADDR	$8		/* bad virtual address */

#define	C0_TLBHI	$10		/* tlb entry hi */

#define	C0_SR		$12		/* status register */
#define	C0_CAUSE	$13		/* exception cause */
#define	C0_EPC		$14		/* exception pc */
#define	C0_PRID		$15		/* revision identifier */

/* Cause register values used to decode what took place
 */
#define R2000_INT 0
#define R2000_MOD 1
#define R2000_TLBL 2
#define R2000_TLBS 3
#define R2000_ADEL 4
#define R2000_ADES 5
#define R2000_IBE 6
#define R2000_DBE 7
#define R2000_SYS 8
#define R2000_BP 9
#define R2000_RI 10
#define R2000_CPU 11
#define R2000_OV 12
/*
 * Coprocessor 0 operations
 */
#define	C0_READI  0x1		/* read ITLB entry addressed by C0_INDEX */
#define	C0_WRITEI 0x2		/* write ITLB entry addressed by C0_INDEX */
#define	C0_WRITER 0x6		/* write ITLB entry addressed by C0_RAND */
#define	C0_PROBE  0x8		/* probe for ITLB entry addressed by TLBHI */
#define	C0_RFE	  0x10		/* restore for exception */

/*
 * Flags for the nofault handler. 0 means no fault is expected.
 */
#define	NF_BADADDR	1	/* badaddr, wbadaddr */
#define	NF_COPYIO	2	/* copyin, copyout */
#define	NF_ADDUPC	3	/* addupc */
#define	NF_FSUMEM	4	/* fubyte, subyte, fuword, suword */
#define	NF_USERACC	5	/* useracc */
#define	NF_SOFTFP	6	/* softfp */
#define	NF_REVID	7	/* revision ids */
#define	NF_COPYSTR	8	/* copyinstr, copyoutstr */
#define	NF_SOFTFPI	9	/* fp instr fetch */
#define	NF_FIXADE	10	/* fix address errors */
#define NF_INTR		11	/* faults in interrupt handler */
#define	NF_NENTRIES	12

#define SBE_ADDR 0xB7000000	/* loc which latches physical addr of bus err */
#define SEG_BITS 0xe0000000 	/* bits in virt addr that indicate segment */
#define VA_BYTEOFFS 0x00000fff	/* bits for byte within page */
#define CPEINTVL (60*15)	/* timeout value to log CPEs (15 minutes) */
#define PROMPRINT 0x1		/* only print available is thru PROM */
#define CONSPRINT 0x2		/* print thru console device driver available */
#define MEMPRINT  0x4		/* print to errlog available */
#define PANICPRINT 0x8		/* panic in progress, print to screen */

/*
 * Chip interrupt vector
 */
#define	NC0VECS		8
#ifndef LOCORE
#ifdef KERNEL
extern int (*c0vec_tbl[])();
int	cpu_avail;		/* Number of processors in the system */
#endif
#endif /* !LOCORE */

#define KSTACKBITS	0x80000000 /* bits indicating processor is using 
	      kernel stack and not user stack. This is used in exception() 
	      in locore.s to decide to swtich stacks on exception. This sort
	      of replaces kstackflag, received from MIPSCo. This change needed
	      for SMP. Valid for R2000/3000 chips where MSB for user stack is
	      clear. */

#ifndef LOCORE
static struct tlb_pid_state {
	struct proc	*tps_owner;	/* owner of the tlbpid */
	int		tps_procpid;	/* if this tlbpid is in use will be
					 the pid of the process, else -1*/
};
struct	cpu_archdep {
	struct proc 	*cp_fpowner;	/* owner of FP unit */
        int		cp_nofault;	/* nofault flag */
	int		cp_nofault_cause;
	int		cp_nofault_badvaddr;
	struct  proc	*cp_exitproc; /* if the process is exiting, its proc */
	struct	proc    *cp_idleproc; /* address of the idle proc for this cpu */
	struct	tlb_pid_state cp_tps[TLBHI_NPID]; /* list of tlbpid
					 owners in this processor */
	int		cp_next_tlbpid;  /* next free tlb pid */
	int		cp_tlbcount; /* no. of times the cpu forced to get
					    a new tlbpid during process
					    migration */
	int		cp_inisr;	/* IN an ISR */
}; 
#endif /* !LOCORE  */

#define cpu_fpowner	cpu_archdep.cp_fpowner
#define cpu_nofault	cpu_archdep.cp_nofault
#define cpu_nofault_cause	cpu_archdep.cp_nofault_cause
#define cpu_nofault_badvaddr	cpu_archdep.cp_nofault_badvaddr
#define cpu_exitproc	cpu_archdep.cp_exitproc
#define cpu_idleproc	cpu_archdep.cp_idleproc
#define cpu_tps		cpu_archdep.cp_tps
#define cpu_next_tlbpid	cpu_archdep.cp_next_tlbpid
#define cpu_tlbcount	cpu_archdep.cp_tlbcount
#define	cpu_inisr	cpu_archdep.cp_inisr
#endif /* CPU_HDR */
