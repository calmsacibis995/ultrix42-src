#ifndef lint
static	char	*sccsid = "@(#)sm_machdep.c	4.2  (ULTRIX)        9/6/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 ************************************************************************/
/*
 *
 *   Modification History:
 *
 * 4 Sep 90 -- dlh
 *	added include of vectors.h
 *
 * 31 Aug 89 -- gg
 *	Initialise u_odsize and u_ossize before calling ptexpand
 *
 * 24 Jul 89 -- jmartin
 *	Change interface to ptexpand.
 *
 * 5-May-89 -- Adrian Thoms
 *	Added code to check shared mem allocations against
 *	user page table limit maxprocptes
 *
 * 10-Feb-89 -- depp
 *	New file -- machine dependent routines for SYSTEM V shared memory
 *
 */
#include "../machine/mtpr.h"
#include "../machine/pte.h"

#include "../h/param.h"		/* includes types.h & vax/param.h */
#include "../h/dir.h"
#include "../h/user.h"		/* includes errno.h		*/
#include "../h/proc.h"
#include "../h/seg.h"
#include "../h/cmap.h"
#include "../h/vm.h"

#include "../h/kmalloc.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#ifdef vax
#include "../machine/vectors.h"
#endif vax

extern struct sminfo sminfo;


/*
 * This routine does the actual attach of the SMS to the process' address
 * 	space.
 *
 * input parameters:
 *	p	proc pointer
 *	sp	smem pointer
 *	addr	addr parameter from syscall
 *	flag	flag parameter from syscall
 *
 * return codes:
 *	0	Normal return, attach was successful
 *	1	error return, u_error set
 *
 * panics:
 *	"sm_attach: out of segments" -- Upon entering routine,
 *	we are guaranteed that there is enough room in the sm_p array
 *	to store the per process attach information.  However, we find
 *	this not to be true.
 * 
 */
sm_attach(p, sp, addr, flag)
struct proc *p;
register struct smem *sp;
char *addr;
int flag;
{
	struct p_sm *psm;
	register int i, smindex = 0;
	register int *seg, *sm;
	register int segbeg, segend;
	int pflag;
	struct pte *ptaddr;
	size_t osms;		/* SM size before this attach */
	int smsize;		/* SM size in CLSIZE chunks */
	int p0lr, p1lr;
	int s;

	smsize = clrnd(btoc(sp->sm_size));

	/* An address of 0 places the shared memory	*/
	/* into a first fit location.			*/
	if (addr == NULL){
		seg = ((int *)mfpr(P0BR)) + u.u_tsize +	
			u.u_dsize + sminfo.smbrk;
		seg = (int *)clrnd((int)seg);
		segbeg = (int *)seg - (int *)mfpr(P0BR);

		if (p->p_smcount) {
			for ( ; smindex < sminfo.smseg; smindex++) {
			       if (p->p_sm[smindex].sm_p == NULL)
				       break;
			       if (segbeg + smsize < p->p_sm[smindex].sm_saddr)
				       break;
			       segbeg = p->p_sm[smindex].sm_eaddr + CLSIZE;
			}
			if (smindex == sminfo.smseg)
				panic("sm_attach: out of segments");
		}
		segend = segbeg + smsize;

	} else {

			/* Check to make sure segment does not	*/
			/* overlay any valid segments.		*/

			/* "addr" was tested or rounded to	*/
			/* cluster boundary in code above	*/
		segbeg = btop(addr);
		if (isatsv(p, segbeg) ||  vtodp(p, segbeg) < p->p_dsize ) {
			u.u_error = EINVAL;
			return(1);
		}

		segend = segbeg + smsize;
		if (p->p_smcount) {
			for ( ; smindex < sminfo.smseg; smindex++) {
				if (p->p_sm[smindex].sm_p ==(struct smem *)NULL)
					break;
				if (segend <= p->p_sm[smindex].sm_saddr)
					break;
			}
			if (smindex == sminfo.smseg)
			        panic ("sm_attach: out of segments 2");
				
			if (smindex && 
			    (segbeg < p->p_sm[smindex - 1].sm_eaddr)) {
				u.u_error = EINVAL;
				return(1);
			}
		}

	}

	/* Need to increase the size of the page table,	*/
	/* allocate and clear.				*/
	p0lr = u.u_pcb.pcb_p0lr & ~AST_CLR;
	p1lr = u.u_pcb.pcb_p1lr & ~PME_CLR;
	{	/* Check that limit for p0 + p1 page tables not exceeded */
		extern unsigned maxprocptes; /* param.c */

		if (segend + u.u_ssize + HIGHPAGES > maxprocptes) {
			u.u_error = ENOMEM;
			return(1);
		}
	}
	u.u_odsize = p->p_dsize;
	u.u_ossize = p->p_ssize;
	u.u_osmsize = p->p_smsize;
	if (p->p_smbeg == 0 || p->p_smbeg > segbeg)
		p->p_smbeg = segbeg;
	if (p->p_smend < segend){
		p->p_smend = segend;
		p->p_smsize = u.u_smsize = segend - u.u_tsize - u.u_dsize;
	}

	if (p0lr < segend) {
		/* Compute the end of the text+data regions and	*/
		/* the beginning of the stack region in the	*/
		/* page tables, and expand the page tables if	*/
		/* necessary.					*/
		register struct pte *p0, *p1;
		register int change;

		p0 = u.u_pcb.pcb_p0br + p0lr;
		p1 = u.u_pcb.pcb_p1br + p1lr;
		if ((change = segend - p0lr) > p1 - p0) {
			SM_UNLOCK(sp);	/* avoid deadlock (yuch) */
			ptexpand(clrnd(ctopt(change - (p1 - p0))), 0);
			SM_LOCK(sp);
		}
		seg = (int *)p->p_p0br + p0lr;
		for (i = 0; i < change; i++)
			*seg++ = 0;

		setp0lr(segend);
	}

	/* Copy the shared memory segment in	*/
	sm = (int *)sp->sm_ptaddr;
	seg = (int *)p->p_p0br + segbeg;
	pflag = ((flag & SM_RDONLY) ? PG_URKW : PG_UW);
	sm_ins_psm(p, smindex);    /* create hole in p_sm array */
	psm = &p->p_sm[smindex];
	psm->sm_p = sp;
	psm->sm_saddr = segbeg;
	psm->sm_eaddr = segend;
	psm->sm_pflag = pflag;

	s = splimp();
	smp_lock(&lk_smem, LK_RETRY);
	for (i = 0; i < smsize; i++)
		if (((struct fpte *)sm)->pg_fod)
			*seg++ = *sm++ | pflag;
		else
			*seg++ = *sm++ | PG_ALLOC | pflag;
	psm->sm_link = sp->sm_caddr;
	sp->sm_caddr = p;
	sp->sm_count++;
	sp->sm_ccount++;
	smp_unlock(&lk_smem);
	(void)splx(s);

	u.u_r.r_val1 = ctob(segbeg);
	return(0);
}

/*
 * This routine is called by smccdec to propogate any "dirty" PTEs on user
 * level back to the SMS global page table
 *
 * this process is detaching from this SMS (due to exit, swapout, 
 * or detach) so if the modify bit is on for any of the SMS PTEs 
 * we must be sure to propogate it back to the primary PTEs before 
 * the copy-PTEs are zeroed.  Note: Since the primary PTEs are only
 * table entries and are not used by the hardware, an TBIS is not
 * required.
 *
 */

sm_cpdirty(p,sp)
	register struct proc *p;
	register struct smem *sp;
{
	register i;
	register struct pte *pte;
	register int smsize;

	for (i = 0; i < sminfo.smseg; i++) {
		if (p->p_sm[i].sm_p == sp){
			break;
		}
	}
	if (i == sminfo.smseg)
		panic("smccdec: smseg");

	smsize = clrnd(btoc(sp->sm_size));
	pte = p->p_p0br + p->p_sm[i].sm_saddr;
	for (i = 0; i < smsize; i += CLSIZE, pte += CLSIZE){
		register int j;
		if (dirtycl(pte)){
			*(int *)(sp->sm_ptaddr + i) |= PG_M;
			distcl(sp->sm_ptaddr + i);
		}
		for (j = 0; j < CLSIZE; j++)
			*(int *)(pte+j) = 0;
	}
}


/* 
 * VM_SYSTEM_SMGET() -- kernel level smget() routine for kernel/user mapping
 *
 *   This routine MUST only be used to map static kernel page aligned data
 *   structures into an application's address space.  This routine will
 *   perform a special version of the shmget() system call.  The assumptions
 *   are as follows
 *       1. The system virtual address space, and the underlaying PTEs are
 *          fully static (i.e., DO NOT use km_alloc'd sva)
 *       2. The caller validates that the application is permitted to 
 *          create this Shared Memory Segment (SMS)
 *       3. Once the application has requested the driver to "get" the 
 *          SMS (via an IOCTL I assume), full SMS semantics are in effect.
 *          This means that the application must now call the shmat() system 
 *          call to actually attach to this segment.  Also, the SMS can
 *          be detached, reattached, and (upon the last detach) removed.
 *       4. The beginning system virtual address and size are page cluster
 *          aligned.  It is up to the calling routine to insure that the 
 *          protocol for finding a non-page aligned structure is in place for
 *          the application.
 *
 *   INPUT PARAMETERS:
 *          sva        Beginning system virtual address to be double mapped
 *          size       Size of the segment in bytes
 *
 *               Note: Both must be page aligned
 *
 *          mode       This is the protection field.  It uses file type
 *                     protection (user, group, others) for read/write.
 *                     Note that execute permission is not used.
 *
 *   RETURN PARAMETER:
 *          -1         Error setting up SMS, u.u_error has been set to
 *                     indicate error condition
 *          smid       Shared memory identifier, to be passed back to the
 *                     application.  This is the normal return value.
 */

vm_system_smget(sva, size, mode) 
int sva;
int size;
int mode;
{
	register int num_ptes = btop(size);
	register struct pte *spte;
	register struct pte *upte;
	register struct smem *sp;
	register int smid;
	register int nocache;
	extern struct smem *smconv();

	XPRINTF(XPR_SM,"enter vm_system_smget: sva 0x%x size %d mode %o",
		sva, size, mode, 0);
	if (((sva & VA_SPACE) != VA_SYS) || 
	    !isclbnd(sva) || !isclbnd(size) || (size < CLBYTES)) {
		u.u_error = EINVAL;
		return (-1);
	}

	spte = svtopte(sva); 
	while (num_ptes--) {
		if ((spte++)->pg_v == 0) {
			u.u_error = EINVAL;
			return(-1);
		}
	}

	mode &= 0777;
	if ((smid = smget(sva, size, IPC_CREAT | IPC_SYSTEM | mode)) == -1) {
		return(-1);
	}

	/* 
	 * get the smem pointer, as it has just been allocated or 
	 * retrieved, it must exist.
	 */
	if ((sp = smconv(smid, 0)) == NULL)
		panic("vm_system_smget: invalid SMS");

	/* if first time, then fill */
	upte = sp->sm_ptaddr;
	if (upte->pg_v == 0) {
		num_ptes = btop(size);
		spte = svtopte(sva);
		while (num_ptes--) 
			*(int *) (upte++) = 
				PG_V | (*(int *) (spte++) & PG_PFNUM);
		XPRINTF(XPR_SM,"vm_system_smget: first pte 0x%x *pte 0x%x",
			sp->sm_ptaddr, *(int *) sp->sm_ptaddr,0,0);
	}
	return (smid);
}

