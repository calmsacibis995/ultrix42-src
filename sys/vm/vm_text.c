#ifndef lint
static	char	*sccsid = "@(#)vm_text.c	4.5	(ULTRIX)	4/4/91";
#endif lint

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
 *   Modification History:
 *
 * 17-May-90 jaa
 *	xalloc(): after locking text, if (text gp != gp) retry
 *	(another processor may have freed text) else if gp not 
 *	associated with text anymore, panic text corruption
 *
 * 03-Mar-90 jaw
 *	pass spl to restore to vinitpt routine.
 *
 *  06 Feb 90 gmm
 *	Do not clear XLOCK bit in x_flag before calling X_UNLOCK. Else
 *	causes X_UNLOCK to panic.
 *
 *  11 Dec 89 jaa
 *	change dynamic swap to account for swap up front (ala v3.1) 
 * 	but actually do the allocation only when pushing the page/process
 *
 * 04 Dec 89 -- sekhar
 *	minor changes to xccdec to track swap useage
 *
 * 12 Jun 89 -- gg
 *	Modified xccdec() to allocate  swap space before swapping the
 *	text out.
 *	Added new routine xclenup().
 *	Modified xfree() to call xcleanup().
 *	Modified all calls to xccdec() to check for return value and take
 *	appropriate action.
 *
 * 14 Sep 88 -- jaa
 *	Added error leg to rdwri() in xalloc for mips
 *
 * 31 Aug 88 -- jmartin
 *	Protect access to various text structures with lk_text and
 *	overload XLOCK to protect more than reading in text--it was
 *	already doing more.
 *
 * 25 Jul 88 -- jmartin
 *	Use macros SET_P_VM and CLEAR_P_VM to manipulate the field
 *	(struct proc *)foo->p_vm under SMP lock.
 *	
 * 9 Jun 88	-- jaa
 *	Added fix for lost text structures
 *
 * 19 May 88 -- cb
 *	Modified GFS interface.
 *
 * 14 Dec 87 -- jaa
 *	Added new KM_ALLOC/KM_FREE macros
 *
 * 04 Sep 87 -- depp
 *      A number of changes, all involved with removing the xflush_free_text()
 *      algorithm, and replacing it with an array (g_hcmap) to hold the
 *      indexes of remote cmap entries that are hashed.  Xalloc() now
 *      allocates the g_hcmap array if required.  Xflush_free_text() was
 *      removed and replaced by xflush_remote_hash().
 *
 * 18 Aug 86 -- depp, rr
 *      Added a couple of bugfixes and a mechanism for flushing the text
 *      if the remote a.out file has been modified.
 *
 * 20 Feb 86 -- depp
 *	Added a tracing flag (GTRC) in the gnode to indicate that one
 *	or more processes are tracing this gnode.
 *
 * 15 Dec 86 -- depp
 *	Fixed a number of small problems:
 *	  1. In xalloc(), insured that 0407 processes return normally.
 *	  2. In General, insured that memory unhashing (when a gnode
 *	     is written to or the file system is umounted, occurs in a
 *	     consistent manner by using the new macro X_FLUSH (text.h).
 *	     Currently, X_FLUSH works in the traditional manner, not
 *	     flushing local references, only remote.
 *	  3. (from a previous submit) added macros to place/remove text
 *	     table entries on/from the free list -- X_QFREE/XDQFREE (text.h)
 *	  4. Removed the obsolete (and unused) mlock/munlock/mwait routines.
 *
 * 30 Oct 86 -- depp
 *	Fixed a problem with text table allocation (xalloc()).  The x_flag
 *	field of struct text has never been properly cleared on allocation.
 *	This fact was masked until two changes were maded: 1. the use
 *	of a paging threshold to determine whether to demand page a
 *	process from an inode or read in the entire process prior to 
 *	execution.  2. The reworking of text management for local execution
 *	of remote files.  
 *
 * 11 Sep 86 -- koehler
 *	added text management
 *
 * 29 Apr 86 -- depp
 *	converted to locking macros from calls routines
 *
 * 02 Apr 86 -- depp
 *	Added new parameter to "xalloc", a pointer to the a.out header
 *	data.  This data is now on the stack rather than in the "u" 
 *	structure.
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 14 Oct 85 -- reilly
 *	Modified user.h
 *
 * 18 Sep 85 -- depp
 *	Added reset of x_lcount when new text struct created (xalloc)
 *
 */

/*	vm_text.c	6.1	83/07/29	*/

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/text.h"
#include "../h/gnode.h"
#include "../h/mount.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/exec.h"
#include "../h/kmalloc.h"

extern int swapfrag;

struct xfree freetext;

/*
 * relinquish use of the shared text segment
 * of a process.
 */
xfree()
{
	register struct proc *p = u.u_procp;
	register struct text *xp = p->p_textp;
	register struct gnode *gp;
	register int count;
	register int s;
	
	if(xp == NULL)
		return;
	X_LOCK(xp);
	gp = xp->x_gptr;

	if((count = --xp->x_count) < 0) {
		cprintf("xfree: text 0x%x count bad\n", xp);
		panic("xfree");
	}

	if((count == 0) && 
	   (((gp->g_mode & GSVTX) == 0) || (xp->x_flag & XBAD))) {
		xcleanup(xp, p, 1, 1);
	} else {
		if ((count == 0) && (gp->g_flag & GTRC))
			gp->g_flag &= ~GTRC;
		xccdec(xp, p);
	}
	X_UNLOCK(xp);
	p->p_textp = NULL;
}


/*
 * Attach to a shared text segment.  If there is no shared text, just
 * return.  If there is, hook up to it: if it is not currently being used,
 * it has to be read in from the gnode (gp); the written bit is set to
 * force it to be written out as appropriate.  If it is being used, but is
 * not currently in core, a swap has to be done to get it back.
 */


xalloc(gp, ep, pagi)
	register struct gnode *gp;
	register struct exec *ep;
	int pagi;
{
	register struct text *xp;
	register struct proc *p = u.u_procp;
	int xbad_count = 0;
	extern int lbolt;

	/* This accounts for the 0407 case */
	if(ep->a_text == 0)
		return(!NULL);
again:
	if(gp->g_flag & GTEXT) {	/*
					 * We have text pointer lets check it
					 * and run.
					 */
		xp = gp->g_textp;
		X_LOCK(xp);
		if (xp->x_gptr != gp) {
			X_UNLOCK(xp);
			goto again;
		} else if( ! (gp->g_flag & GTEXT))
			panic("xalloc: text corruption ! (gp & GTEXT)");

		if (xp->x_flag & XTRC) {
			u.u_error = ETXTBSY;
			X_UNLOCK(xp);
			uprintf("xalloc: Text file became busy\n");
			psignal(p, SIGKILL);
			return(NULL);
		}

		/* If cleanup occuring, give it a second to complete */
		if (xp->x_flag & XBAD) {
			X_UNLOCK(xp);
			if (xbad_count++) {
				uprintf
			     ("remote text modified and not yet cleaned up\n");
				psignal(p, SIGKILL);
				return(NULL);
			}
			sleep(&lbolt, PZERO);
			goto again;
		}
		      
		if ((xp->x_count > 0) || (gp->g_mode & GSVTX)) {
			xp->x_count++;
			p->p_textp = xp;
			(void) xlink(p);
			X_UNLOCK(xp);
			return(!NULL);
		} else {
			X_DQFREE(xp);
		}
	} else {
		/*
		 * find a free text slot, if table is consumed, go
		 * searching for a potentially free slot
		 */

		int s = splimp();
	        smp_lock(&lk_text, LK_RETRY);
		if((xp = freetext.xun_freeb)  == (struct text *) &freetext) {
			tablefull("text");
			uprintf("text table full\n");
			psignal(p, SIGKILL);
			p->p_textp = NULL;
			smp_unlock(&lk_text);
			(void)splx(s);
			return(NULL);
		}
		xp->x_flag |= XLOCK;
		xp->x_ownlock = u.u_procp;
		if ((((xp)->x_flag ^= XFREE) & XFREE) != 0)
			panic("dequeuing non-free text");
		remque(&(xp->x_free));
		smp_unlock(&lk_text);
		(void)splx(s);
		if(xp->x_gptr) {
			struct gnode *xgp;

			xgp = xp->x_gptr;

			/*
			 * avoid race condition between gp/xp locking
			 */
			
		        if (smp_lock(&xgp->g_lk, LK_ONCE) == LK_LOST) {
				X_QFREE(xp);
				if(xp->x_freef == (struct text *)&freetext) {
					/*
					 * we are here because we are
					 * trying to commit the last
					 * text slot but cannot get all
					 * the resources to fulfill the
					 * exec.  
					 * N.B. the code assumes that the
					 * X_QFREE() puts things on the 
					 * front of the list
					 */
					tablefull("text (commit)");
					uprintf("text table full\n");
					psignal(p, SIGKILL);
					p->p_textp = NULL;
					X_UNLOCK(xp)
					return(NULL);
				}
				X_UNLOCK(xp);
				sleep(&lbolt, PZERO); /* avoid spinning */
				goto again;
			}
			X_FLUSH(xp);
			X_CLEAR(xp,xgp);
			gput(xgp);
		}
		xp->x_flag = XLOCK;	/* Clear all bits but XLOCK. */
	}
	
	xp->x_flag |= XLOAD;
	if (pagi)
		xp->x_flag |= XPAGI;
	xp->x_size = clrnd(btoc(ep->a_text));
	if (p->p_trace & STRC) 
		gp->g_flag |= GTRC;
	X_SET(xp, gp);

	if (vsxalloc(xp) == 0) {
		swfail_stat.text_dmap_fail++;
		swkill(p, "xalloc: no swap space");

		/*
                 * process is not executable, if this was a
                 * reclaim, then leave enough
                 * context so that text reclaim may be possible
                 * else, simply unwind
                 */

		if ((gp->g_flag & GTEXT) == 0) {
			xp->x_flag = XLOCK;	/* Clear all bits but XLOCK. */
			xp->x_size = 0;
			p->p_textp = NULL;		
			X_CLEAR(xp,gp);
		}
		X_QFREE(xp);
		X_UNLOCK(xp);
		return(NULL);
	}
	if(!(gp->g_flag & GTEXT)) {   /*
				       * if the flag is set then the gnode
				       * is already allocated and the count
				       * was already bumped. if not set the
				       * flag and bump the count
				       */
		gp->g_flag |= GTEXT;
		gref(gp);
		if (X_DO_RHASH(xp)) {
		        if (gp->g_hcmap_struct == 0)
			        KM_ALLOC(gp->g_hcmap_struct, struct x_hcmap *, 
				G_HCMAP_SIZE(xp), KM_TEMP, KM_CLEAR);
			gp->g_xcount++;
		}
	}

	xp->x_count = 1;
	xp->x_ccount = 0;
	xp->x_lcount = 0;	/* XLOCK is sufficient protection here  */
	xp->x_rssize = 0;	/* because no one else is attached yet. */
	p->p_textp = xp;
	if (xlink(p) == 0) {
		p->p_textp = NULL;
		xp->x_flag &= ~(XLOAD | XPAGI | XTRC);
		xp->x_count = 0;
		X_CLEAR(xp,gp);
		grele(gp);
		vsxfree(xp);
		xp->x_size = 0; /* clear x_size after vsxfree */
		X_UNLOCK(xp);
		swkill(p, "xalloc: no memory for page tables");
		return (0);
	}
	if (pagi == 0) {
		settprot(RW);
		SET_P_VM(p, SKEEP);
		u.u_error = rdwri(UIO_READ, gp, 
		    (caddr_t)ctob(tptov(p, 0)),
		    (int)ep->a_text,
#ifdef vax
		    (int)(ep->a_magic==0413 ? CLBYTES : sizeof (struct exec)),
#endif vax
#ifdef mips
		    (off_t)N_TXTOFF(ep->ex_f, ep->ex_o),
#endif mips
		    2, (int *)0);
		if (u.u_error) {
			swkill (p, "xalloc: error reading text");
			X_UNLOCK(xp);
			settprot(RO);
			return(NULL);
		}
		CLEAR_P_VM(p, SKEEP);
	}
	settprot(RO);
	xp->x_flag |= XWRIT;
	xp->x_flag &= ~XLOAD;
	if (p->p_trace & STRC) 
		gp->g_flag |= GTRC;
	X_UNLOCK(xp);
	return(!NULL);
}

/*
 * Decrement the in-core usage count of a shared text segment.
 * When it drops to zero, free the core space. Caller locks xp.
 */
xccdec(xp, p)
register struct text *xp;
register struct proc *p;
{
	register int s;

	if(xp == NULL || xp->x_ccount==0)
		return(1);
	if(--xp->x_ccount == 0) {
		if(xp->x_flag & XWRIT) {
			if (vsalloc(xp->x_dmap, CTEXT) == 0) {
				register struct gnode *gp = xp->x_gptr;
				register int complete = 0;
				if((xp->x_count == 0) && (gp->g_mode & GSVTX))
					complete = 1;
				xcleanup(xp, p, complete, 0);
				swfail_stat.text_swap_fail++;
				return(0);
			}
			vsswap(p, tptopte(p, 0), CTEXT, 0, 
			       xp->x_size, (struct dmap *)0);
			if (xp->x_flag & XPAGI){
				register int *dp, poff, ptsize, nfrag;
				dp = xp->x_dmap->dm_ptdaddr;
				poff = 0;
				nfrag = dtob(swapfrag);
				ptsize = xp->x_size * sizeof (struct pte);
				while(ptsize > nfrag) {
					if(*dp == 0)
						panic("xccdec: text pt swap addr 0");
				   swap(p, *dp++, (caddr_t)tptopte(p, poff),
				   nfrag, B_WRITE, B_PAGET, swapdev,0);

					ptsize -= nfrag;
					poff += nfrag/sizeof(struct pte);
				}
				if(*dp == 0)
					panic("xccdec: text pt swap addr 0");
				swap(p, *dp, (caddr_t)tptopte(p, poff),
		    		ptsize, B_WRITE, B_PAGET, swapdev, 0);
			}
			xp->x_flag &= ~XWRIT;
		} else {
			register int freed = vmemfree(tptopte(p,0),xp->x_size);
			s = splimp();
			smp_lock(&lk_text, LK_RETRY);
			xp->x_rssize -= freed;
			smp_unlock(&lk_text);
			(void)splx(s);
		}
		if (xp->x_rssize != 0)
			panic("text rssize");
	}
	s = splimp();
	smp_lock(&lk_text, LK_RETRY);
	xunlink(p);
	smp_unlock(&lk_text);
	(void)splx(s);
	return(1);
}

/*
 * free the swap image of all unused saved-text text segments
 * which are from device dev (used by umount system call).
 */
xumount(dev)
register int dev;
{
	register struct text *xp;

	for (xp = text; xp < textNTEXT; xp++) {
		if (xp->x_gptr && dev == xp->x_gptr->g_dev) 
			xuntext(xp);
	}

}

/*
 * remove a shared text segment from the text table, if possible.
 */
xrele(gp)
register struct gnode *gp;
{
	register struct text *xp;

	if ((gp->g_flag & GTEXT) == 0)
		return;
	xp = gp->g_textp;
	if (gp != xp->x_gptr)
		panic("xrele");
	xuntext(xp);
}

/*
 * remove text image from the text table.
 * the use count must be zero.
 */
xuntext(xp)
register struct text *xp;
{
	register struct gnode *gp;
	int s;

	X_LOCK(xp);
	if (xp->x_count) {
		X_UNLOCK(xp);
		return;
	}
	gp = xp->x_gptr;
	if (gp->g_mode & GSVTX)
		vsxfree(xp);

	s = splimp();
	smp_lock(&lk_text, LK_RETRY);
 	if ((xp->x_flag & XFREE) == 0) {
		xp->x_flag |= XFREE;
		insque(&(xp)->x_free, &freetext);
	}
	smp_unlock(&lk_text);
	(void)splx(s);

	X_FLUSH(xp);
	X_CLEAR(xp,gp);
	X_UNLOCK(xp);
	grele(gp);
}

int xkillcnt = 0;

/*
 * Invalidate the text associated with gp.
 * Kill all active processes.
 * Used for remote file systems that can't lock texts from writes.
 */
xinval(gp)
	register struct gnode *gp;
{
	static char pid_fmt[] = "pid %d killed due to text modification\n";
	register struct text *xp = gp->g_textp;
	register struct proc *p;
	int takelock;
	register int s;

	s = splimp();
	smp_lock(&lk_text, LK_RETRY);	
	takelock = !((xp->x_flag & XLOCK) && (xp->x_ownlock == u.u_procp));
	smp_unlock(&lk_text);
	(void) splx(s);

	if (takelock) X_LOCK(xp);
	/* kill them only if paging in from gnode */
	if ((xp->x_flag & XPAGI) && (xp->x_gptr == gp)) {
		int s = splimp(), pid_match = 0;
		smp_lock(&lk_text, LK_RETRY);
		for (p = xp->x_caddr; p; p = p->p_xlink) {
			if (p->p_pid == u.u_procp->p_pid) pid_match = 1;
			mprintf(pid_fmt, p->p_pid);
			psignal(p, SIGKILL);
			SET_P_VM(p, SULOCK);
			xkillcnt++;
		}
		smp_unlock(&lk_text);
		(void)splx(s);
		if (pid_match) uprintf(pid_fmt, u.u_procp->p_pid);
	}
	if (xp->x_count > 0)
		xp->x_flag |= XBAD;	/* remove on last xfree */
	if (takelock) {
		X_UNLOCK(xp);
		xuntext(xp);
	}
}

/*
 * Add a process to those sharing a text segment by
 * getting the page tables and then linking to x_caddr.
 */
xlink(p)
register struct proc *p;
{
	register struct text *xp = p->p_textp;
	register int s;

	if (xp == 0)
		return(0);
	s = splimp();
	smp_lock(&lk_text, LK_RETRY);
        if (vinitpt(p,s) == 0) {
		smp_unlock(&lk_text);
		(void)splx(s);
                return (0);
	}
	p->p_xlink = xp->x_caddr;
	xp->x_caddr = p;
	smp_unlock(&lk_text);
	(void)splx(s);
	xp->x_ccount++;
	return(1);
}

xunlink(p)
register struct proc *p;
{
	/* It is assumed that the caller has executed X_LOCK(xp); */
	register struct text *xp = p->p_textp;
	register struct proc *q;

	if (xp == 0)
		return;
	if (xp->x_caddr == p) {
		xp->x_caddr = p->p_xlink;
#ifdef mips
                /*
                 * free text page tables
                 */
                if ((xp->x_count == 0) || (xp->x_ccount == 0)) {
			if (xp->x_caddr != NULL)
				panic("xunlink x_caddr !NULL");
                        if (p->p_textpt) {
				register int a;
                                a = btokmx(p->p_textbr);
					/* unlock for lock ordering rules */
				smp_unlock(&lk_text);
                                (void) vmemfree(&Usrptmap[a], p->p_textpt);
                                rmfree(kernelmap, (long)p->p_textpt, (long)a);
					/* relock for calling function */
				smp_lock(&lk_text,LK_RETRY);
                        } else
                                panic("xunlink no text page tables");
                }
                p->p_textpt = 0;
                p->p_textbr = (struct pte *)0;
#endif mips
		p->p_xlink = 0;
		return;
	} 
	for(q = xp->x_caddr; q; q = q->p_xlink) 
		if (q->p_xlink == p) {
			q->p_xlink = p->p_xlink;
#ifdef mips
			p->p_textpt = 0;
			p->p_textbr = (struct pte *)0;
#endif mips
			p->p_xlink = 0;
			return;
		}

	panic("xunlink: lost text");
}

/*
 * Replace p by q in a text incore linked list.
 * Used by vfork(), internally.
 */
xrepl(p, q)
register struct proc *p, *q;
{
	/* It is assumed that the caller has executed X_LOCK(xp); */
	register struct text *xp = q->p_textp;

	if (xp == 0)
		return;
#ifdef vax
	xunlink(p);
	q->p_xlink = xp->x_caddr;
	xp->x_caddr = q;
#endif vax
#ifdef mips
	/*
	 * actually replace p by q in the text incode linked list instead
	 * of removing p and then adding q.
	 */

	if (xp->x_caddr == p) {
		xp->x_caddr = q;
		q->p_xlink = p->p_xlink;
		p->p_xlink = 0;
	} else {
		register struct proc *r;
		for (r = xp->x_caddr; r->p_xlink; r = r->p_xlink) {
			if (r->p_xlink == p) {
				r->p_xlink = q;
				q->p_xlink = p->p_xlink;
				p->p_xlink = 0;
				break;
			}
		}
		if (r->p_xlink == NULL)
			panic("xrepl: lost text");
	}
#endif mips
}

/*
 *  This routine will travel down the array containing the text's cmap indexes
 *  and insure that they are removed from the hash list.  If more than one text
 *  struct is associated with a given gnode, then the hash is flushed, but the
 *  array isn't deallocated.
 */
xflush_remote_hash(xp)
     register struct text *xp;
{
        register struct gnode *gp = xp->x_gptr;
        register int *hcmap;
	register int size_array = xp->x_size >> CLSIZELOG2;
        register int j;
	register struct cmap *c;
	struct cmap *c1;
	int s;

	if (gp->g_hcmap_struct == NULL)
	        panic("xflush_remote_hash: x_hcmap == NULL");

	hcmap = gp->g_hcmap;

	s = splimp();
        for (j = 0; j < size_array; j++, hcmap++) {
	        if (*hcmap == 0)
		        continue;
		
		smp_lock(&lk_cmap, LK_RETRY);
		c = &cmap[*hcmap];
		if (c->c_blkno)
		        maunhash(c);
		smp_unlock(&lk_cmap);
	}
	(void)splx(s);

	if (--gp->g_xcount > 0)
	        return;

	/* release the array */
	if (gp->g_hcount != 0)
	        panic("xflush_remote_hash: g_hcount != NULL");
	KM_FREE(gp->g_hcmap_struct, KM_TEMP);
	gp->g_hcmap_struct = NULL;
}

textinit()
{
	
	/* 
	 * set up the doubly linked list of free text structures
	 */
	
	register struct text *xp;
	
	freetext.xun_freef = freetext.xun_freeb = (struct text *) &freetext;
	
	lockinit(&lk_text, &lock_text_d);
	for(xp = &text[ntext - 1]; xp >= text; xp--) 
		X_QFREE(xp);
}

/*
 * xcleanup() is called with complete=1 and who=1 from xfree()
 * and who=0 and complete=1 (if x_count=0 and sticky case)and
 * complete=0 otherwise from xccdec()
 * NOTE:  caller locks xp;
 */

xcleanup(xp, p, complete, who)
register struct text *xp;
register struct proc *p;
register int complete, who; 
{
	register struct gnode *gp = xp->x_gptr;
	int s, freed;

	freed = vmemfree(tptopte(p, 0),	xp->x_size);
	s = splimp();
	smp_lock(&lk_text, LK_RETRY);
	xp->x_rssize -= freed;
	if (xp->x_rssize != 0)
		panic("xcleanup rssize");
	if(who == 0)
		xp->x_flag |= XNOSPCE;
	xunlink(p);
	if(complete == 0) {
		smp_unlock(&lk_text);
		(void)splx(s);
		return;
	}
	while (xp->x_poip) {
		sleep_unlock((caddr_t)&xp->x_poip, PSWP+1, &lk_text);
		smp_lock(&lk_text, LK_RETRY);
	}
	smp_unlock(&lk_text);
	(void)splx(s);
			
	gp->g_flag &= ~GTRC;
	vsxfree(xp);

	/*
         * place the text slot on the free text list
	 * note that text slots are taken off the rear
	 * and returned to the front
	*/
	X_QFREE(xp);
	if( (xp->x_flag & XBAD) || (xp->x_flag & XTRC)) {
		X_FLUSH(xp);
		X_CLEAR(xp,gp);
        	grele(gp);
		xp->x_flag &= ~XTRC;
	}
} 
