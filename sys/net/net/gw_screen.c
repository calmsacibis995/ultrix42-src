#ifndef lint
static char *sccsid = "@(#)gw_screen.c	4.2	(ULTRIX)	2/28/91";
#endif lint

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

/************************************************************************
 *			Modification History				*
 *									*
 *	25 May 1990		Jeffrey Mogul/DECWRL			*
 *		SMP locking						*
 *									*
 *	16 December 1988	Jeffrey Mogul/DECWRL			*
 *		Created.						*
 *									*
 ************************************************************************/

/*
 * Gateway Screening mechanism
 *
 *	This used to use elevated IPL for synchronization, and then
 *	SMP locking was added.  I think the IPL changes can be
 *	removed safely now, except that they may be necessary
 *	to maintain the minimum IPL requirements for the SMP lock.
 */

#include "../h/param.h"
#include "../h/ioctl.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/uio.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../h/systm.h"
#include "../h/smp_lock.h"
#include "../net/net/if.h"
#include "../net/net/route.h"
#include "../net/net/af.h"
#include "../net/net/gw_screen.h"

int gw_screen_on = 0;		/* Screening on, if true */

struct screen_stats screen_stats;	/* statistics */

int screen_list_initialized = 0;

struct lock_t lk_gwscreen;	/* SMP: gateway screen lock */

/*
 * Access to this facility should be by a new system call, but
 * to keep things simple, we use several new ioctls instead.
 * screen_control() is called from in_control().
 */

screen_control(so, cmd, data, ifp)
	struct socket *so;
	int cmd;
	caddr_t data;
	register struct ifnet *ifp;
{
	int old_screen_on, new_screen_on;
	struct screen_data *sdp = (struct screen_data *)data;

	if (!screen_list_initialized) {	/* serious configuration error? */
	    return(ENOBUFS);
	}

	switch (cmd) {

	case SIOCSCREENON:
		/*
		 * Turns screening on/off (based on arg)
		 * and returns previous setting (value-result arg)
		 */
		new_screen_on = *((int *)data);
		old_screen_on = gw_screen_on;
		switch (new_screen_on) {
		    case SCREENMODE_OFF:
		    case SCREENMODE_ON:
			if (!suser())
			    return(u.u_error);
			gw_screen_on = new_screen_on;
			break;

		    case SCREENMODE_NOCHANGE:
		    default:
			/* no change */
			break;
		}

		/* return old value in any case */
		*((int *)data) = old_screen_on;
		break;

	case SIOCSCREEN:
		/*
		 * Transmits from user to kernel an screen_data
		 * struct, and then copies the next one from kernel
		 * to user into the same buffer (value-result arg).
		 * This allows us to do each transaction with
		 * only one system call.
		 *
		 * This ioctl blocks until the next transaction
		 * is available.
		 */

		if (!suser())
			return(u.u_error);

		if (!gw_screen_on) {
			return(ENOPROTOOPT);
		}

		/* Honor user's wishes on previous packet */
		screen_disposition(sdp->sd_xid, sdp->sd_action);
		
		/* Wait for next packet, create next transaction */
		screen_getnext(sdp);
		break;

	case SIOCSCREENSTATS:
		/*
		 * Provides current statistics block
		 */
		*(struct screen_stats *)data = screen_stats;
		break;
		
	default:
		printf("screen_control: unknown cmd %x\n", cmd);
		panic("screen_control");
	}

	return(0);
}

/*
 * This procedure is called, for example,
 *	instead of ip_forward() from ipintr().
 */
gw_forwardscreen(pkt, ifp, af, fwdproc, errorproc)
	struct mbuf *pkt;
	struct ifnet *ifp;
	int af;
	void (*fwdproc)();
	void (*errorproc)();
{
	if (gw_screen_on == SCREENMODE_OFF) {	/* not our problem */
		(*fwdproc)(pkt, ifp);
	}
	else {
		screen_bufenter(pkt, ifp, af, fwdproc, errorproc);
	}
}

/*
 * Unprocessed packets kept on a list
 */

extern int screen_maxpend;	/* max # of packets pending */

struct screen_listentry {
	struct screen_listentry *next;	/* forward pointer */
	struct screen_listentry *prev;	/* backward pointer */
	int	xid;		/* transaction id */
	struct mbuf *pkt;		/* pointer to the packet */
	struct ifnet *ifp;	/* where it arrived on */
	int	pid;		/* "owning" process, if nonzero */
	struct timeval arrival;	/* arrival time for the packet */
	short	family;		/* address family */
	void	(*fwdproc)();	/* forwarding action (takes pkt, ifp) */
	void	(*errorproc)();	/* error action (takes pkt, ifp) */
};

struct screen_listentry screen_pending;		/* queue header */
struct screen_listentry screen_free;		/* free queue header */

u_long	screen_next_xid = 99;	/* next transaction id */

screen_space_needed()	/* used in mapinit() and screen_init_freelist() */
{
	return(screen_maxpend * sizeof(struct screen_listentry));
}

/*
 * Called from mapinit()
 */
screen_init_freelist(sclp_char)
char *sclp_char;	/* pointer to right amount of storage */
{
	register struct screen_listentry *sclp;
	register int i;
	register int allocsize = screen_space_needed();

	screen_pending.next = &screen_pending;
	screen_pending.prev = &screen_pending;

	screen_free.next = &screen_free;
	screen_free.prev = &screen_free;

	sclp = (struct screen_listentry *)sclp_char;

	if (sclp == NULL) {
		return;		/* we COULD try again later */
	}

	/* Chop up the memory and insert it on the free queue */
	for (i = 0; i < screen_maxpend; i++) {
		insque(sclp, &screen_free);
		sclp++;
	}
	screen_list_initialized = 1;
}

/*
 * Unprocessed packets go onto the pending list.  When the list
 * is full, we drop incoming packets (I think this is the "right
 * thing" to do, based on the way congestion control algorithms
 * work).  We wake up any available screening processes.
 */
screen_bufenter(pkt, ifp, family, fwdproc, errorproc)
struct mbuf *pkt;
struct ifnet *ifp;
int family;
void (*fwdproc)();
void (*errorproc)();
{
	register struct screen_listentry *sclp;
	int s;

	s = splimp();
	smp_lock(&lk_gwscreen, LK_RETRY);

	screen_stats.ss_packets++;

	/* get next free listentry */
	if ((sclp = screen_free.next) == &screen_free) {
	    screen_stats.ss_nobuffer++;
	    m_freem(dtom(pkt));			/* drop packet */
	    smp_unlock(&lk_gwscreen);
	    (void)splx(s);
	    wakeup((caddr_t)&screen_pending);	/* just in case */
	    return;
	}
	remque(sclp);
	
	sclp->xid = screen_next_xid++;
	sclp->pkt = pkt;
	sclp->ifp = ifp;
	sclp->pid = 0;
	sclp->arrival = time;
	sclp->family = family;
	sclp->fwdproc = fwdproc;
	sclp->errorproc = errorproc;

	/* link this on onto pending list */
	insque(sclp, &screen_pending);
	
	smp_unlock(&lk_gwscreen);
	(void)splx(s);

	/* notify waiting screen processes */
	wakeup((caddr_t)&screen_pending);
}

/*
 * Block until something is there, then mark entry as "owned"
 * and return the contents.
 */
screen_getnext(sdp)
register struct screen_data *sdp;
{
	int s;
	register struct screen_listentry *sclp;
	register struct mbuf *m;
	register int remlen;
	register int len;
	register char *dp;

	s = splimp();
	while (1) {
	    smp_lock(&lk_gwscreen, LK_RETRY);
    
	    /* search buffer for un-owned entry, FIFO order */
	    sclp = screen_pending.prev;
	    while (sclp != &screen_pending) {
		/* if not claimed and family matches or is wildcarded */
		if (sclp->pid == 0
			&& ((sdp->sd_family == sclp->family)
				|| (sdp->sd_family == AF_UNSPEC)) ) {
		   goto found;
		}
		sclp = sclp->prev;
	    }

	    /* buffer is empty */
	    
	    screen_purgeold();	/* get rid of stale entries */
    
	    sleep_unlock((caddr_t)&screen_pending, PUSER, &lk_gwscreen);
			    /* this sleep is interruptible */
	}

found:
	sclp->pid = u.u_procp->p_pid;
	sdp->sd_xid = sclp->xid;
	sdp->sd_arrival = sclp->arrival;
	sdp->sd_family = sclp->family;
	m = dtom(sclp->pkt);

	/* copy packet header into sd_data */
	remlen = SCREEN_DATALEN;
	dp = sdp->sd_data;
	while (m && (remlen > 0)) {
	    len = min(remlen, m->m_len);
	    bcopy(mtod(m, caddr_t), dp, len);
	    dp += len;
	    remlen -= len;
	    m = m->m_next;
	}
	
	sdp->sd_count = sizeof(struct screen_data);
	sdp->sd_dlen = dp - sdp->sd_data;
	sdp->sd_action = SCREEN_DROP|SCREEN_NONOTIFY;

	smp_unlock(&lk_gwscreen);
	(void)splx(s);
	return;
}

screen_disposition(xid, action)
register u_long xid;
int action;
{
	int s;
	register struct screen_listentry *sclp;
	register int mypid = u.u_procp->p_pid;

	s = splimp();
	smp_lock(&lk_gwscreen, LK_RETRY);

	/* search for our current transaction; flush stale ones */
	sclp = screen_pending.prev;
	while (sclp != &screen_pending) {
		if (sclp->pid == mypid) {
			remque(sclp);
			if (sclp->xid == xid) {
				/* match */
				goto found;
			}
			else {
				/* stale, flush it */
				m_freem(dtom(sclp->pkt));
				insque(sclp, &screen_free);
				screen_stats.ss_badsync++;
				sclp = screen_pending.prev;
					/* rescan is slow but simple */
			}
		}
		else
			sclp = sclp->prev;
	}
	smp_unlock(&lk_gwscreen);
	(void)splx(s);
	return;		/* nothing to dispose of */

found:
	if (action & SCREEN_ACCEPT) {
	    screen_stats.ss_accept++;
	    smp_unlock(&lk_gwscreen);	/* unlock AFTER changing stats */
	    (void)splx(s);
	    (*(sclp->fwdproc))(sclp->pkt, sclp->ifp);
		    /* this frees sclp->pkt */
	} else {
	    /* this packet is rejected */
	    screen_stats.ss_reject++;
	    smp_unlock(&lk_gwscreen);	/* unlock AFTER changing stats */
	    (void)splx(s);
    
	    if (action & SCREEN_NOTIFY) {
		(*(sclp->errorproc))(sclp->pkt, sclp->ifp);
		/* this frees sclp->pkt */
	    }
	    else
		m_freem(dtom(sclp->pkt));
	}

	s = splimp();
	smp_lock(&lk_gwscreen, LK_RETRY);
	insque(sclp, &screen_free);
	smp_unlock(&lk_gwscreen);
	(void)splx(s);
}

#define	SCREEN_MAXAGE	5	/* default maximum age for packets */

int	screen_maxage = SCREEN_MAXAGE;

/*
 * Free up entries on the pending queue that are more than
 * screen_maxage seconds old.
 *
 * ASSUMPTION: called at high IPL, with lock held
 */
screen_purgeold()
{
	register int cutoff;
	register struct screen_listentry *sclp;
	
	/* Calculate oldest legal age; grace period of 1 sec for roundoff */
	cutoff = (time.tv_sec - screen_maxage) - 1;
	
	sclp = screen_pending.next;
	while (sclp != &screen_pending) {
		if (sclp->arrival.tv_sec < cutoff) {
			screen_stats.ss_stale++;
			remque(sclp);
			m_freem(dtom(sclp->pkt));
			insque(sclp, &screen_free);
			sclp = screen_pending.next;
		}
		else
			sclp = sclp->next;
	}
}
