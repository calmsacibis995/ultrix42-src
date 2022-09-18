/*	@(#)cpudata.h	4.8	(ULTRIX)	1/10/91	*/
/************************************************************************
 *									*
 *			Copyright (c) 1986, 1988 by			*
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

/* ---------------------------------------------------------------------
 * Modification History 
 *
 * 8-Sep-90 paradis
 *	Added IPI_MCHK and IPIMSK_MCHK
 *
 * 4-Sep-90 dlh
 *	added pointer to  vpdata (per cpu vector processor information)
 *
 * 31-Aug-90 sekhar
 *	Changes to handle bus timeouts for memory mapped devices:
 *	Added cpu_wto_event, cpu_wto_pfn, cpu_consinfo, cpu_log_errinfo
 *	fields to cpu data structure.
 *
 * 29-Mar-90 jaw
 *	State flags added to reschedule psignal calls in hardclock
 *
 * 29-Mar-90 jaw/gmm
 *	changes to send SIGFPE for mips processes
 *
 * 12-Dec-89 jaw
 *	add icache flush support for ISIS.
 *
 * 09-Nov-89 jaw
 *	change to timeout support in smp.
 *
 * 19-Oct-89 gmm
 *	Include dir.h for mips since user.h needs it
 *
 * 13-Oct-89 gmm
 *	changes for MIPS smp support. New CURRENT_CPUDATA for mips.
 *
 * 20-Jul-89 jaw
 *	cpu_buf is now km_alloc'd.
 *
 * 12-Jun-89 bp
 *	Added kernel memory allocator TB sync flags.
 *
 * 09-Feb-89 -- jaw
 *	Move CPU_TBI state flag to seperate longword.  This was done
 *	because the state flag field can only be written by the cpu
 *	who owns it.
 *
 *  08-Feb-89	jaw
 *	fix race condition between timeout and untimeout 
 *
 *  02-Feb-89	jaw
 *	Make longjmp mpsafe.  move newpc and newfp global to per-cpudata base.
 *
 *  26-Jan-89	jaw
 *	fix up start/stop cpu.
 *
 * 12-Jan-89 - jaw
 *	hierpos now 0 for high priority.
 *
 * 19-Aug-88 -- miche
 *	Add field to indicate why a cpu is stopped:  set on behalf
 *	of self in hold_cpu and free_cpu; checked in the idle loop.
 *
 * 18-Jun-88 -- jaw  change to new cpu data format.
 *
 * 05-May-88 -- jmartin
 *	Added SMP debugging macro ASSERT_PRIMARY_CPU.
 *
 * 27-Jan-88 -- gmm
 *	Added the field for IP interrupts (cpu_int_req) in cpudata structure
 *	and the defines for different IPI types
 *
 * 11-DEC-86 -- jaw 	made MP variables externs so lint would work
 *	properly that include this file.
 *
 * 02-Apr-86 -- jrs
 *	Added bit defn for panicing
 *
 * 03-Mar-86 -- jrs
 *	Replaced unused c_panicstr element with c_cptime array to
 *	hold the per processor utilization summary.
 * 
 * 05-Feb-86 --jrs
 *	Added new c_paddr element for adb to latch onto
 *	Added translation buffer inval needed flag
 * 
 * 15 Jul 85 --jrs
 *	Created to hold per cpu related structures
 * ---------------------------------------------------------------------
 */


#ifndef CPUDATA_HDR
#define CPUDATA_HDR

#ifndef LOCORE

#ifndef	CPUSTATES
#ifdef	KERNEL
#include "../h/ansi_compat.h"
#include "../h/dk.h"
#include "../h/types.h"
#include "../machine/cpu.h"
#include "../machine/param.h"
#include "../h/param.h"
#ifdef __mips
#include "../h/dir.h"
#include "../h/user.h"
#endif /* __mips */
#else /*	KERNEL */
#include <ansi_compat.h>
#include <sys/dk.h>
#include <sys/smp_lock.h>
#include <machine/cpu.h>
#ifdef __mips
#include <sys/dir.h>
#include <sys/user.h>
#endif
#endif /*	KERNEL */
#endif /*	CPUSTATES */

#define MAXCPU 32
#define CPUDATA_BUFSIZE  512		/* size of print buffer for slaves. */

struct cpudata {
	/****************
	 Process state
	 ***************/
	struct pte *cpu_paddr;		/* physical address of pcb */
	int	cpu_runrun;		/* rescheduled flag */
	int	cpu_noproc;		/* no assigned process */
	struct  proc *cpu_proc;		/* pointer to assigned proc */
	short 	cpu_call_pending;	/* set if callout for this cpu */
	short	cpu_call_donot_requeue; /* set when an untimeout is executed
					    on another processor while the
					    timeout at the head of the queue
					    is being executed. */  
	struct callout *cpu_call_fwd;	/* Callout to do for this cpu */
	int	cpu_roundrobin;		/* takes place of round robin */
	caddr_t	cpu_newpc;		/* used in longjmp to save off pc*/
	caddr_t cpu_newfp;		/*   and fp for jump */

  	/**********************
	 cpu indentifiers/state
	***********************/
	unsigned cpu_stops;	/* reason for holding cpu in idle */
    	unsigned cpu_state;	/* state of this processor */
    	unsigned cpu_tbi_flag;	/* invalidate TB flag  */
	char	*cpu_stack;	/* pointer to stack after panic */
	char	*cpu_panicstr;	/* pointer to panic string of this processor */
	char	*cpu_istack;	/* pointer to Base of interrupt stack */
	char 	cpu_num;	/* what cpunumber is this cpu */
	unsigned cpu_mask;	/* 1<<Cpu_num, for affinity compares */
	unsigned cpu_int_req;	/* IP interrupt request mask */
	char	*cpu_machdep;	/* pointer to machine dependent info */

#ifdef __mips
	/* We don't want a pointer to the structure here, since the elements
	 * of cpu_archdep should be accessed as fast as possible. Currently
	 * every processor uses the same virtual address to get to its cpudata
	 * (accessed through u.u_pcb.pcb_cpuptr) */
	struct	cpu_archdep cpu_archdep;  /* cpu architecture dependent info */

#define ICACHE_PG_MAP_SZ (((MAXCACHE/NBPG)+(NBBY*NBPW)-1)/(NBBY*NBPW))
	unsigned	cpu_ic_dirt[ICACHE_PG_MAP_SZ];
#endif /* __mips */
	/******************** 
 	  per cpu statistics 
	*********************/
	unsigned  cpu_cptime[CPUSTATES]; /* usage time for each state */
	unsigned  cpu_ttyin;	/* tty characters inputted */
	unsigned  cpu_ttyout;   /* tty characters outputted */
	unsigned  cpu_switch;	/* context switches done on this cpu */
	unsigned  cpu_trap;	/* calls to trap */
	unsigned  cpu_syscall;	/* calls to trap */
	unsigned  cpu_intr;	/* number of interrupts */
	unsigned  cpu_ip_intr;	/* number of IP interrupts */
	
	/************************
	  Console printout queue
	*************************/
	int	cpu_bufin;		/* next free char */
	int 	cpu_bufout;		/* next char to print */
	char	*cpu_buf;		/* pinter to printf buffer */
	
	/***********************
	   Cpu lock information
	************************/
	struct  lock_t *cpu_hlock;	/* pointer to highest lock held */
	struct  lock_trace *cpu_l_ctrace; /*ptr to curr trace data*/
	struct  lock_trace cpu_l_trace[CPU_LK_TRACES];
	
	/*********************************
	   write timeout error information
	 *********************************/
	unsigned  cpu_wto_pfn;		/* pfn on which time out occured */
	int 	  cpu_wto_event;	/* flag: 0 => no timeout occured */
	char	  *cpu_consinfo;	/* pointer to error info for printing on console */
	char	  *cpu_log_errinfo;	/* pointer to error info to log to errlog buffer */

#ifdef __mips
	struct 	proc *cpu_fpe_event;	/* set to proc that needs SIGFPE.
					   used to reschedule FPINTR event
					   down to low ipl event */ 
	int 	cpu_fpe_sendsig;
	
#endif /* __mips */

#ifdef __vax
	/********************************
	   Vector processor information
	*********************************/
	struct	vpdata	*cpu_vpdata;	/* pointer to the VP information */
#endif /* vax */
};


#ifdef KERNEL
/* This macro will be non-zero if the current cpu is the BOOT cpu. */
#define BOOT_CPU \
	(CURRENT_CPUDATA->cpu_state & CPU_BOOT)

/* This macro gives a pointer to cpu data for cpu number "cpunum" */
#define CPUDATA(cpunum) \
	((cpudata[cpunum]))

#ifdef __vax  
/* this macro gives a pointer to the cpu data for the cpu running */
#define CURRENT_CPUDATA \
	((struct cpudata *)(getesp()))
#endif /* __vax */
#ifdef __mips
#define CURRENT_CPUDATA \
	(u.u_pcb.pcb_cpuptr)
#endif /* __mips   */

/*
 *  Boot cpu data
 */
int boot_cpu_num;	/* the slot number of the boot cpu */
int boot_cpu_mask;	/* affinity mask of the boot processor */
struct cpudata boot_cpudata; /* cpudata structure for the boot cpu */


/* array of cpudata structures */
struct cpudata *cpudata[MAXCPU]; 

int lowcpu;	/* low active cpu  num */
int highcpu;	/* high active cpu num */


/* this macro verifies we are on BOOT cpu */
#ifdef SMP_DEBUG
#define ASSERT_PRIMARY_CPU(FUNC_NAME) \
 if (!BOOT_CPU) panic("FUNC_NAME: not boot cpu")
#else /* SMP_DEBUG */
#define ASSERT_PRIMARY_CPU(FUNC_NAME)
#endif /* SMP_DEBUG */

#endif /* KERNEL */

#endif /* LOCORE */

/*
 * state flags
 */

#define	CPU_RUN		0x00000001	/* processor is up and running */
#define	CPU_PANIC	0x00000004	/* this cpu has paniced */
#define CPU_SOFT_DISABLE 0x00000008	/* disable soft errors */
#define CPU_BOOT	0x00000010	/* set if this is the boot cpu */
#define CPU_STOP	0x00000020	/* set if this cpu is stopped */
#define CPU_TIMEOUT	0x00000040	/* set if this cpu is executing a 
					    timeout. */
#define CPU_SIGPARENT	0x00000080	/* set if a signal to be sent to the
					   parent in swtch(). Only for MIPS */
#define CPU_IDLE	0x00000100	/* cpu is running idle proc */
#define CPU_SIGPROF     0x00000200      /* sig prof to send to cur proc */
#define CPU_SIGXCPU     0x00000400      /* sig xcpu to send to cur proc */
#define CPU_SIGVTALRM   0x00000800      /* sig vtalrm to send to cur proc */

/*
 * TBI flag move to own field because stat field can only be written
 * by the processor who owns the structure.
 */
#define	CPU_TBI		0x00000002	/* translation buffer needs inval */

/*
 *	IP interrupt mask bits (interrupt types) and the bit numbers
 *	When calling intrpt_cpu(), the second argument should be the
 *	bit number (eg. IPI_PRINT and NOT IPIMSK_PRINT)
 */


#define IPI_MAX		31	/* max. possible bit position */

#define	IPIMSK_PANIC	0x1	/* panic bit */
#define IPI_PANIC	0	/* panic bit number */

#define IPIMSK_PRINT	0x2	/* console print bit */
#define	IPI_PRINT	1	/* print bit number */

#define IPIMSK_SCHED	0x4	/* process schedule bit */
#define IPI_SCHED	2	/* process schedule bit number */

#define IPIMSK_TBFLUSH	0x8	/* TB flush bit */
#define IPI_TBFLUSH	3	/* TB flush bit number */

#define IPIMSK_KDB	0x10	/* kdb enter/leave bit */
#define IPI_KDB		4	/* kdb enter/leave bit number*/

#define IPIMSK_STOP	0x20	/* stop cpu at next switch */
#define IPI_STOP	5	/* stop cpu at next switch */

#define IPIMSK_START	0x40	/* start cpu that wast stopped */
#define IPI_START	6	/* start cpu that was stopped */

#define IPIMSK_CPUHOLD	0x80	/* hold cpu at next switch */
#define IPI_CPUHOLD	7	/* hold cpu at next switch */

#define	IPIMSK_KMEMTBFL	0x100	/* TB flush of kernel memory allocator */
#define	IPI_KMEMTBFL	8	/* TB flush of kernel memory allocator */

#define IPIMSK_ICACHE	0x200	/* flush ICACHE */
#define IPI_ICACHE	9	

#define IPIMSK_MCHK	0x400	/* Fatal machine check */
#define IPI_MCHK	10

#endif /*  CPUDATA_HDR */
