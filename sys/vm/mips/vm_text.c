#ifndef lint
static	char	*sccsid = "@(#)vm_text.c	4.1	(ULTRIX)	7/2/90";
#endif lint


/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 * 14 Sep 88 -- jaa
 *	Added error leg to rdwri() in xalloc for mips
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

#ifdef GFSDEBUG
extern short GFS[];
#endif

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
	register int isbad;
	
#ifdef mips
	XPRINTF(XPR_VM,"enter xfree",0,0,0,0);
#endif mips
	if(xp == NULL)
		return;
	X_LOCK(xp);
	gp = xp->x_gptr;

	if((count = --xp->x_count) < 0) {
		cprintf("xfree: text 0x%x count bad\n", xp);
		panic("xfree");
	}
	isbad = xp->x_flag & XBAD;

	if((count == 0) && (((gp->g_mode & GSVTX) == 0) || isbad)) {
#ifdef mips
		/* what a hack ... the textbr is zeroed in xunlink */
		/* but we need to free the memory the pte's point to */
		/* so we save the pte's then free later */
		struct pte *tpte = tptopte(p,0);
#endif mips
		xunlink(p);
#ifdef mips
		xp->x_rssize -= vmemfree(tpte, u.u_tsize);
#else ultrix
		xp->x_rssize -= vmemfree(tptopte(p, 0),	u.u_tsize);
#endif !mips
		if (xp->x_rssize != 0) {
			panic("xfree rssize");
		}
		while (xp->x_poip)
			sleep((caddr_t)&xp->x_poip, PSWP+1);
		gp->g_flag &= ~GTRC;
		vsxfree(xp, (long)xp->x_size);

		/*
		 * place the text slot on the free text list
		 * note that text slots are taken off the rear
		 * and returned to the front
		 */
		X_QFREE(xp);
		if (isbad || (xp->x_flag & XTRC)) {
			X_FLUSH(xp);
			X_CLEAR(xp,gp);
        		GRELE(gp);
			xp->x_flag &= ~XTRC;
		}
		X_UNLOCK(xp);
	} 
	else {
		if ((count == 0) && (gp->g_flag & GTRC))
			gp->g_flag &= ~GTRC;
		xp->x_flag &= ~XLOCK;
		xccdec(xp, p);
	}
	p->p_textp = NULL;
}


/*
 * Attach to a shared text segment.
 * If there is no shared text, just return.
 * If there is, hook up to it:
 * if it is not currently being used, it has to be read
 * in from the gnode (gp); the written bit is set to force it
 * to be written out as appropriate.
 * If it is being used, but is not currently in core,
 * a swap has to be done to get it back.
 */


xalloc(gp, ep, pagi)
	register struct gnode *gp;
	register struct exec *ep;
	int pagi;
{
	register struct text *xp;
	register size_t ts;
	register struct proc *p = u.u_procp;
	int xbad_count = 0;
	extern int lbolt;
	
#ifdef mips
	XPRINTF(XPR_VM,"enter xalloc",0,0,0,0);
#endif mips
	/* This accounts for the 0407 case */
	if(ep->a_text == 0)
		return(!NULL);
again:
	if(gp->g_flag & GTEXT) {	/*
					 * We have text pointer lets check it
					 * and run.
					 */
		xp = gp->g_textp;
		if (xp->x_gptr != gp)
			panic("Text Corruption gp != x_gptr");

		/* If cleanup occuring, give it a second to complete */
		if (xp->x_flag & XBAD) {
		      if (xbad_count++) {
		              uprintf("remote text modified and not yet cleaned up\n");
			      psignal(p, SIGKILL);
			      return(NULL);
		      }
		      sleep(&lbolt, PZERO);
		      goto again;
		}
		      
		if ((xp->x_count > 0) || (gp->g_mode & GSVTX)) {
			if (xp->x_flag&XLOCK) {
				X_WAIT(xp);
				goto again;
			}
			X_LOCK(xp);
			xp->x_count++;
			p->p_textp = xp;
			(void) xlink(p);
			X_UNLOCK(xp);
			return(!NULL);
		} else {
			X_LOCK(xp);
			X_DQFREE(xp);
		}
	} 
	else {	/*
		 * find a free text slot, if table is consumed, go
		 * searching for a potentially free slot
		 */

		if((xp = freetext.xun_freeb)  == (struct text *) &freetext) {
			tablefull("text");
			uprintf("text table full\n");
			psignal(p, SIGKILL);
			p->p_textp = NULL;
			return(NULL);
		}
		X_LOCK(xp);
		X_DQFREE(xp);
		if(xp->x_gptr) {
			struct gnode *xgp;

			xgp = xp->x_gptr;

			/*
			 * avoid race condition between gp/xp locking
			 */
			
			if(xgp->g_flag & GLOCKED) {
				X_QFREE(xp);
				if(xp->x_freef == (struct text *)
				&freetext) {

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
			(void) GLOCK(xgp);
			X_FLUSH(xp);
			X_CLEAR(xp,xgp);
			gput(xgp);
		}
		xp->x_flag = 0;
	}
	
	xp->x_flag |= XLOAD;
	if (pagi)
		xp->x_flag |= XPAGI;
	ts = clrnd(btoc(ep->a_text));
	xp->x_size = ts;
	if (p->p_flag & STRC) {
		gp->g_flag |= GTRC;
        }
	X_SET(xp,gp);

	if (vsxalloc(xp) == NULL) {
		swkill(p, "xalloc: no swap space");

		/*
                 * process is not executable, if this was a
                 * reclaim, then leave enough
                 * context so that text reclaim may be possible
                 * else, simply unwind
                 */

		if ((gp->g_flag & GTEXT) == 0) {
			xp->x_flag = 0;
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
		gp->g_count++;
		if (X_DO_RHASH(xp)) {
		        if (gp->g_hcmap_struct == 0)
			        KM_ALLOC(gp->g_hcmap_struct, struct x_hcmap *, G_HCMAP_SIZE(xp), KM_TEMP, KM_CLEAR);
			gp->g_xcount++;
		}
	}

	xp->x_count = 1;
	xp->x_ccount = 0;
	xp->x_lcount = 0;
	xp->x_rssize = 0;
	p->p_textp = xp;
#ifdef vax
	xlink(p);
#endif vax
#ifdef mips
	if (xlink(u.u_procp) == 0) {
		u.u_procp->p_textp = NULL;
		xp->x_flag &= ~(XLOAD | XLOCK | XPAGI | XTRC);
		xp->x_size = 0;
		xp->x_count = 0;
		X_CLEAR(xp,gp);
		GRELE(gp);
		vsxfree(xp, ts);
		X_UNLOCK(xp);
		swkill(u.u_procp, "xalloc: no memory for page tables");
		return (0);
	}
#endif mips
	if (pagi == 0) {
		settprot(RW);
		p->p_flag |= SKEEP;
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
		p->p_flag &= ~SKEEP;
	}
	settprot(RO);
	xp->x_flag |= XWRIT;
	xp->x_flag &= ~XLOAD;
	if (p->p_flag & STRC) 
		gp->g_flag |= GTRC;
	X_UNLOCK(xp);
	return(!NULL);
}

/*
 * Decrement the in-core usage count of a shared text segment.
 * When it drops to zero, free the core space.
 */
xccdec(xp, p)
register struct text *xp;
register struct proc *p;
{

#ifdef mips
	XPRINTF(XPR_VM,"enter xccdec",0,0,0,0);
#endif mips
	if (xp==NULL || xp->x_ccount==0)
		return;
	X_LOCK(xp);
	if (--xp->x_ccount == 0) {
		if (xp->x_flag & XWRIT) {
			vsswap(p, tptopte(p, 0), CTEXT, 0, xp->x_size,
							(struct dmap *)0);
			if (xp->x_flag & XPAGI)
				swap(p, xp->x_ptdaddr, (caddr_t)tptopte(p, 0),
					xp->x_size * sizeof (struct pte),
					B_WRITE, B_PAGET, swapdev, 0);
			xp->x_flag &= ~XWRIT;
		} 
		else {
			xp->x_rssize -= vmemfree(tptopte(p, 0),xp->x_size);
		}
		if (xp->x_rssize != 0)
			panic("text rssize");
	}
	xunlink(p);
	X_UNLOCK(xp);
}

/*
 * free the swap image of all unused saved-text text segments
 * which are from device dev (used by umount system call).
 */
xumount(dev)
register dev;
{
	register struct text *xp;

#ifdef mips
	XPRINTF(XPR_VM,"enter xumount",0,0,0,0);
#endif mips
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

#ifdef mips
	XPRINTF(XPR_VM,"enter xrele",0,0,0,0);
#endif mips
	if((gp->g_flag & GTEXT) == 0)
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

#ifdef mips
	XPRINTF(XPR_VM,"enter xuntext",0,0,0,0);
#endif mips
	X_LOCK(xp);
	if (xp->x_count) {
		X_UNLOCK(xp);
		return;
	}
	gp = xp->x_gptr;
	xp->x_flag &= ~XLOCK;
	if(gp->g_mode & GSVTX) 
		vsxfree(xp, (long)xp->x_size);
 	if ((xp->x_flag & XFREE) == 0)
 	        X_QFREE(xp);
	X_FLUSH(xp);
	X_CLEAR(xp, gp);
	GRELE(gp);
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
	register struct text *xp = gp->g_textp;
	register struct proc *p;

#ifdef mips
	XPRINTF(XPR_VM,"enter xinval",0,0,0,0);
#endif mips
	/* kill them only if paging in from gnode */
	if ((xp->x_flag & XPAGI) && (xp->x_gptr == gp)) {
		for (p = xp->x_caddr; p; p = p->p_xlink) {
			uprintf("pid %d killed due to text modification\n", p->p_pid);
			psignal(p, SIGKILL);
			p->p_flag |= SULOCK;
			xkillcnt++;
		}
	}
	if (xp->x_count == 0)
		xuntext(xp);		/* all done */
	else
		xp->x_flag |= XBAD;	/* remove on last xfree */
}

/*
 * Add a process to those sharing a text segment by
 * getting the page tables and then linking to x_caddr.
 */
xlink(p)
register struct proc *p;
{
	register struct text *xp = p->p_textp;

#ifdef mips
	XPRINTF(XPR_VM,"enter xlink ",0,0,0,0);
#endif mips
	if (xp == 0)
		return(0);
#ifdef vax
	vinitpt(p);
#endif vax
#ifdef mips
        if (vinitpt(p) == 0)
                return (0);
#endif mips
	p->p_xlink = xp->x_caddr;
	xp->x_caddr = p;
	xp->x_ccount++;
	return(1);
}

xunlink(p)
register struct proc *p;
{
	register struct text *xp = p->p_textp;
	register struct proc *q;

#ifdef mips
	XPRINTF(XPR_VM,"enter xunlink",0,0,0,0);
#endif mips
	if (xp == 0)
		return;
	if (xp->x_caddr == p) {
		xp->x_caddr = p->p_xlink;
		p->p_xlink = 0;
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
                                (void) vmemfree(&Usrptmap[a], p->p_textpt);
                                rmfree(kernelmap, (long)p->p_textpt, (long)a);
                        } else {
                                panic("xunlink no text page tables");
                        }
                }
                p->p_textpt = 0;
                p->p_textbr = (struct pte *)0;
#endif mips
		return;
	}
	for (q = xp->x_caddr; q->p_xlink; q = q->p_xlink)
		if (q->p_xlink == p) {
			q->p_xlink = p->p_xlink;
			p->p_xlink = 0;
#ifdef mips
                        p->p_textpt = 0;
                        p->p_textbr = (struct pte *)0;
#endif mips
			return;
		}
	panic("lost text");
}

/*
 * Replace p by q in a text incore linked list.
 * Used by vfork(), internally.
 */
xrepl(p, q)
register struct proc *p, *q;
{
	register struct text *xp = q->p_textp;

#ifdef mips
	XPRINTF(XPR_VM,"enter xrepl",0,0,0,0);
#endif mips
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
	XPRINTF(XPR_TEXT, "xrepl repl pid %d by pid %d", p->p_pid, q->p_pid,
	    0, 0);
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

#ifdef mips
	XPRINTF(XPR_VM,"enter xflush_remote_hash",0,0,0,0);
#endif mips
	if (gp->g_hcmap_struct == NULL)
	        panic("xflush_remote_hash: x_hcmap == NULL");

	hcmap = gp->g_hcmap;

        for (j = 0; j < size_array; j++, hcmap++) {
	        if (*hcmap == 0)
		        continue;
		
		c = &cmap[*hcmap];
		if (c->c_blkno)
		        maunhash(c);
	}

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
	
#ifdef mips
	XPRINTF(XPR_VM,"enter textinit",0,0,0,0);
#endif mips
	freetext.xun_freef = freetext.xun_freeb = (struct text *) &freetext;
	
	for(xp = &text[ntext - 1]; xp >= text; xp--) 
		X_QFREE(xp);
}

#ifdef mips
/*
 * Detach a process from the in-core text.
 * External interface to xccdec, used when swapping out a process.
 */
xdetach(xp, p)
	register struct text *xp;
	struct proc *p;
{
XPRINTF(XPR_VM,"enter xdetach",0,0,0,0);

	if (xp && xp->x_ccount != 0) {
		X_LOCK(xp);
		xccdec(xp, p);
		xunlink(p);
		X_UNLOCK(xp);
	}
}
#endif mips
