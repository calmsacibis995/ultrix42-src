/* SCCSID: "@(#)vectors.h	4.2	ULTRIX	9/10/90" */
/************************************************************************
 *									*
 *			Copyright (c) 1983,86,88 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/* ------------------------------------------------------------------------
 * Modification History: /sys/machine/vax/vectors.h
 *
 * 9/4/90 -- dlh
 *    original
 *
 *
 * ------------------------------------------------------------------------
 */

unsigned	vpmask;		/* 32 bit mask of cpus with VPs attached */
int		vptotal;	/* Total number of VPs in the system */
unsigned	vpfree;		/* 32 bit mask of free and operational VPs */
int		num_vec_procs;	/* num of vector processes currently running */

struct vpdata {
	struct proc *vpd_proc;	/* process owning the VP */
	u_int vpd_state;	/* state of the VP */
	int vpd_in_kernel;	/* indicates kernel using VP */
	int vpd_ccsw;		/* no. of cheap context switches ie.
				 * same vector process rescheduled
				 */
	int vpd_ecsw;		/* no. of expensive context switches
				 * ie. different process scheduled
				 */
	int vpd_success;	/* no. of times the process requesting
				 * this VP got it
				 */
	int vpd_failed;		/* no. of times the process requesting
				 * this VP did not get it
				 */
	};



/* States of the VP (vpd_state) */

#define VPD_ABSENT	0x00	/* VP is not present (initial value) */
#define VPD_ALIVE	0x01	/* VP is alive and operational */
#define VPD_DEAD	0x02	/* VP is dead and not usable */
#define VPD_ENABLED	0x04	/* VP is in enabled state */
#define VPD_DISABLED	0x08	/* VP is disabled */

/* value for vpdata.vpd_in_kernel */

#define	VPD_IN_KERNEL	1	/* set when kernel code may be moving */
				/* to/from a vector processor register */

struct vpcontext {
	int vpc_state;		/* state of the vector process */
	u_int vpc_error;	/* vector errors for this process */
	int vpc_affinity;	/* saved affinity of the process while 
				 * using the VP 
				 */
	int vpc_refuse;		/* no. of times the process has been 
				 * switched around without getting a VP 
				 */
	int vpc_cheap;		/* no. of times the process got 
				 * rescheduled without having to restore 
				 * its vector context 
				 */
	int vpc_expen;		/* no. of times the process's vector context 
				 * had to be restored 
				 */
	u_int vpc_vlr;		/* copy of VLR */
	u_int vpc_vmrlo;	/* copy of VMR - bits<0..31> */
	u_int vpc_vmrhi;	/* copy of VMR - bits<32..63> */
	u_int vpc_vcr;		/* copy of VCR */
	u_int vpc_vaer;		/* copy of VAER */
	char *vpc_vregs;	/* pointer to storage of 16 vector registers */
	};



/* size of KM_ALLOC request for vector registers */

#define VPREGSIZE	8192	/* 16 regs * 64 elements/reg * 8 bytes/elem */


/* States of the vector process (vpc_state) */

#define VPC_WAIT	1	/* new vector process - waiting for a VP    */
				/* to be allocated                          */
#define VPC_LOAD	2	/* vector context present in the vector     */
				/* processor and in the scalar processor;   */
				/* the the process's vector TB valid        */
#define VPC_SAVED	3	/* the process has a vector context in it   */
				/* struct vpcontext, but the process does   */
				/* not currently "own" a vector processor   */
#define VPC_LIMBO	4	/* this vector process "owns" a vector      */
				/* processor, but the process is not        */
				/* necessarily loaded into the scalar       */
				/* processor.  this implies that the        */
				/* process's vector TB is not valid         */

/* Vector errors (in vpc_error) */

#define VPC_ERROR_IMP	0x1	/* Implementation specific hardware error */
#define VPC_ERROR_IVO	0x2	/* Invalid opcode error                   */
#define VPC_ERROR_AEX	0x4	/* Arithmetic Exception                   */
#define VPC_ERROR_PMF	0x8	/* Pending Memory Fault                   */


/* return code from is_legal_vector_instruction() */

#define	is_vect_inst 0
#define	isnt_vect_inst 1


/* return values from vp_proc() */

#define	PROC_MAY_HAVE_VP	1
#define	PROC_MAY_NOT_HAVE_VP	0


/* macro to test for one VP in the system before calling vp_proc() */

#define	VP_PROC(p)	((vptotal == 1) ? PROC_MAY_HAVE_VP : vp_proc(p))


/* return value for vp_* routines */

#define	VP_FAILURE	1
#define	VP_SUCCESS	0


/* return value from vp_imp() */

#define	VP_IMP_FATAL		1
#define	VP_IMP_NOT_FATAL	0


/* input flags for vp_disabled_fault_handler() */
#define	VP_DIS_USER_MODE	1
#define	VP_DIS_KERN_MODE	2

/* bits within VPSR (Vector Processor Status Register) */

#define	VPSR_VEN	0x00000001	/* vector processor enabled */
#define	VPSR_RST	0x00000002	/* vector processor state reset */
#define	VPSR_STS	0x00000004	/* vector state store */
#define	VPSR_RLD	0x00000008	/* vector state reload */
#define	VPSR_MF		0x00000020	/* memory fault */
#define	VPSR_PMF	0x00000040	/* pending memory fault */
#define	VPSR_AEX	0x00000080	/* vector arithmetic exception */
#define	VPSR_IMP	0x01000000	/* implementation-specific hard err */
#define	VPSR_IVO	0x02000000	/* illegal vector opcode */
#define	VPSR_BSY	0x80000000	/* vector processor busy */


/* memory magic supplied by jim paradis */

#ifdef VAX9000

#if defined(VAX6400) || defined(VAX9000)
#define VPSYNC() if(cpu == VAX_9000 || cpu == VAX_6400) \
	{ mfpr(VMAC); mfpr(VMAC); }
#else
#define VPSYNC()
#endif /* VAX6400 || VAX9000 */

#else /* VAX9000 */

#if defined(VAX6400)
#define VPSYNC() if(cpu == VAX_6400) { mfpr(VMAC); mfpr(VMAC); }
#else
#define VPSYNC()
#endif /* VAX6400 || VAX9000 */

#endif /* VAX9000 */
