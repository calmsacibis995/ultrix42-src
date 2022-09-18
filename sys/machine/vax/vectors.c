#ifndef lint
static char *sccsid = "@(#)vectors.c	4.6	ULTRIX	4/12/91";
#endif lint
/************************************************************************
 *									*
 *		  Copyright (c) 1983,1986,1988,1990 by			*
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
 * Modification History: /sys/machine/vax/vectors.c
 *
 * 11-Apr-91	dlh
 * 	decreased number of times pointers were calculated
 * 	
 * 	added smp locks around uses of vpmask, vpfree & vptotal
 * 	
 * 	disabled fault handler:  
 * 		- new affinity after vp_contextsave - now considers pre-vector 
 * 		  affinity.
 * 		- also will not allow new affinity to become zero.  this could 
 * 		  have happened if the boot cpu does not have a vp and the 
 * 		  process has been forced to the boot cpu.
 * 	
 * 	vp_reset:
 * 		set's state of CURRENT cpu, not input cpu.
 * 	
 * 	vp_contextlimbo:
 * 		- if a CPU is being stopped, then it's VP context needs to be
 * 		  saved, and it's affinity needs to be changed.  
 * 		- Also, do not let the affinity go to zero.  This would happen 
 * 		  if the secondary CPUs are being stopped and the boot
 * 		  CPU does not have an attached vector processor.  In
 * 		  this case the vector process would 'hang' until a
 * 		  secondary CPU with a VP is re-started.
 *
 * 12/20/90 - dlh
 *	added parameter to vp_reset()
 *
 * 12/18/90 -- paradis
 * 	Added vp_idle() routine.
 *
 * 10/10/90 -- paradis
 *	Added VAX 9000 support (this consisted primarily of identifying
 *	those sections peculiar to Rigel/Mariah and making them
 *	conditional on "if(cpu == VAX_6400)"
 *
 * 9/4/90 -- dlh
 *    original
 *
 *
 * ------------------------------------------------------------------------
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../machine/vectors.h"
#include "../h/cpudata.h"
#include "../h/time.h"
#include "../machine/mtpr.h"
#include "../h/proc.h"
#include "../h/kmalloc.h"
#include "../../machine/common/cpuconf.h"
#include "../h/user.h"
#include "../h/acct.h"
#include "../h/signal.h"
#ifdef VAX6400
#include "../machine/ka6400.h"
#endif VAX6400

int	max_vec_procs;

#ifdef VECTORS
vp_allocate (p)
struct proc *p;
{
/*
 * input:	pointer to proc structure of process which wants to 
 * 		allocate a vpcontext area
 * output:	VP_FAILURE or VP_SUCCESS
 * description:	Convert a regular process to a vector process by allocating a
 * 		vector context (vpcontext) area for that process.  Initialize
 *		the vpcontext area.
 * side effect:	set AVP flag in the accounting flag of the u area
 * errors:	There is a maximum number of vector processes which are 
 *		allowed.  If this maximum is exceeded then vp_allocate will 
 *		fail.
 * note:	KM_ALLOC will sleep if there is not enough memory available 
 *		for the allocation.  Therefore it is guaranteed that the 
 *		allocation will succeed.
 */
struct	vpcontext	*vpc;

	if (++num_vec_procs > max_vec_procs) {
		--num_vec_procs;
		u.u_code = TERM_VECT_TOOMANY;
		u.u_error = EAGAIN;
		uprintf ("too many vector processes\n");
		psignal (CURRENT_CPUDATA->cpu_proc, SIGTERM);
		return (VP_FAILURE);
	}

	KM_ALLOC ( p->p_vpcontext, struct vpcontext *,
	    sizeof(struct vpcontext), KM_VECTOR, KM_CLEAR | KM_CONTIG);

	vpc = p->p_vpcontext;

	u.u_acflag |= AVP;

	vpc->vpc_state = VPC_WAIT;
		/* waiting for a vector processor to be allocated for this 
		 * process; i.e.: this process is a vector process, but it 
		 * does not have it's vector context stored in any vector 
		 * processor.
		 */

	vpc->vpc_affinity = p->p_affinity;
		/* save the current affinity of the process.  This will allow 
		 * for returning to the saved affinity if this process ever 
		 * decides it is no longer a vector processes
		 */

	/* no need to initialize the vpc_refuse, vpc_cheap or the 
	 * vpc_expen fields since the memory was zeroed by KM_ALLOC() 
	 */

	KM_ALLOC (vpc->vpc_vregs, char *, 
	    VPREGSIZE, KM_VECTOR, KM_CLEAR | KM_CONTIG);

	return (VP_SUCCESS);

}

vp_proc (p)
struct proc *p;
{
/*
 * input:	pointer to proc structure of process which is requesting the
 *		attached vector processor
 * output:	PROC_MAY_HAVE_VP or PROC_MAY_NOT_HAVE_VP (defined in vectors.h)
 * description:	decide if the process can be given the attached vector
 *		processor.
 * errors:	
 * assumption:	before calling vp_proc() the caller should test to see if
 *		there is only one VP in the system.  This may be done by 
 *		using the VP_PROC macro defined in vectors.h
 * issues:	what if vptotal == 1 && vpd_state == VPD_DEAD ??  this
 *		routine will never be called.
 */
struct	cpudata	*pcpu;
struct	vpdata	*vpd;

	pcpu = CURRENT_CPUDATA ;
	vpd = pcpu->cpu_vpdata ;

	if (vpd->vpd_state == VPD_DEAD)
		return (PROC_MAY_NOT_HAVE_VP);
	else if (vpd->vpd_proc == NULL)
		return (PROC_MAY_HAVE_VP);
     /*	else if # of times this process has been refused is too great */
     /*		return (PROC_MAY_HAVE_VP);                            */
	else if (pcpu->cpu_mask == p->p_affinity)
		return (PROC_MAY_HAVE_VP);
	else if (vpfree)
		return (PROC_MAY_NOT_HAVE_VP);
	else
		return (PROC_MAY_HAVE_VP);
}

vp_remove ()
{
/*
 * input:	none
 * description:	permanently disable and remove the attached vector processor 
 *		by clearing the Vector Present bit in the Access Control and 
 *		Status register.  Also remove software reference to this 
 *		processor.
 * note:	the variable cpu is declared in cpu.h, but I have not yet
 *		tracked down where it is initialized.
 */
struct	cpudata	*pcpu;
struct	vpdata	*vpd;

	pcpu = CURRENT_CPUDATA ;

	if (cpu==VAX_6400) {
		mtpr(ACCS,mfpr(ACCS) & ~1);
	} else {
		printf ("vp_remove(): cpu %d\n", cpu);
	}

	
	/*
	 * update the system wide masks and count
	 */

	clear_bit_atomic (pcpu->cpu_num, &vpmask);
	clear_bit_atomic (pcpu->cpu_num, &vpfree);
	adawi ((-1), &vptotal);

	/*
	 * update the vpdata struct withing the cpudata struct to reflect 
	 * the fact that the attached vp is out of commission.
	 */

	vpd = pcpu->cpu_vpdata ;
	vpd->vpd_proc = NULL;
	vpd->vpd_state &= ~(VPD_ALIVE | VPD_ENABLED);
	vpd->vpd_state |= VPD_DEAD;
}

vp_contextsave (p)
struct	proc	*p;
{
/*
 * input:	proc struct pointer
 * output:	VP_FAILURE or VP_SUCCESS
 * description:	copy the vector process context area from the vector 
 *		processor to the vpcontext area pointed to by the proc 
 *		struct.
 * assumptions:	process is running on the SP-VP pair whose VP contains the
 *		vpcontext
 * note:	caller is responsible for:
 *			updating the p_affinity
 *			updating the vpc_state
 *			updating the vpd_proc
 *		this routine leaves the vector processor enabled.
 */
struct	cpudata		*pcpu;
struct	vpcontext	*vpc;
struct	vpdata		*vpd;

	pcpu = CURRENT_CPUDATA;
	vpd = pcpu->cpu_vpdata;
	vpc = p->p_vpcontext;
		
	/*
	 * when ever vector instructions will be executed while the
	 * process is kernel mode, the VPD_IN_KERNEL flag must be set.
	 */
	vpd->vpd_in_kernel = VPD_IN_KERNEL;

	/*
	 * enable the vector processor.  this will prevent a vector
	 * disabled fault
	*/
	mtpr (VPSR, (mfpr(VPSR) | VPSR_VEN));
	mfpr(VPSR);


	/*
	 * wait for the VP to be idle
	 */
	while (mfpr(VPSR) & VPSR_BSY) ;

	/*
	 * wait for completion of vector memory access, and all
	 * errors to be reproted.
	 */
	mfpr (VMAC);
	mfpr (VMAC);

	if (vpc->vpc_error & VPC_ERROR_PMF) {
		/*
		 * According to the Chap 13 of the VAX Arch spec (the
		 * chapter on vector processors), this fault will only
		 * happen in a system which implements the "asyncronous
		 * method of memeory management", which I don't think we
		 * have. (Can you tell I'm not sure of myself!)  Also,
		 * after talking to the ULTRIX vm experts, I don't think
		 * this will happen as long as I continue to guarentee
		 * that a vector process will never run on a vector
		 * processor / scalar processor pair where the scalar
		 * process is running some other process.  Until I find
		 * out different, or get to that stage of debug, this if
		 * clause will just write to the error logger.
		 *
		 * If I ever do add in code to fill this if clause it will 
		 * save the hardware state by:
		 *	1. load the VSAR (Vector State Address Register) with 
		 *	   a pointer to the hardware area.  this must be in 
		 *	   memory which is guarentee'd not to cause an 
		 *	   exception (see section 13.2.3, Internal Processor 
		 *	   Registers , of vax arch spec)
		 *	2. Set the VSS (Vector State Store) bit of the VPSR 
		 *	   (Vector Status Register).  This will cause the 
		 *	   state to be save.
		 */

		panic ("vp_contextsave(): got a VPC_ERROR_PMF, now what ?!");
	} else if (vpc->vpc_error & VPC_ERROR_IMP) {
		panic ("vp_contextsave(): got a VPC_ERROR_IMP, now what ?!");
	} else {
		/*
		 * save the vector control registers: VCR, VLR and all 
		 * 64-bits of VMR
		 */

		vpc->vpc_vcr = mfvcr();
		vpc->vpc_vlr = mfvlr();
		vpc->vpc_vmrlo = mfvmrlo();
		vpc->vpc_vmrhi = mfvmrhi();

		/*
		 * save the vector registers (load the length register and 
		 * issue the Store Vector Register Data into Memory 
		 * instruction)
		 * (debby: does this need to be a macro or maybe a routine?)
		 */

		mtvlr (64);
		vstq (vpc->vpc_vregs);

		/* wait for the store to complete */

		while (mfpr(VPSR) & VPSR_BSY) ;
		mfpr (VMAC);
		mfpr (VMAC);
		
	}

	vpd->vpd_in_kernel = ~ VPD_IN_KERNEL;

	if (vpc->vpc_error & VPC_ERROR_IMP)
		return (VP_FAILURE);
	else
		return (VP_SUCCESS);
}

vp_contextrest (p)
struct	proc	*p;
{
/*
 * input:	proc struct pointer
 * output:	VP_FAILURE or VP_SUCCESS
 * description:	copy the vector process context area from the vpcontext 
 *		area pointed to by the proc struct to the vector processor 
 * note:	caller is responsible for updating the vpc_state and vpd_proc
 */
struct	vpdata		*vpd;
struct	vpcontext	*vpc;

	vpd = CURRENT_CPUDATA->cpu_vpdata;
	vpc = p->p_vpcontext;

	vpd->vpd_in_kernel = VPD_IN_KERNEL;

	if (vpc->vpc_error & VPC_ERROR_IMP) {
		panic ("vp_contextrest(): got a VPC_ERROR_IMP, now what ?!");
	} else {
		/*
		 * restore the vector registers (load the length register and 
		 * issue the Load Memory Data into Vector Register 
		 * instruction)
		 * (debby: does this need to be a macro or maybe a routine?)
		 */

		mtvlr (64);
		vldq (vpc->vpc_vregs);
		
		/*
		 * restore the vector control registers: VCR, VLR and all 
		 * 64-bits of VMR
		 */

		mtvcr(vpc->vpc_vcr);
		mtvlr(vpc->vpc_vlr);
		mtvmrlo(vpc->vpc_vmrlo);
		mtvmrhi(vpc->vpc_vmrhi);

		/* 
		 * wait for the vector instruction(s), the memory references 
		 * to complete and all errors to be reported
		 */

		while (mfpr(VPSR) & VPSR_BSY) ;
		mfpr (VMAC);
		mfpr (VMAC);

		if (vpc->vpc_error & VPC_ERROR_PMF) {
			/*
			 * According to the Chap 13 of the VAX Arch spec (the 
			 * chapter on vector processors), this fault will only 
			 * happen in a system which implements the 
			 * "asyncronous method of memeory management", which 
			 * is on neither Rigel nor Aquarius, but will be 
			 * on Mariah, so I'll worry about this one 
			 * latter! Also, after talking to the ULTRIX vm 
			 * experts, I don't think this will happen as long as 
			 * I continue to guarentee that a vector process will 
			 * never run on a vector processor / scalar processor 
			 * pair where the scalar process is running some other 
			 * process.  Until I find out different, or get to 
			 * that stage of debug, this if clause will just write 
			 * to the error logger.
			 *
			 * If I ever do add in code to fill this if clause it 
			 * will restore the hardware state by:
			 *	1. load the VSAR (Vector State Address 
			 *	   Register) with a pointer to the hardware 
			 *	   area.  this must be in memory which is 
			 *	   guarentee'd not to cause an exception (see 
			 *	   section 13.2.3, Internal Processor 
			 *	   Registers , of vax arch spec)
			 *	2. Set the RLD (vector ReLoaD) bit of the VPSR 
			 *	   (Vector Status Register).  This will cause 
			 *	   the state to be restored.
			 *	3. Clear the soft copy of vpcontext area
			 *	   Pending Memory Fault (PMF) bit
			 *	   (p->p_vpcontext->vpc_error &= ~VPC_ERROR_PMF)
			 */
	
			panic ("vp_contextrest(): VPC_ERROR_PMF, now what ?!");
		}

	}

	vpd->vpd_in_kernel = ~ VPD_IN_KERNEL;

	if (vpc->vpc_error & VPC_ERROR_IMP)
		return (VP_FAILURE);
	else
		return (VP_SUCCESS);
}

vp_cleanup (p)
struct	proc	*p;
{
/*
 * input:	proc struct pointer
 * output:	none
 * description:	make sure usage of the vp is complete.  this includes 
 *		releasing the KM_ALLOC'd memory.  leave the vp in a disabled 
 *		state.
 * assumption:	test for a vp_context area has already been made.
 */

int	saveaffinity;
struct	cpudata		*pcpu;
struct	vpdata		*vpd;
struct	vpcontext	*vpc;

	pcpu = CURRENT_CPUDATA;
	vpd = pcpu->cpu_vpdata;
	vpc = p->p_vpcontext;

	/* debby:  if the vpcontext state is VPC_SAVED, then there is no 
	 *		need to wait on the vp, return now.
	 */

	if (p != vpd->vpd_proc) {
		/*
		 * if this process does not own the VP then don't touch the
		 * hardware!
		 */
		goto resume_scalar;
	}


	/* 
	 * wait for the VP to go idle, and all memory references to complete 
	 * and all errors to be reported
	 */

	while (mfpr(VPSR) & VPSR_BSY) ;
	mfpr (VMAC);
	mfpr (VMAC);


	/* 
	 * debby: is a RST really necessary ???
	 * clear the VPSR (vector processor status register) and the VAER 
	 * (vector arithmetic exception register, This will also clear all 
	 * the exceptions in the VP.  Leave the VP disabled
	 * Also re-enable memory mapping for the vector processor (the reset
	 * disabled it)
	 */

	mtpr (VPSR, VPSR_RST);
	mfpr(VPSR);

	if(cpu == VAX_6400) {
		/* Enable mapping */
		mtpr(VIADR, 0x509);
		mtpr(VIDLO, 1);

		/* Initialize cache controller: Enable memory transactions,
		 * enable cache, enable error reporting, clear errors,
		 * and flush cache.  Do this by writing a 0x138e00 to the 
		 * VIR_LSX_CCSR.
		 */
		mtpr(VIADR, 0x520);
		mtpr(VIDLO, 0x138e00);

		/* Initialize vector controller status register; enable
		 * hard and soft error reporting.
		 */
		mtpr(VIADR, 0x489);
		mtpr(VIDLO, 0xc0000);
	}


	/* 
	 * clear the vp_proc pointer in the cpudata struct, and set the 
	 * vpdata state to VPD_ALIVE | VPD_DISABLED (VP is alive and 
	 * operational and disabled) 
	 */

	if (p == vpd->vpd_proc ) {
		vpd->vpd_proc =  NULL;
		vpd->vpd_state &= ~VPD_ENABLED;
		vpd->vpd_state |= VPD_DISABLED;
		set_bit_atomic (pcpu->cpu_num, &vpfree);
	}

resume_scalar:

	/* sanity check */
	if ((u.u_acflag & AVP) != AVP)
		uprintf ("u_acflag error, proc #%d\n", u.u_procp->p_pid);

	/* save the old affinity before deallocating the vpcontext area */
	saveaffinity = vpc->vpc_affinity ;

	/* deallocate the vpcontext area */

	KM_FREE (vpc->vpc_vregs, KM_VECTOR);
	vpc->vpc_vregs = 0;

	KM_FREE (vpc, KM_VECTOR);
	p->p_vpcontext = 0;

	/* clear the vector process flag */
	u.u_acflag &= ~AVP;
	--num_vec_procs;

	/*
	 * restore the original process affinity.  
	 */
	p->p_affinity = saveaffinity ;
}

vp_contextlimbo (cpudata_ptr)
struct	cpudata	*cpudata_ptr;
{
int	before, after;
int	new_affinity;

/*
 * input:	cpudata struct pointer
 * output:	none
 * description:	
 *		- in most cases, this routine will leave the context area a 
 *		  vector process in a state of limbo.  The vector context will 
 *		  still be in the vector processor, but it will no longer be 
 *		  the process running on the scalar process.
 *		- this routine will leave the vector processor in an idle and 
 *		  disabled state.
 *		- this routine may decide to context save the vector context.
 *		  - if this process has an affinity to more than one
 *		    processor.
 *		  - if this process is being debugged (ptrace'd)
 */
struct	vpdata		*vpd;
struct	vpcontext	*vpc;
struct	proc		*vproc;

	vpd = cpudata_ptr->cpu_vpdata;
	vproc = vpd->vpd_proc;
	vpc = vproc->p_vpcontext;
	
	if (vpc->vpc_state == 
	    VPC_LOAD) {
		/*
		 * wait for the VP to be idle
		 */
		while (mfpr(VPSR) & VPSR_BSY) ;

		/*
		 * wait for completion of vector memory access, and all
		 * errors to be reproted.
		 */
		mfpr (VMAC);
		mfpr (VMAC);

		/*
		 * update the state of this process - record the fact that 
		 * there is a valid vector context in the vector processor - 
		 * this will remain true even if there is another process's 
		 * context in the scalar processor.  note: the disabled fault 
		 * handler may decide to context this vector process at a 
		 * latter time.
		 */
		vpc->vpc_state = 
		    VPC_LIMBO;
	}
	/*
	 * if the process's affinity is not limiited to one (this) processor, 
	 * then do a vp_contextsave().  this will prevent this process from 
	 * being scheduled on some other scalar processor while it's vector 
	 * context is still stored in this scalar-vector pair.
	 * Also, if this process is being debugged, then do a vp_contextsave() 
	 * so that the debugger has access to the vector registers and state.
	 * Also, if this processor is being stopped, then do a 
	 * vp_contextsave().  
	 */
	if ( (vproc->p_affinity != 
	     cpudata_ptr->cpu_mask) || (cpudata_ptr->cpu_stops) ) {
		/*
		 * save the process's context; set VPD_IN_KERNEL flag before 
		 * call, and clear it after.  This tells the Disabled Fault
		 * handler that we know that we are executing vector 
		 * instructions in kernel mode.
		 * debby: may be able to avoid this vp_contextsave() if the
		 *  vpc_state is VPC_SAVED.
		 */
		vpd->vpd_in_kernel = VPD_IN_KERNEL;

		if (vp_contextsave (vproc) == 
		    VP_FAILURE) {
			/*
			 * vp_contextsave() will only fail
			 * if it gets a vector IMP error.  When this
			 * happen the system panics, to the following
			 * signal will never be reached.  However... Some
			 * future vector implementation may allow a
			 * recovery from an IMP error.  If that ever
			 * happens then the following psignal should be
			 * reconsidered.
			 */
			psignal (vproc, SIGKILL);
		}

		vpd->vpd_in_kernel = ~ VPD_IN_KERNEL;

		/* 
		 * update the state of this process - record the fact that 
		 * this process does not have it's context stored in any VP
		 */
		vpc->vpc_state = 
		    VPC_SAVED;

		/*
		 * update the processes affinity so that it can run on any 
		 * CPU which has an attached vector processor.  Remember to 
		 * account for any pre-vector process affinity.
		 * note: do not let the affinity become zero.  testing has 
		 *       shown that this may happen if the stopcpu system 
		 *       call is made when the boot cpu does not have a 
		 *       vector processor.
		 */
		new_affinity = vpmask & 
		   vpc->vpc_affinity;
		if (new_affinity)
		   vproc->p_affinity = new_affinity;


		/*
		 * update the vpfree mask
		 * note: if this cpu is being stopped, then leave vpfree
		 *       alone.  stopcpu() has already updated vpfree.
		 */
		if ( ! (cpudata_ptr->cpu_stops) ) {
			set_bit_atomic (cpudata_ptr->cpu_num, &vpfree);
		}

		/* 
		 * mark the vector processor as free by clearing the vpd_proc 
		 * pointer in the cpudata structure of this cpu 
		 */
		vproc = NULL;
	}

	/*
	 * disable the vector processor.  this is done by writing a 0 to the 
	 * VPSR followed by a read of the VPSR.
   	 * note:  this will leave the exceptions states intact so that a 
	 * later call of the vector processor disabled fault handler can 
	 * examine the exception states.
	 */
	mtpr (VPSR, 0);
	mfpr (VPSR);
	vpd->vpd_state &= ~VPD_ENABLED;
	vpd->vpd_state |= VPD_DISABLED;
}

#define	LS_VINTSR_SET	(VINTSR_BUS_TIMEOUT      | VINTSR_VECTOR_UNIT_HERR)
#define	LS_VINTSR_CLR	(VINTSR_VECTOR_UNIT_SERR | VINTSR_VECTL_VIB_SERR | \
			 VINTSR_VECTL_VIB_HERR   | VINTSR_CCHIP_VIB_SERR | \
			 VINTSR_CCHIP_VIB_HERR)

#define	VIR_MOD_REV		0x48A
#define	VIR_VCTL_CSR		0x489
#define	VIR_LSX_CCSR		0x520
#define	VIR_ALU_DIAG_CTRL	0x45C

#define	VCTL_CSR_LSS         0x1	/* Load store soft error */
#define	VIR_VCTL_CSR_CDS     0x4	/* Soft internal bus error */
#define	VIR_VCTL_CSR_VIS     0x10	/* VIB bus soft error */
#define	VIR_VCTL_CSR_VIH     0x20	/* VIB bus hard error */
#define	VIR_VCTL_CSR_ISE     0x40	/* Illegal sequence error */
#define	VIR_VCTL_CSR_VHE     0x800	/* Verse hard error */

#define	LS_VCTL_CSR_CLR	( VCTL_CSR_LSS | VIR_VCTL_CSR_CDS |     \
			  VIR_VCTL_CSR_VIS | VIR_VCTL_CSR_VIH | \
			  VIR_VCTL_CSR_ISE | VIR_VCTL_CSR_VHE )

#define	VIR_LSX_CCSR_CPE	0x200	/* Cache parity error */
#define	VIR_LSX_CCSR_XSE	0x400	/* XMI interface soft error */
#define	VIR_LSX_CCSR_XHE	0x800	/* XMI interface hard error */

#define	LS_LSX_CCSR_CLR	( VIR_LSX_CCSR_CPE | VIR_LSX_CCSR_XSE | \
			  VIR_LSX_CCSR_XHE )

#define	VIR_ALU_DIAG_CTRL_ABE		0x100	/* AB bus parity error */
#define	VIR_ALU_DIAG_CTRL_CPE		0x200	/* CP bus parity error */
#define	VIR_ALU_DIAG_CTRL_IFO		0x400	/* Illegal FAVOR opcode */

#define	LSX_ALU_DIAG_CTRL_CLR	( VIR_ALU_DIAG_CTRL_ABE |   \
				  VIR_ALU_DIAG_CTRL_CPE |   \
				  VIR_ALU_DIAG_CTRL_IFO )

#ifdef	TRUE
#undef	TRUE
#endif	TRUE
#define	TRUE	1

#ifdef	FALSE
#undef	FALSE
#endif	FALSE
#define	FALSE	0

vp_ls_bug(ls_vintsr)
long	ls_vintsr;
{
long	ls_mod_rev;
long	ls_vctl_csr;
long	ls_lsx_ccsr;
long	ls_alu_diag_ctrl;
long	tmp_vintsr;

	if (cpu != VAX_6400) {
		return (FALSE);
	} else {

		/*
		 * Disable the vector processor by clearing the present
		  * bit in the ACCS and by setting the disabled bit in the
		  * VINTSR.  Save a copy of the VINTSR
		 */

		mtpr (ACCS, mfpr(ACCS) & ~1);
		ls_vintsr = mfpr (VINTSR);
		mtpr (VINTSR, ls_vintsr | VINTSR_DISABLE_VECT_INTF);

		/*
		 * Unlock the VINTSR by clearing all the error bits.
		 * This is done by writing a one to the error bits.
		 * Enable the vector processor by writing a 0 to the
		 * disabled bit in the VINTSR and setting the present bit
		 * in the ACCS.
		 */

		mtpr (VINTSR, ( VINTSR_VECTOR_UNIT_SERR |
				VINTSR_VECTOR_UNIT_HERR |
				VINTSR_VECTL_VIB_SERR |
				VINTSR_VECTL_VIB_HERR |
				VINTSR_CCHIP_VIB_SERR |
				VINTSR_CCHIP_VIB_HERR |
				VINTSR_BUS_TIMEOUT ) );
		mtpr (ACCS, (mfpr(ACCS) | 1));

		/*
		 * Wait for not busy
		 */

		while (mfpr(VPSR) & VPSR_VEN) ;

		/* VMS does another disable - enable&unlock here */

		/*
		 * Get a copy of the MOD_REV register.  This register
		 * can only be accessed indirectly through the VIADDR and
		 * VIDLO registers
		 */

		mtpr (VIADR, VIR_MOD_REV);
		ls_mod_rev = mfpr (VIDLO);

		/* VMS does another disable - enable&unlock here */

		/*
		 * Get a copy of the VCTL CSR register.  This register
		 * can only be accessed indirectly through the VIADDR and
		 * VIDLO registers
		 */

		mtpr (VIADR, VIR_VCTL_CSR);
		ls_vctl_csr = mfpr (VIDLO);

		/* VMS does another disable - enable&unlock here */

		/*
		 * Get a copy of the LSX_CCSR register.  This register
		 * can only be accessed indirectly through the VIADDR and
		 * VIDLO registers
		 */

		mtpr (VIADR, VIR_LSX_CCSR);
		ls_lsx_ccsr = mfpr (VIDLO);

		/* VMS does another disable - enable&unlock here */

		/*
		 * Get a copy of the DIAG_CTRL register.  This register
		 * can only be accessed indirectly through the VIADDR and
		 * VIDLO registers
		 */

		mtpr (VIADR, VIR_ALU_DIAG_CTRL);
		ls_alu_diag_ctrl = mfpr (VIDLO);

		/* VMS does a disable here */

		/*
		 * If the vpd_in_kernel flag is set, then the mchk
		 * probably happened while executing the disabled fault
		 * handler.  In this case the vector processor should be
		 * left in an enabled state.  This way when the
		 * instruction is re-started and control is returned to
		 * the disabled fault handler, it will where it left
		 * off.  If the vpd_in_kernel flag is not set then the
		 * vector instruction causing the mchk was in a user
		 * program, so leave the processor disabled so that when
		 * the instruction is restarted, it will hit the disabled
		 * fault handler.
		 */

		if ((CURRENT_CPUDATA->cpu_vpdata->vpd_in_kernel | VPD_IN_KERNEL)
			== VPD_IN_KERNEL) {
			mtpr (VINTSR, 0);
		} else {
			mtpr (VINTSR, VINTSR_DISABLE_VECT_INTF);
		}

		if ((ls_vintsr & LS_VINTSR_SET) != LS_VINTSR_SET) {
			return (FALSE);
		} else if ((ls_vintsr & LS_VINTSR_CLR) != 0) {
			return (FALSE);
		} else if ((ls_mod_rev & 0x80) != 0x80) {
			return (FALSE);
		} else if ((ls_vctl_csr & LS_VCTL_CSR_CLR) != 0) {
			return (FALSE);
		} else if ((ls_lsx_ccsr & LS_LSX_CCSR_CLR) != 0) {
			return (FALSE);
		} else if ((ls_alu_diag_ctrl & LSX_ALU_DIAG_CTRL_CLR) != 0) {
			return (FALSE);
		} else {
			return (TRUE);
		}
	}
}

#define	VPC_ERROR	(VPC_ERROR_IMP | VPC_ERROR_IVO | VPC_ERROR_AEX | \
			 VPC_ERROR_PMF)
#define	VPSR_ERROR	(VPSR_IMP | VPSR_IVO | VPSR_AEX | VPSR_PMF)

vp_disabled_fault_handler (mode)
int	mode;	/* VP_DIS_USER_MODE or VP_DIS_KERN_MODE */
{
/*
 * input:	mode	VP_DIS_USER_MODE or 
 *			VP_DIS_KERN_MODE
 * output:	unknown at this time
 * description:	
 */

int		new_affinity;	/* new affinity mask of a process becoming a 
				 * vector process or of a process which needs
  				 * to switch it's affinity for some other 
				 * reason.
				 */
int		save_VPSR;	/* save contents of VPSR for multiple tests
				 * of various error bits.
				 */
struct	cpudata		*pcpu;
struct	vpdata		*vpd;
struct	vpcontext	*current_vpc;
struct	vpcontext	*owning_vpc;

    pcpu = CURRENT_CPUDATA;
    vpd = pcpu->cpu_vpdata;
    current_vpc = pcpu->cpu_proc->p_vpcontext;

    if (mode == VP_DIS_KERN_MODE) {
	/*
	 * process is running in kernel mode.  since kernel code is not 
	 * supposed to use the VP for calculating, this is most likely a 
	 * fatal error.  there are some exceptions to this:
	 * 1. during vector process context switches, kernel mode code 
	 *    will move data to/from VP registers.  however, the code 
	 *    which does this will set the VPD_IN_KERNEL flag and make sure
	 *    that the vector processor is enabled.
	 * 2. an implentation dependent error may have occured.  in this 
	 *    case it may be possible to switch processors and keep going.
	 */
	if (vpd->vpd_proc == NULL) {
		/* 
		 * This process does not "own" the Vector Processor.  Either 
		 * this process issued a vector instruction while in kernel 
		 * mode, or some hardware error has occured.  In either case, 
		 */
		panic("DVF in Kernel Mode - no owner of the vector processor");
	} else
	  if (vpd->vpd_in_kernel !=  VPD_IN_KERNEL) {
		/*
		 * since this process is running in kernel mode, it should 
		 * have set the VPD_IN_KERNEL flag before doing anything that 
		 * could have cause this fault.
		 */
		panic("DVF in Kernel mode - vpd_in_kernel not set");
	} else if (mfpr(VPSR) & VPSR_IMP) {
		/*
		 * There was an implementation dependent error.  There may be 
		 * hope.  go see if this is a fatal error (fatal to the VP, 
		 * that is).  It is possible that the VP had a problem which 
		 * need not be fatal to the process.
		 */
		/* NOTE:  
		 * to date, the only processors (6000-400, 6000-500 and 9000) 
		 * which have vectors cannot produce this error.  So, 
		 * vp_imp() will panic if it is called.  BUT:  if this 
		 * section of code is ever to be executed, it should be 
		 * reviewed first.  There are a couple of problems that I 
		 * can see:
		 *	1. if the VP went bad with state in it, then the 
		 *	   user process should be killed, not switched to 
		 *	   another processor.
		 *	2. if the VP state is saved, then the process can 
		 *	   continue, but switch_affinity should not be 
		 *	   called.  If the disabled fault is allowed to 
		 *	   just exit, then the retry of the instruction 
		 *	   will automatically cause it to be moved to 
		 *	   another processor
		 */
		if (vp_imp() == VP_IMP_FATAL) {
			/*
			 * permanently diable the VP and remove knowledge of
			 * it's existance
			 */
			vp_remove ();

			/* update statistics here */

			/*
			 * set the VPC_ERROR_IMP bit in vpc_error so that when 
			 * this instruction is re-tried on another processor, 
			 * or if the process dies, it will still know that 
			 * there was a IMP failure.
			 */
			current_vpc->vpc_error |= 
				VPC_ERROR_IMP;

			/*
			 * if there is still a functioning vector processor, 
			 * then set process affinity to vpmask.  This way when  			 * the instruction is re-tried it will either get 
			 * another disabled fault on another processor.  if 
			 * this was the last functioning vector processor in 
			 * this system (know this by checking vptotal) then 
			 * kill the process.
			 */

			if (vptotal) {
				/*
				 * switch_affinity()  (defined in 
				 * sys/kern_subr.c) is a routine to assign a 
				 * new affinity to a process and to switch the 
				 * process to the new cpu.  this routine will 
				 * do the swtch().  it uses u.u_procp as the 
				 * proc pointer, so at least for debug 
				 * purposes i want to check this out. 
				 */
				
				if (pcpu->cpu_proc != u.u_procp)
					panic ("DVF: pcpu->cpu_proc != u.u_procp");
				new_affinity = vpmask & 
				    current_vpc->vpc_affinity;
				if (new_affinity == 0)
					panic ("DVF: vector affinity problem");
				else {
					switch_affinity (new_affinity);
    					pcpu = CURRENT_CPUDATA;
    					vpd = pcpu->cpu_vpdata;
    					current_vpc = 
					    pcpu->cpu_proc->p_vpcontext;
				}

			} else { /* no more functioning VPs in system */
				/* need a u_code or a u_error here */
				u.u_code = TERM_VECT_HARD;
				psignal (pcpu->cpu_proc, SIGKILL);
			}
		} else { 
			/* 
			 * IMP error was not fatal to the VP, so it is fatal 
			 * to the process.  Set the VPC_ERROR_IMP bit in
			 * vpc_error so that when this process dies, it will 
			 * know that there was an IMP failure.
			 */
			current_vpc->vpc_error |= VPC_ERROR_IMP;

			/*
			 * send a SIGKILL (kill signal) to the process
			 * note:  if there is ever a vector
			 *	  implementation which can recover from a IMP
			 *	  error, then it might be possible to let the
			 *	  process continue.
			 */

			psignal (pcpu->cpu_proc, SIGKILL);
		}
	} else
		/* 
		 * This process is in Kernel mode, the vpd_in_kernel flag is 
		 * set and there has not been an IMP error, so some kernel 
		 * mode code is accessing VP registers without first making 
		 * sure that the VP is enabled, or something went wrong with 
		 * load/store/test of a VP register.  The only thing left to 
		 * do is ... panic
		 *
		 * debby - note that the design spec spelled out checking for 
		 * AEX and IVO.  since i am panicing at this point, i don't 
		 * see the purpose, but i didn't want to forget that i had 
		 * made this decision
		 */
		panic("DVF while in Kernel Mode");
    } else { /* in user mode */
	/*
	 * wait for the VP to go idle, all memory references to
	 * complete and all errors to be reported.  This is a hook for
	 * future changes to the vector support.  If, in the futrue, we
	 * decide to let one process run on the VP while another runs on
	 * the SP, then this step becomes necessary.  As long as we force
	 * the same process to be running on the SP as is running on the
	 * VP this step is uncessary.
	 */

	while (mfpr(VPSR) & VPSR_BSY) ;
	mfpr (VMAC);
	mfpr (VMAC);


	/*
	 * sanity check - debug only
	 * make sure cpu_proc and u_procp agree
	 */
	if (pcpu->cpu_proc != u.u_procp)
	    panic ("DVF: CURRENT_CPUDATA->cpu_proc and u.uprocp do not agree");

	/*
	 * if this process does not have a vpcontext area then it asking to 
	 * become a vector process.  go ahead and call vp_allocate() to 
	 * allocate the vpcontext area, and to intialize it.  mask out the
	 * non-vector present processors from the process affinity.  this will 
	 * limit this process to:
	 *	1. scalar processors it was already limited to
	 *	2. scalar processors which are part of a scalar/vector pair
	 * note: do not set affinity to just this processor because there is a
	 * possibility that vp_proc will tell us that we cannot have this
 	 * processor.
	 * if the new process affinity is zero (this will only happen if the
	 * process was already limited to a set of processors which were not 
	 * part of a scalar/vector pair) then kill the process.
	 */
	if (current_vpc == NULL) {
		if (vp_allocate (pcpu->cpu_proc) == VP_FAILURE) {
			return;
		}
		current_vpc = pcpu->cpu_proc->p_vpcontext;
		new_affinity = pcpu->cpu_proc->p_affinity & vpmask;
		if (new_affinity == 0) {
			panic ("DVF: vector affinity problem");
		} else {
			pcpu->cpu_proc->p_affinity = new_affinity;
		}
		current_vpc->vpc_state = VPC_WAIT;
	}


	/*
	 * check for and deal with any errors which may have occured.
	 * if the vector processor has no "owner" then
	 *	- clear the hardware (if necessary)
	 *	- log the error
	 * 	- allow the process which caused this disabled vector 
	 *	  processor error to continue.
	 * if the vector processor has an "owner" then
	 *	- charge the owning process  with the error
	 *	- determine if the owning process can continue processing 
	 *	  (most likely not).
	 */

	if (vpd->vpd_proc == NULL) {
		/*
		 * there is no current vector process which "owns" this vector 
		 * processor, so clean up any pending errors
		 */
		if (mfpr(VPSR) & VPSR_IMP) {
			/*
			 * there has been an implementation dependent error.  
			 * call vp_imp() to perform any necessary error 
			 * recovery and to determine if the error is fatal to 
			 * the vector processor.
			 */
			/* NOTE:  
			 * to date, the only processors (6000-400, 6000-500 
			 * and 9000) which have vectors cannot produce this 
			 * error.  So, vp_imp() will panic if it is called.
			 */
			if (vp_imp() == VP_IMP_FATAL) {
				/*
				 * permanently diable the VP and remove 
				 * knowledge of it's existance
				 */
				vp_remove ();

				/* update statistics here */

				if (vptotal) {
					/* switch_affinity()  (defined in 
					 * sys/kern_subr.c) is a routine to 
					 * assign a new affinity to a process 
					 * and to switch the process to the 
					 * new cpu.
					 */

					/* the new affinity of this process is 
					 * the logical and of the processors 
					 * which were in the affinity mask 
					 * before this process became a vector 
					 * process and the cpu mask of those 
					 * processors which have a functioning 
					 * vector processor.  if this new 
					 * affinity is zero, then there are no 
					 * processors which are capable of 
					 * running this process
					 */

					new_affinity = vpmask & 
					    current_vpc->vpc_affinity;
					if (new_affinity == 0) {
					    panic 
					    ("DVF: vector affinity problem");
					} else {
						switch_affinity (new_affinity);
						pcpu = CURRENT_CPUDATA;
						vpd = pcpu->cpu_vpdata;
						current_vpc =
						    pcpu->cpu_proc->p_vpcontext;
					}

				} else { /* no more functioning VPs in system */
					u.u_code = TERM_VECT_HARD;
					psignal (pcpu->cpu_proc, SIGKILL);
					return ;
				}
			} /* the IMP error was not fatal to the VP */
			panic ("DVF: uerf: IMP error, u mode, no own\n");
		} /* endif - test for VPSR_IMP error */
		if (mfpr(VPSR) & VPSR_AEX)
			/* vector processor arithmetic exception error */
			mprintf ("DVF: uerf: AEX error, u mode, no owner\n");
		if (mfpr(VPSR) & VPSR_IVO)
			/* vector processor illegal operand error */
			panic ("DVF: uerf: IVO error, u mode, no owner\n");
	} else { /* some process owns the VP */

		owning_vpc = vpd->vpd_proc->p_vpcontext;

		/*
		 * look for the errors.  if there are any errors then log 
		 * them in the vpc_error field of the vpcontext structure and 
		 * clear the hardware. 
		 */
		save_VPSR = mfpr (VPSR);
		if (save_VPSR & VPSR_IMP) {
			/*
			 * there has been an implementation dependent error.  
			 * call vp_imp() to handle the error and to determine 
			 * if it is fatal to the vector processor.  if it is 
			 * fatal to the vector processor, then call 
			 * vp_remove() to perminently remove the vector 
			 * processor from consideration as an operational 
			 * vector processor.
			 */
			/* NOTE:  
			 * to date, the only processors (6000-400, 6000-500 
			 * and 9000) which have vectors cannot produce this 
			 * error.  So, vp_imp() will panic if it is called.
			 */
			if (vp_imp() == VP_IMP_FATAL)
				vp_remove();

			owning_vpc->vpc_error |= VPC_ERROR_IMP;
		}
		if (save_VPSR & VPSR_AEX) {
			/*
			 * there has been an arithmetic exception error on the 
			 * vector processor.  set the error bit in the 
			 * process's vpcontext struct, and make a copy of the 
			 * Vector Arithmethic Exception Register (this 
			 * register contains the encoded exception condition 
			 * summary - see chap 13 of the VAX Architecture 
			 * Standard for a breakdown of the contents of this 
			 * register.
			 */

			owning_vpc->vpc_error |= VPC_ERROR_AEX;
			owning_vpc->vpc_vaer = mfpr(VAER);
		}
		if (save_VPSR & VPSR_IVO) {
			/*
			 * there has been an illegal vector opcode error.  set 
			 * the error bit in the process's vpcontext struct.  
			 * debby - need to save pc ??  if so where and how ??
			 */

			owning_vpc->vpc_error |= VPC_ERROR_IVO;
		}
		if (save_VPSR & VPSR_PMF) {
			/*
			 * there has been a pending memory fault error.  this 
			 * is cause for panic, since I have absolutely no code 
			 * in place to handle this
			 */
			panic("DVF: pending memory fault");
		}


		/*
		 * now that we have save off all the info we need, it there
		 * have been any errors, then reset clear the * hardware.  
		 * then decide whether the process lives or dies.
   		 * if it dies as the result of an arithmetic error, then 
		 * send it a nice error packet.
		 */

		if (save_VPSR & VPSR_ERROR) {

			/* 
			 * there was some error, so reset the hardware.  
			 * this is done by setting the VPSR_BIT.  This 
			 * will clear the VPSR (vector processor status 
			 * register) and the VAER (vector arithmetic 
			 * exception register. This will also clear all 
			 * the exceptions in the VP.
			 */

			mtpr (VPSR, VPSR_RST);
			mfpr(VPSR);

			if(cpu == VAX_6400) {
				/* note:  re-setting the Rigel/Mariah
				 * vector processor disables memory
				 * mapping; need to re-enable it here.
				 */

				/* Enable mapping */
				mtpr(VIADR, 0x509);
				mtpr(VIDLO, 1);

				/* Initialize cache controller: Enable memory 
				 * transactions, enable cache, enable error 
				 * reporting, clear errors, and flush cache.
				 */
				mtpr(VIADR, 0x520);
				mtpr(VIDLO, 0x138e00);

				/* Initialize vector controller status 
				 * register enable hard and soft error 
				 * reporting.
				 */
				mtpr(VIADR, 0x489);
				mtpr(VIDLO, 0xc0000);

				/* read VMAC ??? */
				/* do SYNC ??? */
			}


			if (owning_vpc->vpc_error & VPC_ERROR_IMP) {
			    u.u_code = TERM_VECT_HARD;
			    psignal (vpd->vpd_proc, SIGKILL);
			}
			if (owning_vpc->vpc_error & VPC_ERROR_IVO) {
			    u.u_code = ILL_VECOP_FAULT;
			    uprintf
			      ("illegal vector opcode on vector capable cpu\n");
			    psignal (vpd->vpd_proc, SIGILL);
			    return ;
			}
			if (owning_vpc->vpc_error & VPC_ERROR_AEX) {
			    if (vpd->vpd_proc  != u.u_procp)
			        panic ("DVF: not loading VAER into u_code");
			    else
			        u.u_code = owning_vpc->vpc_vaer;
			    psignal (vpd->vpd_proc, SIGFPE);
			}

			/*
			 * if the current proc and the owning proc are the 
			 * same, then return.  this will give control back 
			 * to trap().  trap() will see the signal and kill 
			 * the process.  since the process is killed, the 
			 * instruction which caused the disabled fault will 
			 * not be re-issued (debby - is this true ???)
			 */

			if ((owning_vpc->vpc_error & VPC_ERROR) && 
			    (pcpu->cpu_proc == vpd->vpd_proc)) {
				return ;
			}
		}


	} /* end else VP has an owner */

	/* end error processing */


	if ( u.u_procp == vpd->vpd_proc ) {

		/* the current process already owns the vector processor */

		/* Refresh the memory management registers */
		/*
		 * This is necessary for Rigel, and so it must be
		 * necessary for Mariah.  Not necessary for Aquarius.
		 */

		if(cpu == VAX_6400) {
			mtpr (P0BR, mfpr (P0BR));
			mtpr (P0LR, mfpr (P0LR));
			mtpr (P1BR, mfpr (P1BR));
			mtpr (P1LR, mfpr (P1LR));
			mtpr (SBR, mfpr (SBR));
			mtpr (SLR, mfpr (SLR));
		}

		/*
		 * since there is an "owner" for the VP, it cannot be VPC_WAIT 
		 * nor VPC_SAVED.  it must be VPC_LOAD or VPC_LIMBO.  if it is 
		 * VPC_LOAD then the translation buffer is already o.k.  if 
		 * it is VPC_LIMBO, then the translation buffer needs to be 
		 * invalidated
		 */

		if (owning_vpc->vpc_state == VPC_LIMBO) {
			/* flush the vector translation buffer */
			/*
			 * This is necessary for Rigel, and so it must be
			 * necessary for Mariah.  Not necessary for Aquarius.
			 */
			if(cpu == VAX_6400)  mtpr (VTBIA, 0);
			owning_vpc->vpc_state = VPC_LOAD;
		}

		/* update statistics */

		/* cheap context switches per process */
		owning_vpc->vpc_cheap ++ ;

		/* cheap context switches per cpu */
		vpd->vpd_ccsw ++ ;

		/* requesting process given "ownership" of vector processor */
		vpd->vpd_success ++ ;

		/* 
		 * enable the vector processor (that is, after all, why I'm
		 * here
		 */

		mtpr (VPSR, (mfpr(VPSR) | VPSR_VEN));
		mfpr(VPSR);
		vpd->vpd_state &= ~ VPD_DISABLED;
		vpd->vpd_state |= VPD_ENABLED;
		return ;
	}

	/*
	 * current process does not own the vector processor.  if this is the 
	 * only vector processor in the system, then take it.  if there is 
	 * more than one, then call vp_proc() to ask permission to take it.
	 */

	if ((vptotal == 1) || (vp_proc(pcpu->cpu_proc) == PROC_MAY_HAVE_VP)) {
		/*
		 * go ahead and take the vector processor.  
		 * since we might have to call vp_contextsave() or 
		 * vp_contextrest() which will be executing stuff in kernel 
		 * mode, enable the vector processor now.
		 */


		mtpr (VPSR, (mfpr (VPSR) | VPSR_VEN) );
		mfpr(VPSR);
		vpd->vpd_state &= ~ VPD_DISABLED;
		vpd->vpd_state |= VPD_ENABLED;

		/*
		 * look to see if there is a context in the vector processor.
		 * if there is, then the vector processor is "owned" by 
		 * someone else, and that vector process needs to be saved 
		 * before this vector process can "steal" the vector 
		 * processor.
		 */

		if (vpd->vpd_proc != NULL) {

			if (vp_contextsave (vpd->vpd_proc) == VP_FAILURE) {
				/*
				 * vp_contextsave() will only fail if it
				 * gets a vector IMP error.  When this
				 * happen the system panics, to the
				 * following signal will never be
				 * reached.  However... Some future
				 * vector implementation may allow a
				 * recovery from an IMP error.  If that
				 * ever happens then the following
				 * psignal should be reconsidered.
				 */
				psignal (vpd->vpd_proc, SIGKILL);
				return ;
			}

			/*
			 * vp_contextsave() succeeded so restore the saved 
			 * affinity and update the vpc_state.  update vpfree
			 * and update the state of the saved process
			 * note:  if the saved affinity does not include a 
			 *	  cpu with a vector unit, then don't touch 
			 *	  the process affinity.  The only time 
			 *	  this could occur is if the following 
			 *	  happens:
			 *	  - there is no vector processor on the 
			 *	    boot cpu
			 *	  - the vector process, while in a state 
			 *	    of VPC_LIMBO issues an instruction 
			 *	    which causes the kernel to move the 
			 *	    process to the boot cpu.  While there, 
			 *	    the process needs to wait for some 
			 *	    reason and is taken off of the run queue.  
			 *	  - another vector process takes this 
			 *	    vector cpu, forcing a vp_contextsave() 
			 *	    on this process.  Since vpc_affinity 
			 *	    is kept up to date by switch_affinity, 
			 *	    the saved affinity would not include a 
			 *	    cpu with a vector processor.
			 *	  - note:  the routine which calls 
			 *	    switch_affinity is responsible for 
			 *	    keeping a copy of the old affinity.  
			 *	    Therefore, before the user level 
			 *	    process can execute another vector 
			 *	    instruction, an affinity which includes 
			 *	    a vector processor will be in place.
			 */

			new_affinity = 
			    owning_vpc->vpc_affinity & vpmask;
			if (new_affinity != 0) {
				vpd->vpd_proc->p_affinity = new_affinity;
			}
			owning_vpc->vpc_state = VPC_SAVED;
			vpd->vpd_proc = NULL;
			set_bit_atomic (pcpu->cpu_num, &vpfree);
		}

		/*
		 * make the current process the owner of the vector processor.
		 * update the vpfree mask.  load this process's vector context 
		 * into the vector processor.
		 */

		vpd->vpd_proc = pcpu->cpu_proc;
		owning_vpc = vpd->vpd_proc->p_vpcontext;
		if (vp_contextrest (pcpu->cpu_proc) == VP_FAILURE) {
			/*
			 * vp_contextrest() will only fail if it gets a
			 * vector IMP error.  When this happen the system
			 * panics, to the following signal will never be
			 * reached.  However... Some future vector
			 * implementation may allow a recovery from an
			 * IMP error.  If that ever happens then the
			 * following psignal should be reconsidered.
			 */
			psignal (pcpu->cpu_proc, SIGKILL);
			return ;
		} else {
			owning_vpc->vpc_state =
			    VPC_LOAD;
		}
		clear_bit_atomic (pcpu->cpu_num, &vpfree);
			

		/* limit this process's affinity to this cpu */
		pcpu->cpu_proc->p_affinity = pcpu->cpu_mask;

		/*
		 * refresh the memory management registers, this is done by 
		 * writing to these regs on the scalar, this will cause the 
		 * vector copy to be updated.  and invalidate the translation 
		 * buffer
		 */
		/*
		 * This is necessary for Rigel, and so it must be
		 * necessary for Mariah.  Not necessary for Aquarius?
		 */

		if(cpu == VAX_6400) {
			mtpr (P0BR, mfpr (P0BR));
			mtpr (P0LR, mfpr (P0LR));
			mtpr (P1BR, mfpr (P1BR));
			mtpr (P1LR, mfpr (P1LR));
			mtpr (SBR, mfpr (SBR));
			mtpr (SLR, mfpr (SLR));

			mtpr (VTBIA, 0);
		}

		/* update statistics */

		/* expensive context switches per process */
		owning_vpc->vpc_expen ++ ;

		/* expensive context switches per cpu */
		vpd->vpd_ecsw += 1 ;

		/* requesting process given "ownership" of vector processor */
		vpd->vpd_success ++ ;

		/* clear the vpd_in_kernel flag */

		vpd->vpd_in_kernel = ~VPD_IN_KERNEL;
		return ;
	} else { /* vp_proc() returned PROC_MAY_NOT_HAVE_VP */

		/* update statistics */

		/* no. times process refused "ownership" (count per process) */
		current_vpc->vpc_refuse ++ ;

		/* requesting process refued "ownership" of vector processor
		 * (count per cpu)
		 */
		vpd->vpd_failed ++ ;

		/*
		 * eliminate this processor from the process's affinity mask.  
		 * this will prevent this process from being immediately 
		 * scheduled on this processor again.  note: don't need to 
		 * check for an affinity of zero here because that possibility 
		 * would have been eliminated by vp_proc().
		 * (need switch_affinity ???)
		 */

		pcpu->cpu_proc->p_affinity &= ~(pcpu->cpu_mask);
		return ;
	}

    } /* endif - kernel or user mode */
}

vp_reset (cpu_number)
int	cpu_number;
{
int	i;
/*
 * input:	cpu_number
 *		the cpu number of the cpu whose vector processor is being
 *		reset.
 *		note: this cannot be obtained from CURRENT_CPUDATA because 
 *		      when a non-boot processor is being started, 
 *		      CURRENT_CPUDATA refers to the boot cpu, but this 
 *		      routine is resetting one of the non-boot cpus.
 * description:	reset the vector processor from the scalar side and from the 
 *		vector side.  leave the vector processor disabled, but make 
 *		sure that memory management is enabled.
 * issues:	
 */

	if(cpu == VAX_6400) {	/* Rigel/Mariah only */
		/*
		 * first, reset from the scalar side.  This is done by setting
		 * the reset bit in the Vector INTerface Status Register.  This
		 * bit must be set for 100 scalar cycles.  Then it may be
		 * cleared.  It must be cleared for 100 cycles before accessing
		 * the vector processor.
		 */
		mtpr (VINTSR, VINTSR_VECTOR_MODULE_RESET);
		for (i=0; i<100; i++) ;
		mtpr (VINTSR, 0);
		for (i=0; i<100; i++) ;
	}

	/* 
	 * wait for the VP to go idle, and all memory references to complete 
	 * and all errors to be reported.  this is probably unnessary since
	 * it follows a reset, but it's unlikely that it will hurt.  also,
	 * the spec says that a VPSR_RST while VPSR_BSY is set will have 
	 * undefined results, so ...
	 */

	while (mfpr(VPSR) & VPSR_BSY) ;
	mfpr (VMAC);
	mfpr (VMAC);


	/* 
	 * now reset the vector side.  setting the VPSR_RST bit will
	 * clear the VPSR (vector processor status register), the VAER 
	 * (vector arithmetic exception register and the VCTL_CSR.
	 * Leave the VP disabled.
	 */

	mtpr (VPSR, VPSR_RST);
	mfpr(VPSR);

	if (cpu==VAX_6400) {
		/*
		 * Enable mapping by writting a 1 to VIR_LSX_TBCSR (TB control
		 * register)
		 */
		mtpr(VIADR, 0x509);
		mtpr(VIDLO, 1);

		/* Initialize cache controller: Enable memory transactions,
		 * enable cache, enable error reporting, clear errors,
		 * and flush cache.
		 * this is donw by writing a 0x138e00 to VIR_LSX_CCSR (Cache 
		 * control register)
		 */
		mtpr(VIADR, 0x520);
		mtpr(VIDLO, 0x138e00);

		/* Initialize vector controller status register; enable
		 * hard and soft error reporting.
		 * This is done by writing a 0xc0000 to VIR_VCTL_CSR (Vector
		 * controller Status)
		 */
		mtpr(VIADR, 0x489);
		mtpr(VIDLO, 0xc0000);

		/*
		 * refresh the memory management registers, this is done by
		 * writing to these regs on the scalar, this will cause the
		 * vector copy to be updated.  and invalidate the translation
		 * buffer
		 */

		mtpr (P0BR, mfpr (P0BR));
		mtpr (P0LR, mfpr (P0LR));
		mtpr (P1BR, mfpr (P1BR));
		mtpr (P1LR, mfpr (P1LR));
		mtpr (SBR, mfpr (SBR));
		mtpr (SLR, mfpr (SLR));

		mtpr (VTBIA, 0);

	} 

	CURRENT_CPUDATA->cpu_vpdata->vpd_state = VPD_ALIVE | VPD_DISABLED;
}

vp_imp ()
{
/*
 * input:	unknown at this time
 * output:	VP_IMP_FATAL or VP_IMP_NOT_FATAL
 * description:	handle implementation specific vector hardware errors.  do
 *		whatever the specific implementation has recommended and
 *		clear VPSR<IMP>.
 */
	panic ("vp_imp(): now what??");
		
	return (VP_IMP_FATAL);
}

vp_idle()
{
/*
 * description:  Idle the vector processor (if any) associated with the
 * current scalar processor.
 */
	if(vpmask & (1 << CURRENT_CPUDATA->cpu_num)) {
		while(mfpr(VPSR) & VPSR_BSY) ;
		mfpr(VMAC);
		mfpr(VMAC);
	}
}
#endif VECTORS
