/*	@(#)vmmeter.h	4.2	(ULTRIX)	9/4/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1988 by			*
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
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*	vmmeter.h	6.1	7/29/83	*/
/*
 *
 *   Modification History:
 * 09-Nov-89 jaw
 *	remove fields moved to the per-cpu data structure.
 *
 * 25 Jul 88 -- jmartin
 *	Added the meter v_mprace, which counts the number of page faults
 *	resolved by some other thread of execution than the faulting
 *	thread.
 */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

/*
 * Virtual memory related instrumentation
 */
struct vmmeter
{

#define	v_first	v_pdma
	unsigned v_pdma;	/* pseudo-dma interrupts */
#ifdef __mips
	unsigned v_tlbpid;	/* calls to new_tlbpid */
#endif /* __mips */
#ifdef __mips
	unsigned v_soft;	/* software interrupts */
#endif /* __mips */
	unsigned v_pswpin;	/* pages swapped in */
	unsigned v_pswpout;	/* pages swapped out */
	unsigned v_pgin;	/* pageins */
	unsigned v_pgout;	/* pageouts */
	unsigned v_pgpgin;	/* pages paged in */
	unsigned v_pgpgout;	/* pages paged out */
	unsigned v_intrans;	/* intransit blocking page faults */
	unsigned v_pgrec;	/* total page reclaims */
	unsigned v_xsfrec;	/* found in free list rather than on swapdev */
	unsigned v_xifrec;	/* found in free list rather than in filsys */
	unsigned v_exfod;	/* pages filled on demand from executables */
	unsigned v_zfod;	/* pages zero filled on demand */
	unsigned v_vrfod;	/* fills of pages mapped by vread() */
	unsigned v_nexfod;	/* number of exfod's created */
	unsigned v_nzfod;	/* number of zfod's created */
	unsigned v_nvrfod;	/* number of vrfod's created */
	unsigned v_pgfrec;	/* page reclaims from free list */
	unsigned v_faults;	/* total faults taken */
	unsigned v_scan;	/* scans in page out daemon */
	unsigned v_rev;		/* revolutions of the hand */
	unsigned v_seqfree;	/* pages taken from sequential programs */
	unsigned v_dfree;	/* pages freed by daemon */
	unsigned v_fastpgrec;	/* fast reclaims in locore */
#define	v_last v_fastpgrec
	unsigned v_swpin;	/* swapins */
	unsigned v_swpout;	/* swapouts */
	unsigned v_mprace;	/* pages validated on another processor */
#ifdef __mips
#ifdef USE_IDLE
	unsigned v_zeros;	/* pages zero'ed in idle loop */
	unsigned v_zero_hits;	/* clearseg's saved by idle clearing */
#endif /* USE_IDLE */
#endif /* __mips */
};
#ifdef KERNEL
struct	vmmeter cnt, rate, sum;
#endif /* KERNEL */

/* systemwide totals computed every five seconds */
struct vmtotal
{
	short	t_rq;		/* length of the run queue */
	short	t_dw;		/* jobs in ``disk wait'' (neg priority) */
	short	t_pw;		/* jobs in page wait */
	short	t_sl;		/* jobs sleeping in core */
	short	t_sw;		/* swapped out runnable/short block jobs */
#ifndef __mips
	int	t_vm;		/* total virtual memory */
	int	t_avm;		/* active virtual memory */
	int	t_rm;		/* total real memory in use */
	int	t_arm;		/* active real memory */
	int	t_vmtxt;	/* virtual memory used by text */
	int	t_avmtxt;	/* active virtual memory used by text */
	int	t_rmtxt;	/* real memory used by text */
	int	t_armtxt;	/* active real memory used by text */
	int	t_free;		/* free memory pages */
#else /* mips */
	long	t_vm;		/* total virtual memory */
	long	t_avm;		/* active virtual memory */
	long	t_rm;		/* total real memory in use */
	long	t_arm;		/* active real memory */
	long	t_vmtxt;	/* virtual memory used by text */
	long	t_avmtxt;	/* active virtual memory used by text */
	long	t_rmtxt;	/* real memory used by text */
	long	t_armtxt;	/* active real memory used by text */
	long	t_free;		/* free memory pages */
#endif /* __mips */
};
#ifdef KERNEL
struct	vmtotal total;
#endif /* KERNEL */

/*
 * Optional instrumentation.
 */
#ifdef PGINPROF

#define	NDMON	128
#define	NSMON	128

#define	DRES	20
#define	SRES	5

#define	PMONMIN	20
#define	PRES	50
#define	NPMON	64

#define	RMONMIN	130
#define	RRES	5
#define	NRMON	64

/* data and stack size distribution counters */
unsigned int	dmon[NDMON+1];
unsigned int	smon[NSMON+1];

/* page in time distribution counters */
unsigned int	pmon[NPMON+2];

/* reclaim time distribution counters */
unsigned int	rmon[NRMON+2];

int	pmonmin;
int	pres;
int	rmonmin;
int	rres;

unsigned rectime;		/* accumulator for reclaim times */
unsigned pgintime;		/* accumulator for page in times */
#endif /* PGINPROF */
