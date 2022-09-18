#ifndef lint
static	char	*sccsid = "@(#)sm_machdep.c	4.2	(ULTRIX)	11/9/90";
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
 * 30-Nov-89 -- jmartin
 *	Support for coprocessor access to process virtual memory.
 *
 * 26-Oct-89 -- jmartin
 *	Add KSEG2 memory to vm_system_smget
 *
 * 10-Feb-89 -- depp
 *	New file -- machine dependent routines for SYSTEM V shared memory
 *
 */
#include "../machine/cpu.h"
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
#define PG_OF_PT_RNDUP(x) ((x) + (NPTEPG - 1) & ~(NPTEPG - 1))

sm_attach(p, sp, addr, flag)
struct proc *p;
struct smem *sp;
char *addr;
int flag;
{
	register int smindex = 0;
	register int seg;
	register int segbeg, segend;
	register int i;
	register struct pte *pte;
	struct pte tmp_pte;
	struct p_sm *psm;
	int smsize;		/* SM size in CLSIZE chunks	*/
	int s;

	XPRINTF(XPR_SM,"entering sm_attach: p 0x%x sp 0x%x",
		p, sp, 0, 0);
	smsize = clrnd(btoc(sp->sm_size));

	/* An address of 0 places the shared memory	*/
	/* into a first fit location.			*/
	if(addr == NULL){
		seg = btop(p->p_textstart) + p->p_tsize + sminfo.smbrk;
		seg = PG_OF_PT_RNDUP(seg);
		if (p->p_smcount) {
			for (smindex = 0; smindex < sminfo.smseg; smindex++) {
			       if (p->p_sm[smindex].sm_p == NULL)
				       break;
			       if (seg + smsize < p->p_sm[smindex].sm_saddr)
				       break;
			       seg = PG_OF_PT_RNDUP(p->p_sm[smindex].sm_eaddr);
			}
			if (smindex == sminfo.smseg)
				panic("sm_attach: out of segments");
		}
		segbeg = seg;
		segend = segbeg + smsize;
		if (isadsv(p, segbeg) || isadsv(p, segend - 1) || 
		    isassv(p, segbeg) || isassv(p, segend - 1)) {
			u.u_error = EINVAL;
			return(1);
		}

	} else {

		/* rev. 3 r2000 chip forbids 0 - 4Mb from being used */
		if (addr >= (caddr_t)0 && addr < (caddr_t)USRTEXT) {
			u.u_error = EINVAL;
			return(1);
		}
		/* 
		 * Check to make sure segment does not
		 * overlay any valid segments.
		 * "addr" was tested or rounded to
		 * cluster boundary in smat()
		 */
		segbeg = btop(addr);
		segend = segbeg + smsize;
		if (isatsv(p, segbeg) || isatsv(p, segend - 1) ||
		    isadsv(p, segbeg) || isadsv(p, segend - 1) ||
		    isassv(p, segbeg) || isassv(p, segend - 1) ) {
			u.u_error = EINVAL;
			return(1);
		}

		if (p->p_smcount) {
			for (smindex = 0; smindex < sminfo.smseg; smindex++) {
				if (p->p_sm[smindex].sm_p == 
				    (struct smem *) NULL)
					break;
				if (segend <= p->p_sm[smindex].sm_saddr)
					break;
			}
			if  (smindex == sminfo.smseg)
			        panic ("sm_attach: out of segments 2");
				
			if (smindex && 
			    (segbeg < p->p_sm[smindex - 1].sm_eaddr)) {
				u.u_error = EINVAL;
				return(1);
			}
		}
	}

	sm_ins_psm(p, smindex);    /* create hole in p_sm array */
	psm = &p->p_sm[smindex];
	psm->sm_p = sp;
	psm->sm_saddr = segbeg;
	psm->sm_eaddr = segend;

	newptes(p, segbeg, segend - segbeg);

	/* 
	 * default PTE protection is PROT_KW; if any attaches occur URKR, then
	 * change protection to URWR., and let tlbmod() handle the writability
	 * of the page.  Note: this default for the segment will not return to 
	 * PROT_KW, though that's OK as it was purely a performance
	 * micro-optimization.
	 */
	s = splimp();
	smp_lock(&lk_smem, LK_RETRY);
	if (flag & SM_RDONLY) {
		psm->sm_pflag = PROT_URKR;
		if ((pte = sp->sm_ptaddr)->pg_prot != PROT_URKR) {
			for (i = 0; i < smsize ; pte++,i++) {
				*(int *)& tmp_pte = *(int *) pte;
				tmp_pte.pg_prot = PROT_URKR;
				if (tmp_pte.pg_m) {
					tmp_pte.pg_swapm = 1;
					tmp_pte.pg_m = 0;
				}
				*(int *) pte = *(int *) &tmp_pte;
			}
		}
	} else
		psm->sm_pflag = PROT_UW;

	psm->sm_link = sp->sm_caddr;
	sp->sm_caddr = p;
	sp->sm_count++;
	sp->sm_ccount++;
	smp_unlock(&lk_smem);
	(void)splx(s);

	u.u_r.r_val1 = ctob(segbeg);
	SM_PSM_UPDATE(p);
        u.u_smsize = p->p_smsize;
	XPRINTF(XPR_SM, "exit sm_attach: addr = 0x%x smbeg = 0x%x smend 0x%x",
		u.u_r.r_val1, p->p_smbeg, p->p_smend, 0);
	return(0);
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
	register int spfn;
	register struct pte *upte;
	register struct smem *sp;
	register int smid;
	register int nocache;
	extern struct smem *smconv();

	XPRINTF(XPR_SM,"enter vm_system_smget: sva 0x%x size %d mode %o",
		sva, size, mode, 0);
	if (IS_KUSEG(sva) ||
	    !isclbnd(sva) || !isclbnd(size) || (size < CLBYTES)) {
		u.u_error = EINVAL;
		return (-1);
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
		if (IS_KSEG2(sva)) {
			struct pte *spte, *espte;
			
			for (spte = svtopte(sva), espte = spte+num_ptes;
			     spte < espte;
			     ++spte) {
				if (spte->pg_v == 0) {
					u.u_error = EINVAL;
					return(-1);
				}
			}
			for (spte = svtopte(sva); spte < espte; ++spte,++upte)
				Hard(upte) =  Hard(spte) & PG_PFNUM
#ifdef NOMEMCACHE
						| PG_N
#endif NOMEMCACHE
							| PG_UW | PG_V | PG_M;
			return smid;
		} else if (IS_KSEG0(sva)) {
			spfn = K0_TO_PHYS(sva) >> PGSHIFT;
			nocache = 0;
		} else if (IS_KSEG1(sva)) {
			spfn = K1_TO_PHYS(sva) >> PGSHIFT;
			nocache = 1;
		} else {
			u.u_error = EINVAL;
			return(-1);
		}
		while (num_ptes--) {
			upte->pg_v = 1;
			upte->pg_m = 1;
#ifdef NOMEMCACHE
			upte->pg_n = 1;
#else  NOMEMCACHE
			upte->pg_n = nocache;
#endif NOMEMCACHE
			upte->pg_prot = PROT_UW;
			(upte++)->pg_pfnum = spfn++;
		}
		XPRINTF(XPR_SM,"vm_system_smget: first pte 0x%x *pte 0x%x",
			sp->sm_ptaddr, *(int *) sp->sm_ptaddr,0,0);
	}
	return (smid);
}

/*
 * sm_clear_dev_tlbs is called by any subsystem--only pageout at this
 * time--which clears the valid bit of a shared memory PTE.  Its
 * function is to synchronize the invalidation with devices which may
 * have cached a translation to the shared memory page.  Such a device
 * is assumed to be bound to a process, which in turn contains a pointer
 * to a function supplied by the device driver.  sm_clear_dev_tlbs
 * operates by examining the function pointer of each process attached
 * to the shared memory and executing it if non-NULL.  The function
 * returns when the device is done with the shared memory page.
 */
sm_clear_dev_tlbs(shm, vpn)
	struct smem	*shm;	/* shared memory segment */
	int		vpn;	/* virtual page number in shared memory */
{
	struct proc	*p;	/* a process attached to shm */
	int		smi;	/* index of shm data in p */

	/* Examine each process attached to shm for a service function. */
	for (p=shm->sm_caddr; p; p=p->p_sm[smi].sm_link) {
		/*
		 * If the process has a service function, translate the
		 * shared memory vpn to a process space vpn and call
		 * the function.
		 */
		if (p->p_dev_VM_maint)
			(*p->p_dev_VM_maint)(PDEVCMD_ONE, smptov(p, shm, vpn));

		/*
		 * This is just loop control:  finding the per-process
		 * information for the shared memory segment so that
		 * we can move on to the next attached process.
		 */
		for (smi = 0; smi < sminfo.smseg && p->p_sm[smi].sm_p; smi++)
			if (p->p_sm[smi].sm_p == shm)
				break;
		if (smi==sminfo.smseg || p->p_sm[smi].sm_p==(struct smem *)NULL)
			panic("sm_clear_dev_tlbs: missing proc-to-shm pointer");
	}
	return !NULL;
}
