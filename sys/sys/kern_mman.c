#ifndef lint
static char *sccsid = "@(#)kern_mman.c	4.6	(ULTRIX)	9/18/90";
#endif

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
/*
 *
 *   Modification history:
 *
 * 4 Sep 90	dlh
 *	added include of vectors.h
 *
 *  13 Aug 90 sekhar
 *	made mmap and munmap code common to both vax and mips. It has
 *	been tested on mips. However on vax it exists only as latent 
 *	support.
 *
 *	made get_mmap_alignment common to both vax and mips. Tested on
 *	both vax and mips.
 *
 *  23 Apr 90 sekhar
 * 	Added mmap,munmap and get_mmap_alignment for memory mapped 
 *	character devices(mips only).
 *      
 *  11 Dec 89 jaa
 *	change dynamic swap to account for swap up front (ala v3.1) 
 * 	but actually do the allocation only when pushing the page/process
 *
 * 13 Oct 89 - gmm
 *	smp changes for MIPS. Make nofault mp safe
 *
 * 1 Sep 89 -- jaa
 *	obreak() needs signed variables
 *
 * 12 Jun 89 -- bp
 *	Removed old kernel memory allocator debug code.
 *
 * 12 Jun 89 -- gg
 *	Dynamic swap changes in routine obreak() and grow.
 *	changed calls swpexpand() to dmexpand().
 *
 * 31 Aug 88 -- jmartin
 *	Use lk_text to make SMP safe textlock/tunlock functions.
 *
 * 25 Jul 88 -- jmartin
 *	Add the SMP lock lk_cmap.  Replace "mtpr(TBIA, 0)" with
 *	"newptes".  Replace various "|=" and "&= ~" operations with the
 *	SMP versions, SET_P_VM and CLEAR_P_VM.  Replace the vmtest suite
 *	with the one from vHe.
 *
 * 24 Feb 86 -- depp
 *	Added new system call "mprotect".
 *
 * 12-Feb-86 -- jrs
 *	Added calls to tbsync() for mp translation buffer control
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 18 Sep 85 -- depp
 *	Added "plock" system call
 *
 * 26 Jul 85 -- depp
 *	Put check into ovadvise to insure that if the calling process
 *	has shared memory segments attached, that an error is returned
 *	unless the input parameter indicates normal processing.
 *
 * 19 Jul 85 -- depp
 *	Corrected calculation in first arg of smexpand in obreak. 
 *
 * 09 Apr 85 -- depp
 *	Added System V Shared memory support to obreak
 *
 */

#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/gnode.h"
#include "../h/seg.h"
#include "../h/acct.h"
#include "../h/wait.h"
#include "../h/vm.h"
#include "../h/text.h"
#include "../h/file.h"
#include "../h/vadvise.h"
#include "../h/cmap.h"
#include "../h/trace.h"
#include "../h/mman.h"
#include "../h/conf.h"
#include "../h/lock.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#include "../h/cpudata.h"
#ifdef vax
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/vectors.h"
#endif vax

extern int maxssiz; 

sbrk()
{

}

sstk()
{

}

getpagesize()
{

	u.u_r.r_val1 = NBPG * CLSIZE;
}

/*
 * Purpose: This is for use by the mmap system call.  A user can
 *	    can specify an address va (with option MAP_FIXED) where
 *	    the mapping is to be done. This routine specifies the 
 *	    minimum alignment required for such a va.
 */

get_mmap_alignment()
{
	return(SMLBA);
}

/*
 * Purpose: Entry point for the mmap system call.
 *
 * Inputs:
 *     addr     address to begin mapping 
 *     len      len bytes to map
 *     prot     PROT_READ or PROT_WRITE
 *     flags    MAP_FIXED and/or MAP_SHARED
 *     fd       file descriptor of character device 
 *     off      offset into device 
 *
 * Returns:
 *     address where device memory is mapped.
 *
 * Panics:
 *     panic("mmap: invalid SMS").
 *  
 */

smmap()
{
	register struct a {
		caddr_t addr; 
		size_t len;   
		int prot;     
		int flags;   
		int fd;       
		int off;      
	} *uap = (struct a *) u.u_ap;

        extern struct file *getgnode();
	extern struct smem *smconv(), *ipcget(), smem[];
	extern struct sminfo sminfo;
	extern int nulldev(), nodev();

	register struct pte *upte;
	register int i;
        register int (*mapfun)();
	register struct file *fp;
	register struct gnode *gp;
        dev_t dev;

	int saveaffinity, s, devpfn, smid, mapadr, st;
	int shmflg = 0;
	size_t len;
	key_t  key;
	struct smem *sp;
#ifdef vax
        /* mmap not yet supported on vax. hence return an error */
        u.u_error = EINVAL;
        return;
#endif

	if (!(uap->flags & MAP_SHARED || uap->flags & MAP_FIXED)) {
		u.u_error = EINVAL;
		return;
	}

	if (!(uap->prot & PROT_READ || uap->prot & PROT_WRITE)) {
		u.u_error = EINVAL;
		return;
	}

	if (uap->off & CLOFSET) {
		u.u_error = EINVAL;
		return;
	}

	/*
	 * 	We just check here for the restriction placed by 
	 *	mmap on the user. Other checks for validity of 
	 *	addresses are performed when the address is 
	 *	actually attached.
	 */

	if ((uap->flags & MAP_FIXED) && 
	    ((u_int)uap->addr & (get_mmap_alignment() - 1))) {
		u.u_error = EINVAL;
		return;
	}

	if ((fp = getgnode(uap->fd)) == NULL) 
		return;

	if ((uap->prot & PROT_WRITE) && !(fp->f_flag & FWRITE)) {
		u.u_error = EACCES;
		return;
	}

	if ((uap->prot & PROT_READ) && !(fp->f_flag&FREAD)) {
		u.u_error = EACCES;
		return;
	}

	gp = (struct gnode *)fp->f_data;

	/*
	 *	allow only character devices to be mapped
	 */

	if ((gp->g_mode & GFMT) != GFCHR) { 
		u.u_error = EINVAL;
		return;
	}
	dev = gp->g_rdev;

	mapfun = cdevsw[major(dev)].d_mmap;
	/*
	 *	device cannot  be memory mapped if :
	 *	    d_mmap = NULL    => no entry point in cdevsw table
	 *	    d_mmap = nodev   => nodev always returns failure
	 *	    d_mmap = nulldev => nulldev returns success (i.e.0)
	 *		     no good for mmap since we expect a pfn 
	 * 		     which is filled in page tables.
	 */
	
	if (mapfun == NULL || mapfun == nodev || mapfun == nulldev) {
		u.u_error = ENODEV;
		return;
	}

	len = ctob(clrnd(btoc(uap->len)));

	/*
	 * 	check to see if that the device memory being mapped
	 *	is within range. The starting pfn of the memory is
	 *	used as key for shared memory segments. Do this 
	 * 	outside the loop for performance.
	 */

        if ((devpfn = (*mapfun)(dev, uap->off, uap->prot)) == -1) {
		u.u_error = ENXIO;
		return;
	}

        for (i=NBPG; i<len; i += NBPG) {
                if ((*mapfun)(dev, uap->off+i, uap->prot) == -1) {
                        u.u_error = ENXIO;
                        return;
		}
	}

	key = (unsigned) ptob(devpfn) + uap->off;

	/*
	 *	The section of code from here till end is not totally 
	 *	mp-safe. However some locks have been used to make it
	 *	easier to make the code mp-safe in future. This code
	 *	will have to be redesigned. But for now single thread
	 *	the code by running only on the boot cpu.
	 */

        saveaffinity = switch_affinity(boot_cpu_mask);

        s = splimp();
	smp_lock(&lk_smem, LK_RETRY);

	/*
	 *	if another process already has created a segment
	 *	with key but different length than what we
	 *	are trying to map, then create a new private segment.
	 */

	if (   ((sp = ipcget(key, (gp->g_in.gn.gc_mode & 0777) | IPC_SYSTEM,
		     &smem[0], sminfo.smmni, sizeof(*sp), &st)) != NULL)   
	    && (sp->sm_size != len) )
		key = IPC_PRIVATE;

	smp_unlock(&lk_smem);
	(void)splx(s);

	if ((smid = smget(key,len,
			  (gp->g_in.gn.gc_mode & 0777)
			  | IPC_CREAT | IPC_SYSTEM | IPC_MMAP )) < 0) {

		/*
		 * 	map share memory specific errors into errors
		 *	which have an equivalent in the mmap interface.
		 */

		if (u.u_error == ENOSPC) 
			u.u_error = ENOMEM;
		goto error;
	}

	if ((sp = smconv(smid, 0)) == NULL)
		panic("mmap: invalid SMS ");

	/*
	 *	If a shared memory segment has just been created by 
	 *	initialize the page table entries.
	 *	Must be done under SMLOCK to lock out the page daemon 
	 *	and other processes which may use the same segment.
	 */
	
	SM_LOCK(sp);
	upte = sp->sm_ptaddr;
        if (upte->pg_v == 0) 

	    /*
	     *	    pte set up as follows:
   	     *		non-cacheable for i/o space.
	     *	        default protection is user write and kernel write
	     *	        smat() changes the prot if PROT_READ is specified.   
	     */

	    for (i=0; i < len ; i += NBPG, upte++)
#ifdef mips
		Hard(upte) = 
		 (*mapfun)(dev, uap->off+i, uap->prot) << PTE_PFNSHIFT
		| PG_UW | PG_M | PG_V | PG_N ; 
#else
		;
		/* NEED TO CODE THIS VAX */
#endif

	SM_UNLOCK(sp);
					
	/* attach to the process address space */
	if (!(uap->prot & PROT_WRITE))
		shmflg |= SM_RDONLY;
	if (uap->addr && !(uap->flags & MAP_FIXED))
		shmflg |= SHM_RND;
	if ((mapadr = smat(smid, uap->addr, shmflg)) == -1) { 
	    /*
	     *        if we have run out of shared memory segments then
	     *	      treat this error as running out of system resources
	     */
	    if (u.u_error == EMFILE) 
		u.u_error = ENOMEM;
	}
	else 
	    u.u_r.r_val1 = mapadr;
error:
        switch_affinity(saveaffinity);
}
 
mremap()
{

}

/*
 * Purpose: Entry point for the system call munmap. This maps
 *	    device memory(NOTE: NOT FILES) into user address space.
 *
 * Input:
 *     addr - starting address to unmap.
 *     len  - no of bytes to unmap starting at addr.
 * 
 * Returns:
 *     0 if no error or -1 on error.
 */ 

munmap()
{
	register struct a {
		caddr_t	addr;
		int	len;
	} *uap = (struct a *) u.u_ap;

	register struct p_sm *psm;
	struct p_sm *fpsm,*lpsm;
	int saveaffinity;
	
#ifdef vax
        /* mumap not yet supported on vax. hence return an error */
        u.u_error = EINVAL;
        return;
#endif

	if ((int)uap->addr & CLOFSET) {
		u.u_error = EINVAL;
		return;
	}

	if (sm_get_smlist(u.u_procp, btop(uap->addr),
		          btop(uap->addr) + clrnd(btoc(uap->len)),
			  &fpsm, &lpsm) == -1) {
		u.u_error = EINVAL;
	  	return;
	}
	
	/*  	until we make it mp-safe run the code only on boot cpu.  */

        saveaffinity = switch_affinity(boot_cpu_mask);

	/*
	 * 	only segments created because of mmap system call
	 * 	can be unmapped by this system call.
	 */

	for (psm = fpsm; psm <= lpsm; psm++)
	    if (!(psm->sm_p->sm_perm.mode & IPC_MMAP)) {
		u.u_error = EINVAL;
		return;
	    }

	for (psm = fpsm; psm <=  lpsm ; psm++) 
		/* 
		 * SHMT_MMAP => detach segment created by mmap call.
		 */ 
		smdt(ptob(psm->sm_saddr), SHMT_MMAP);
        switch_affinity(saveaffinity);
}


munmapfd(fd)
{
#ifdef notdef
	register struct fpte *pte;
	register int i;

	for (i = 0; i < u.u_dsize; i++) {
		pte = (struct fpte *)dptopte(u.u_procp, i);
		if (pte->pg_v && pte->pg_fod && pte->pg_fileno == fd) {
			*(int *)pte = (PG_UW|PG_FOD);
			pte->pg_fileno = PG_FZERO;
		}
	}
	u.u_pofile[fd] &= ~UF_MAPPED;
#endif
	
}

/*
 * Function:
 *
 *	mprotect -- system call
 *
 * Function description:
 *
 *	This system call permits a user process to change the protection
 *	of a block of the caller's virtual memory.  Currently, only private
 *	data memory can be so protected.  Protection is performed on a cluster
 *	and a cluster size may increase in future ULTRIX versions.
 *
 * Interface:
 *
 *	mprotect (addr, len, prot)
 *	  caddr_t addr;		address of the beginning of the block
 *	  int len;	  	length of the block in bytes
 *	  int prot;	  	protection flags (see <mman.h> for values )
 *
 *	"addr" must fall on a page cluster boundary.
 *	"len" will be rounded up to a cluster boundary.
 *
 * Return Value:
 *
 *	> 0	Normal -- size of protected block (in bytes)
 *	< 0	Error - Errno set
 *
 * Error Handling:
 *
 *	ERRNO value	Description
 *
 *	  
 *	  EALIGN	Input address not cluster aligned
 *	  EINVAL	Invalid protection mask (parameter "prot")
 *	  EACCES	Memory block not within private data space
 *
 * Side Effects:
 *
 *	If the user handles a SIGBUS signal, the signal handler MUST either
 *	abort the process or correct the condition that caused the protection
 *	fault (SIGBUS).  If some corrective action is not taken, an infinite
 *	loop will result.
 *
 * Machine Dependencies
 *
 *      Since the MIPS architecture does not care about PTE layout and 
 *      since we are using a simplified VAX protection scheme, the only
 *      MIPS dependency is that PG_URKW maps to PG_URKR
 *      
 */

#define PROT_MASK	(PROT_READ | PROT_WRITE | PROT_EXEC)
#ifdef mips
#define PG_URKW  PG_URKR
#endif mips

/* used to map "prot" parameter to VAX protection */
u_int vaxmprot[] = {
	PG_KW,		/* No Access */
	PG_URKW,	/* Read only */
	PG_UW,		/* Write only */
	PG_UW,		/* Read/Write */
	PG_URKW,	/* Execute */
	PG_URKW,	/* Execute/Read */
	PG_UW,		/* Execute/Write */
	PG_UW,		/* Execute/Read/Write */
};

mprotect()
{
	register struct a {
		caddr_t	addr;
		int	len;
		int	prot;
	} *uap = (struct a *)u.u_ap;
	register int npages;			/* number of full pages */
	register struct pte *pte;		/* pointer to page table */
#ifdef mips
	register int ptemask = 0;
#endif mips
	register u_int vaxprot;			/* VAX protect for block */
	int v;				        /* VA of beginning of block */
#ifndef mips
	int ve;					/* VA of end of block */
#endif mips
	register int i;
	caddr_t addr = uap->addr;		/* "addr" */
	struct proc *p = u.u_procp;		/* proc pointer */
	int nbytes;				/* number of bytes protected */
	int s;

	/* is address on cluster boundary? */
	if (!isclbnd(addr)) {
		u.u_error = EALIGN;
		return;
	}

	/* Retrieve VAX protection code */
	if (uap->prot & ~PROT_MASK) {
		u.u_error = EINVAL;
		return;
	}
	vaxprot = vaxmprot[uap->prot];
#ifdef mips
	/* Create mask, if "no access", then reset valid bit */
	if (uap->prot == NULL)
		ptemask = ~(PG_V | PG_PROT);
	else
		ptemask = ~PG_PROT;
#endif mips

	/* convert to pages on cluster boundary */
	npages = clrnd(rbtop(uap->len));
	nbytes = (int) ptob(npages);

	/* calulate first and last address of block */
	v = btop(addr);
#ifndef mips
	ve = v + npages - 1;

	/* Only permit user to change protection on a private data block */
	if (isadsv(p, v) && (vtodp(p,ve) < p->p_dsize))
#else  mips
	if (isadsv(p, v))
#endif mips
		pte = dptopte(p, vtodp(p, v));
	else {
		u.u_error = EACCES;
		return;
	}

	/* change protection on PTEs */	
	s = splimp();
	smp_lock(&lk_cmap, LK_RETRY);
	for (i=0; i<npages; i++) {
#ifndef mips
		(pte+i)->pg_prot = 0;
#else mips
		/*
		 * if we are decreasing protection and page is dirty
		 *	set swapm bit to insure swap backing
		 *	and clear modify bit for prot checking (tlbmod)
		 */
		if (vaxprot < (*(u_int *) (pte+i) & PG_PROT) &&
		   (*(u_int *) (pte+i) & PG_M) == PG_M) {
			*(u_int *) (pte+i) |= PG_SWAPM;
			*(u_int *) (pte+i) &= ~PG_M;
		}
		*(u_int *) (pte+i) &= ptemask;
#endif mips
		*(u_int *) (pte+i) |= vaxprot;
	}
	newptes(p, v, npages);
	smp_unlock(&lk_cmap);
	(void) splx(s);
	u.u_r.r_val1 = nbytes;
	return;
}

madvise()
{

}

mincore()
{

}

int usermsg = 0;

/* BEGIN DEFUNCT */
obreak()
{
	struct a {
		char	*nsiz;
	};
	register int n, d;
	register struct proc *p = u.u_procp;
	char *msg;

	/*
	 * set n to new data size
	 */
#ifdef mips
	n = btoc(((struct a *)u.u_ap)->nsiz - p->p_datastart);
#endif mips
#ifdef vax
	n = btoc(((struct a *)u.u_ap)->nsiz) - ctos(u.u_tsize)*stoc(1);
#endif vax
	if(n < 0)
		n = 0;
	if ((d = clrnd(n - u.u_dsize)) == 0) 
		return;
	if(d > 0) {
		if (chksize((u_int)u.u_tsize, (u_int)u.u_dsize, 
			    (u_int)d, (u_int)u.u_ssize)) {
			msg = "max data size exceeded";
			goto fail;
		}
	}

#ifdef mips
	if(n) {
		/*
		 * check for data segment growing into text or stack
		 */
		if ((p->p_datastart < p->p_textstart) &&
		    ((unsigned)(ctob(n) + p->p_datastart) > p->p_textstart)) {
			msg = "data would grow into text";
			goto fail;
		}
		if ((unsigned)(ctob(n) + p->p_datastart) >
		    ((USRSTACK+HIGHPAGES*NBPG) - (p->p_stakpt *NPTEPG*NBPG))) {
			msg = "data would grow into stack";
			goto fail;
		}
	}
#endif mips
#ifdef vax
	if(p->p_smcount){
		if(p->p_sm == (struct p_sm *) NULL)
			panic("obreak: p_sm");
		if(d > 0){
			if((u.u_tsize + u.u_dsize + d) > p->p_smbeg){
				msg = "data would grow into shared memory";
				goto fail;
			}
			if(dmexpand(&(u.u_procp->p_dmap), u.u_dsize, 
					   u.u_dsize + d,CDATA) == 0){
				swfail_stat.data_ex_fail++;
				msg = "out of swap space";
				goto fail;
			}
		}

		smexpand(d);
		p->p_smsize -= d;
		u.u_smsize -= d;
		return;
	}
#endif vax

	if(dmexpand(&(u.u_procp->p_dmap), u.u_dsize, u.u_dsize+d, CDATA) == 0){
		swfail_stat.data_ex_fail++;
		msg = "out of swap space";
		goto fail;
	}
	expand((int)d, 0);
	return;

fail:
	if(! u.u_error)
		u.u_error = ENOMEM;
	if (usermsg)
		uprintf("%s, pid %d, proc %s\n", msg, p->p_pid, u.u_comm);
	return;
}

int	both;

ovadvise()
{
	register struct a {
		int	anom;
	} *uap;
	register struct proc *rp = u.u_procp;
	int oanom;
	register struct pte *pte;
	register struct cmap *c;
	register int i;
	int s;

#ifdef lint
	both = 0;
#endif
	uap = (struct a *)u.u_ap;

	/* begin SHMEM */
	/* currently advise and shmem are incompatable */
	if (rp->p_smcount && uap->anom != VA_NORM) {
		u.u_error = EINVAL;
		return;
	}
	/*  end SHMEM */

	trace(TR_VADVISE, uap->anom, u.u_procp->p_pid);
	s = splimp();
	smp_lock(&lk_p_vm, LK_RETRY);
	oanom = (rp->p_vm & SUANOM) && (uap->anom != VA_ANOM);
	rp->p_vm &= ~(SSEQL|SUANOM);
	switch (uap->anom) {

	case VA_ANOM:
		rp->p_vm |= SUANOM;
		break;

	case VA_SEQL:
		rp->p_vm |= SSEQL;
		break;
	}
	smp_unlock(&lk_p_vm);
	if (oanom || uap->anom == VA_FLUSH) {
		for (i = 0; i < rp->p_dsize; i += CLSIZE) {
			pte = dptopte(rp, i);
			smp_lock(&lk_cmap, LK_RETRY);
			if (pte->pg_v) {
				c = &cmap[pgtocm(pte->pg_pfnum)];
				if (c->c_lock == 0) {
					pte->pg_v = 0;
					if (anycl(pte, pg_m))
						pte->pg_m = 1;
					distcl(pte);
				}
			}
			smp_unlock(&lk_cmap);
		}
		newptes(rp, dptov(rp, 0), rp->p_dsize);
	}
	if (uap->anom == VA_FLUSH) {	/* invalidate all pages */
		for (i = 1; i < rp->p_ssize; i += CLSIZE) {
			pte = sptopte(rp, i);
			smp_lock(&lk_cmap, LK_RETRY);
			if (pte->pg_v) {
				c = &cmap[pgtocm(pte->pg_pfnum)];
				if (c->c_lock == 0) {
					pte->pg_v = 0;
					if (anycl(pte, pg_m))
						pte->pg_m = 1;
					distcl(pte);
				}
			}
			smp_unlock(&lk_cmap);
		}
		newptes(rp, sptov(rp, 0), rp->p_ssize);
		for (i = 0; i < rp->p_tsize; i += CLSIZE) {
			pte = tptopte(rp, i);
			smp_lock(&lk_cmap, LK_RETRY);
			if (pte->pg_v) {
				c = &cmap[pgtocm(pte->pg_pfnum)];
				if (c->c_lock == 0) {
					pte->pg_v = 0;
					if (anycl(pte, pg_m))
						pte->pg_m = 1;
					distcl(pte);
					smp_lock(&lk_text, LK_RETRY);
					distpte(rp->p_textp, i, pte);
					smp_unlock(&lk_text);
				}
			}
			smp_unlock(&lk_cmap);
		}
		newptes(rp, tptov(rp, 0), rp->p_tsize);
	}
	(void)splx(s);
}
/* END DEFUNCT */

/*
 * grow the stack to include the SP
 * true return if successful.
 */
grow(sp)
	unsigned sp;
{
	register int si;
        register struct proc *p = u.u_procp;
        int stack_top;
	char *msg;

	if (sp >= USRSTACK - ctob(u.u_ssize)) {
		if(usermsg)
			uprintf("stack grow failed sp >= USRSTACK-ctob(u.u_ssize) 0x%x >= 0x%x\n",
			sp, USRSTACK-ctob(u.u_ssize));
		return (0);
	}
	si = clrnd(btoc((USRSTACK - sp)) - u.u_ssize + SINCR);
	if (chksize((u_int)u.u_tsize, (u_int)u.u_dsize, (u_int)0,
	   (u_int)u.u_ssize + si)) {
		/* trying to grow last page */
		if (chksize((u_int)u.u_tsize, (u_int)u.u_dsize, (u_int)0,
		   (u_int)u.u_ssize + (si -= SINCR))) {
			msg = "stack grow failed because max stack size exceeded ";
			goto fail;
		}
	}
#ifdef mips
	/*
	 * check for stack segment growing into text or data
	 */
	stack_top = USRSTACK - ctob(u.u_ssize+si);
	if ((p->p_textstart + p->p_textpt*NPTEPG*NBPG) > stack_top) {
                msg = "stack grow failed because of stack would grow into text";
		goto fail;
	}
	if ((p->p_datastart + p->p_datapt*NPTEPG*NBPG) > stack_top) {
                msg = "stack grow failed because of stack would grow into data";
		goto fail;
	}
#endif mips
	if(dmexpand(&(u.u_procp->p_smap), u.u_ssize, u.u_ssize+si, CSTACK)==0){
                msg = "stack grow failed because of no swap space";
		swfail_stat.stack_ex_fail++;
                goto fail;
	}
	expand(si, 1);
	return (1);
fail:
	if (!u.u_error)
		u.u_error = ENOMEM;
        /*
         * Give the user a little in figuring out why his program
         * failed.  (And help avoid having them call us with
         * "the kernel is broken" complaints!)
         */
        if (usermsg 
#ifdef mips
	&& !CURRENT_CPUDATA->cpu_nofault && sp > (USRSTACK - maxssiz - 0x100000)
#endif mips
	) 
		uprintf("%s, pid %d, proc %s cur ssiz 0x%x sincr 0x%x\n",
		msg, p->p_pid, u.u_comm, ctob((u_int)u.u_ssize), ctob(si));
        return(0);
}

/*
 * PLOCK -- This system call provides the user with the ability to 
 *	    "lock" the process' segments into memory.  Only a superuser
 *	    process may do this for obvious reasons.  
 *
 *	    When a segment is locked, 2 flags are set, one in u.u_lock that
 *	    tracks the process' memory locking status.  The other in either
 *	    p->p_vm (data) or tp->x_flag (text) to indicate the lock on
 *	    that level for paging purposes.  In addition, the text structure
 *	    contains a count of the number of shared processes have currently
 *	    locked the text.  When a process unlocks it text segment, the 
 *	    text remains locked until the count (x_lcount) drops to 0.  A
 *	    side effect of this is that if one process locks the segment,
 *	    it's locked for all attached procs as this flag is the one used to
 *	    determine eligibility for paging/swapping.
 *
 *	Input:		long opt;	* operation (see ../h/lock.h)
 *	Return:		0		* normal
 *			-1		* error; errno is set
 */
plock()
{
	struct a {
		long opt;
	};

	if (!suser())
		return;
	switch(((struct a *)u.u_ap)->opt) {
	case TXTLOCK:
		if ((u.u_lock & TXTLOCK) || textlock() == 0)
			goto bad;
		break;
	case PROCLOCK:
		if ((u.u_lock & (TXTLOCK|DATLOCK)) || proclock() == 0)
			goto bad;
		break;
	case DATLOCK:
		if ((u.u_lock & DATLOCK)  ||  datalock() == 0)
			goto bad;
		break;
	case UNLOCK:
		if ((u.u_lock & (TXTLOCK|DATLOCK)) == 0 || punlock() == 0)
			goto bad;
		break;

	default:
bad:
		u.u_error = EINVAL;
	}
}

textlock()
{
	register struct text *tp;	/* text pointer */
	register int s;

	if ((tp = u.u_procp->p_textp) == NULL)
		return(0);
	s = splimp();
	smp_lock(&lk_text, LK_RETRY);
	tp->x_flag |= XNOSW;
	tp->x_lcount++;
	smp_unlock(&lk_text);
	(void)splx(s);
	u.u_lock |= TXTLOCK;
	return(1);
}
		
tunlock()
{
	register struct text *tp;	/* text pointer */
	register int s;

	if ((tp = u.u_procp->p_textp) == NULL)
		return(0);
	s = splimp();
	smp_lock(&lk_text, LK_RETRY);
	if (!(--tp->x_lcount))		
		tp->x_flag &= ~XNOSW;
	smp_unlock(&lk_text);
	(void)splx(s);
	u.u_lock &= ~TXTLOCK;
	return(1);
}

datalock()
{
	register struct proc *pp;	/* proc pointer */

	if ((pp = u.u_procp) == NULL)
		return(0);
	SET_P_VM(pp, SULOCK);
	u.u_lock |= DATLOCK;
	return(1);
}
		
dunlock()
{
	register struct proc *pp;	/* proc pointer */

	if ((pp = u.u_procp) == NULL)
		return(0);
	CLEAR_P_VM(pp, SULOCK);
	u.u_lock &= ~DATLOCK;
	return(1);
}

proclock()
{
	return (textlock() && datalock());
}

punlock()
{
	if (u.u_lock & (TXTLOCK))
		tunlock();
	if (u.u_lock & (DATLOCK))
		dunlock();
	return(1);
}

/* 
 *  end of PLOCK code
 */
