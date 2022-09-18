#ifndef lint
static	char	*sccsid = "@(#)uipc_usrreq.c	4.9	(ULTRIX)	4/30/91";
#endif

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

/************************************************************************
 *			Modification History				*
 *									*
 *	28-Apr-91 -- jsd
 *		Add check for m_len==0 in unp_connect(), return EINVAL
 *
 *	11-Apr-91 -- jsd
 *		Add missing splnet() ... splx(s) in case PRU_SENSE
 *
 *	2-Apr-91 -- jsd
 *		Fix SMP bug, exit with lock held in case PRU_SENSE
 *
 *	7-Mar-91 -- lp
 *		Increase buffer space in local domain connections.
 *
 *	28-Feb-91 -- prs
 *		Added support for a configurable number
 *		of open file descriptors.
 *
 *	20-Feb-91 -- jaw
 *	 	fix for holes in MP locking.
 *
 * 	16-Apr-90 -- jaw
 *	performance fixes for single cpu.
 *
 *	U. Sinkewicz  9-11-89
 *		Added an access check on the socket in unp_connect.
 *
 *	R. Bhanukitsiri 12-Sep-89
 *		Bug fixes.  Calculate the sb_cc in PRU_SEND AFTER
 *		the call to sbappend().
 *
 *	U. Sinkewicz 20-July-89
 *		Bug fixes.  Added locks to socket accessed through
 *	unp->unp_ref;  Added smp_owner checks on so_head so soqinsque/
 *	soqremque would work.
 *
 *	condylis 23-Jun-89
 *		Removed reference to unp_gc().
 *
 *	cb - 19-May-88							*
 *		Modified GFS interface.					*
 *									*
 *	lp - 11-Feb-88							*
 *		Fixed getpeername panic. Used wrong pointer.		*
 *									*
 *	Larry Palmer 15-Jan-88						*
 *		43BSD final release. Malloced network.			*
 *									*
 *      cb 14 jul 87							*
 *      changed mknod interface.					*
 *									*
 *	Koehler 11 Sep 86						*
 *	changed the gfs namei interface					*
 *									*
 *	David L. Ballenger, 28-Nov-1984					*
 * 001	Add fixes so that fstat() calls on pipes will set up		*
 *	st_blksize. This will cause I/O on pipes to be buffered.	*
 *									*
 *	Larry Cohen,	5-March-1985					*
 * 002  Add out of band capability on UNIX domain sockets.		*
 *									*
 * 003	Larry Cohen,   09/16/85 -  Add 43bsd changes			*
 *									*
 * 	Stephen Reilly, 09-Sep-85					*
 *	Modified to handle the new 4.3BSD namei code.			*
 *									*
 *	Larry Cohen, 10/02/85						*
 *	remove panics from unp_scan. The panics are no longer needed	*
 *									*
 *	R. Rodriguez, 11/11/85						*
 *	fix the unp_attach routine to handle streams different than 	*
 *	datagrams. This makes pipes buffered at 4096			*
 ************************************************************************/

/*	uipc_usrreq.c	6.2	83/09/08	*/

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/smp_lock.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/unpcb.h"
#include "../h/un.h"
#include "../h/gnode.h"
#include "../h/mount.h"
#include "../h/file.h"
#include "../h/stat.h"
#include "../h/kmalloc.h"

/*
 * Unix communications domain.
 *
 * TODO:
 *	SEQPACKET, RDM
 *	rethink name space problems
 *	need a proper out-of-band
 */
struct	sockaddr sun_noname = { AF_UNIX };
extern struct lock_t lk_udpstat;

struct lock_t lk_so_disconnect;
/* 
 * SMP:
 * This file has smp modifications for smp.  Enter uipc_usrreq() at
 * splnet with an smp lock asserted. Unlocks are added before gfs 
 * routines.
 */
/*ARGSUSED*/
uipc_usrreq(so, req, m, nam, rights)
	register struct socket *so;
	int req;
	struct mbuf *m, *nam, *rights;
{
	struct unpcb *unp = sotounpcb(so);
	register struct socket *so2;
	int error = 0;
	int s;

	if (req == PRU_CONTROL)
		return (EOPNOTSUPP);
	if (req != PRU_SEND && rights && rights->m_len) {
		error = EOPNOTSUPP;
		goto release;
	}
	if (unp == 0 && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}
	switch (req) {

	case PRU_ATTACH:
		if (unp) {
			error = EISCONN;
			break;
		}
		error = unp_attach(so);
		break;

	case PRU_DETACH:
		unp_detach(unp);
		break;

	case PRU_BIND:
		error = unp_bind(unp, nam);
		break;

	case PRU_LISTEN:
		if (unp->unp_inode == 0)
			error = EINVAL;
		break;

	case PRU_CONNECT:
		error = unp_connect(so, nam);
		break;

	case PRU_CONNECT2:
		if (smp) {
			smp_unlock(&so->lk_socket);
			smp_lock(&lk_so_disconnect,LK_RETRY);
			smp_lock(&so->lk_socket,LK_RETRY);
			so2 = (struct socket *)nam;
			smp_lock(&so2->lk_socket, LK_RETRY);
		} else { 
			so2 = (struct socket *)nam;
		}
		if (so2->so_pcb) 
			error = unp_connect2(so, (struct socket *)so2);
		else
			error = ENOTCONN;
			
		if (smp) {
			smp_unlock(&so2->lk_socket);
			smp_unlock(&lk_so_disconnect);
		}
		break;

	case PRU_DISCONNECT:
		if (smp) {
			smp_unlock(&so->lk_socket);
			smp_lock(&lk_so_disconnect,LK_RETRY);
			smp_lock(&so->lk_socket,LK_RETRY);
		}
		unp_disconnect(unp);
		if (smp) smp_unlock(&lk_so_disconnect);
		break;

	case PRU_ACCEPT:
		if (smp) {
			smp_unlock(&so->lk_socket);
			smp_lock(&lk_so_disconnect,LK_RETRY);
			smp_lock(&so->lk_socket,LK_RETRY);
		}
		
		/*
		 * Pass back name of connected socket,
		 * if it was bound and we are still connected
		 * (our peer may have closed already!).
		 */
		if (unp->unp_conn && unp->unp_conn->unp_addr) {
			if (smp) {
				so2 = unp->unp_conn->unp_socket;
				smp_lock(&so2->lk_socket,LK_RETRY);
				smp_unlock(&lk_so_disconnect);
			}

			nam->m_len = unp->unp_conn->unp_addr->m_len;
			bcopy(mtod(unp->unp_conn->unp_addr, caddr_t),
			    mtod(nam, caddr_t), (unsigned)nam->m_len);
			if (smp) smp_unlock(&so2->lk_socket);
		} else {
			if (smp) smp_unlock(&lk_so_disconnect);
			nam->m_len = sizeof(sun_noname);
			*(mtod(nam, struct sockaddr *)) = sun_noname;
		}

		break;

	/*
	 * Called at splnet with socket lock held (soshutdown). 
	 */
	case PRU_SHUTDOWN:
		socantsendmore(so);
		unp_usrclosed(unp);
		break;

	case PRU_RCVD:
		switch (so->so_type) {

		case SOCK_DGRAM:
			panic("uipc 1");
			/*NOTREACHED*/

		case SOCK_STREAM:
#define	rcv (&so->so_rcv)
#define snd (&so2->so_snd)

			if (smp) {
				smp_unlock(&so->lk_socket);

				smp_lock(&lk_so_disconnect,LK_RETRY);
				smp_lock(&so->lk_socket,LK_RETRY);

				if (unp->unp_conn == 0) {
					smp_unlock(&lk_so_disconnect);
					error = ENOTCONN;
					break;
				}
				so2 = unp->unp_conn->unp_socket;
				smp_lock(&so2->lk_socket,LK_RETRY);
				smp_unlock(&lk_so_disconnect);
			} else {
 	                       if (unp->unp_conn == 0) {
					error = ENOTCONN;
        	                        break;
				}
        	                so2 = unp->unp_conn->unp_socket;
        	        }
		        /*
       	                 * Adjust backpressure on sender
       	                 * and wakeup any waiting to write.
                         */
       	                snd->sb_mbmax += unp->unp_mbcnt - rcv->sb_mbcnt;
       	                unp->unp_mbcnt = rcv->sb_mbcnt;
       	                snd->sb_hiwat += unp->unp_cc - rcv->sb_cc;
       	                unp->unp_cc = rcv->sb_cc;
       	                sowwakeup(so2);
			if (smp) smp_unlock(&so2->lk_socket);

#undef snd
#undef rcv
			break;

		default:
			panic("uipc 2");
		}
		break;

	case PRU_SEND:
		if (rights) {
			error = unp_internalize(rights);
			if (error)
				break;
		}
		switch (so->so_type) {

		case SOCK_DGRAM: {
			struct sockaddr *from;
			if (nam) {
				if (unp->unp_conn) {
					error = EISCONN;
					break;
				}
				error = unp_connect(so, nam);
				if (error) 
					break;
			} else {
				if (unp->unp_conn == 0) {
					error = ENOTCONN;
					break;
				}
			}

			if (smp) {
				smp_unlock(&so->lk_socket);

				smp_lock(&lk_so_disconnect,LK_RETRY);
				smp_lock(&so->lk_socket,LK_RETRY);

				if (unp->unp_conn == 0) {
					smp_unlock(&lk_so_disconnect);
					error = ENOTCONN;
					break;
				}
				so2 = unp->unp_conn->unp_socket;
				smp_lock(&so2->lk_socket,LK_RETRY);
				smp_unlock(&lk_so_disconnect);
			} else {
 	                       if (unp->unp_conn == 0) {
					error = ENOTCONN;
        	                        break;
				}
        	                so2 = unp->unp_conn->unp_socket;
        	        }


			if (unp->unp_addr)
				from = mtod(unp->unp_addr, struct sockaddr *);
			else
				from = &sun_noname;

			if (sbspace(&so2->so_rcv) > 0 &&
			sbappendaddr(&so2->so_rcv, from, m, rights)) {
				sorwakeup(so2);
				m = 0;
			} else{
				error = ENOBUFS;
			}
			if (smp) smp_unlock(&so2->lk_socket);

			if (nam) {
				smp_unlock(&so->lk_socket);
				smp_lock(&lk_so_disconnect,LK_RETRY);
				smp_lock(&so->lk_socket,LK_RETRY);
				unp_disconnect(unp);
				smp_unlock(&lk_so_disconnect);
			}
			break;
		}

		case SOCK_STREAM:
#define	rcv (&so2->so_rcv)
#define	snd (&so->so_snd)
			if (smp) {
				smp_unlock(&so->lk_socket);

				smp_lock(&lk_so_disconnect,LK_RETRY);
				smp_lock(&so->lk_socket,LK_RETRY);

				if (unp->unp_conn == 0) {
					smp_unlock(&lk_so_disconnect);
					error = ENOTCONN;
					break;
				}
				so2 = unp->unp_conn->unp_socket;
				smp_lock(&so2->lk_socket,LK_RETRY);
				smp_unlock(&lk_so_disconnect);
			} else {
 	                       if (unp->unp_conn == 0) {
					error = ENOTCONN;
        	                        break;
				}
        	                so2 = unp->unp_conn->unp_socket;
        	        }

			if (so->so_state & SS_CANTSENDMORE) {
				error = EPIPE;
				break;
			}
			so2 = unp->unp_conn->unp_socket;
			/*
			 * Send to paired receive port, and then
			 * give it enough resources to hold what it already has.
			 * Wake up readers.
			 */
			if (rights)
				(void)sbappendrights(rcv, m, rights);
			else
				sbappend(rcv, m);
			snd->sb_mbmax -= 
				rcv->sb_mbcnt - unp->unp_conn->unp_mbcnt;
			unp->unp_conn->unp_mbcnt = rcv->sb_mbcnt;
			snd->sb_hiwat -= rcv->sb_cc - unp->unp_conn->unp_cc;
			unp->unp_conn->unp_cc = rcv->sb_cc;
			sorwakeup(so2);
			if (smp) smp_unlock(&so2->lk_socket);
			m = 0;
#undef snd
#undef rcv
			break;

		default:
			panic("uipc 4");
		}
		break;

	case PRU_ABORT:
		unp_drop(unp, ECONNABORTED);
		break;

	case PRU_SENSE:
		/* Warning:  We do not own the lock on "so" in 
		   this case.  In all other cases that I know of
		   we do... jaw */
		s = splnet();
		if (smp) {
			smp_lock(&lk_so_disconnect,LK_RETRY);
			smp_lock(&so->lk_socket,LK_RETRY);

			if (unp->unp_conn) {
				so2 = unp->unp_conn->unp_socket;
				smp_lock(&so2->lk_socket,LK_RETRY);
			} 
			smp_unlock(&lk_so_disconnect);
		}
		/* DLB001
		 * Set up block size information for fstat() calls on
		 * pipes.
		 */
		((struct stat *) m)->st_blksize = so->so_snd.sb_hiwat ;
		if (so->so_type == SOCK_STREAM && unp->unp_conn != 0) {
			so2 = unp->unp_conn->unp_socket;
			((struct stat *) m)->st_blksize += so2->so_rcv.sb_cc;
		}
		if (smp) {
			if (unp->unp_conn)
				smp_unlock(&so2->lk_socket);
			smp_unlock(&so->lk_socket);
		}
		splx(s);
		return(0);

	case PRU_RCVOOB:
#ifndef 43BSD
		/* 
		 * LSC002
		 * recieve out of band data - dap@tek
		 */
		switch (so->so_type) {
		case SOCK_STREAM:
			if (so->so_oobmark == 0
			&& (so->so_state & SS_RCVATMARK) == 0) {
				error = EINVAL; /* no OOB data to recv */
				break;
			}
			if (unp->unp_oob == 0)  {
				 /*
				  * no data to recv. must have just
				  * received it.
				  */
				 error = EWOULDBLOCK;
				 break;
			}
			/*
			 * link the OOB data mbuf to the mbuf we were
			 * passed.  The caller will free both these mubfs
			 * -dap@tek
			 */
			m->m_len = 0;
			m->m_next = unp->unp_oob;
			unp->unp_oob = 0;
			break;

		default:
			error = EOPNOTSUPP;
			break;
		}
		return (error);
#else 
				
		return (EOPNOTSUPP);
#endif
				
	case PRU_SENDOOB:
#ifndef 43BSD
		/*
		 * LSC002
		 * send OOB data.  At most MLEN bytes will be sent.
		 * At most one outstanding request will be allowed
		 * - dap@tek
		 */ 
		switch (so->so_type) {
			struct unpcb *unp2;
		case SOCK_STREAM:
			if (smp) {
				smp_unlock(&so->lk_socket);

				smp_lock(&lk_so_disconnect,LK_RETRY);
				smp_lock(&so->lk_socket,LK_RETRY);

				if (unp->unp_conn == 0) {
					smp_unlock(&lk_so_disconnect);
					error = ENOTCONN;
					break;
				}
				so2 = unp->unp_conn->unp_socket;
				smp_lock(&so2->lk_socket,LK_RETRY);
				smp_unlock(&lk_so_disconnect);
			} else {
 	                       if (unp->unp_conn == 0) {
					error = ENOTCONN;
        	                        break;
				}
        	                so2 = unp->unp_conn->unp_socket;
        	        }
			unp2 = sotounpcb(so2);
			
			if (unp2->unp_oob) {
				/*
				 * must wait for the preceeding OOB data
				 * to be handled.  - dap@tek
				 */
				error = EWOULDBLOCK;
				smp_unlock(&so2->lk_socket);
				break;
			}
		
			/*
			 * Make sure there is at most MLEN bytes of data.
			 * This is to place a bound on the amt of mbufs that
			 * may be wasted  if the recv()er never asks for 
			 * OOB data.  - dap@tek
			 */
			{
				register int size = 0;
				register struct mbuf *n;
				for (n=m; n; n = n->m_next) {
					size += n->m_len;
					if (size > MLEN) {
						error = EMSGSIZE;
						break;
					}
				}
			}
			if (error){
				smp_unlock(&so2->lk_socket);
				break;
			}
			unp2->unp_oob = m;
			if ((so2->so_oobmark = so2->so_rcv.sb_cc) == 0){
				so2->so_state |= SS_RCVATMARK;
			}
			sohasoutofband(so2);
			smp_unlock(&so2->lk_socket);
			return(0);		/* don't release mbuf chain */
		
		default:
			error =  EOPNOTSUPP;
		}
		break;
#else
			error =  EOPNOTSUPP;
		break;
#endif
	case PRU_SOCKADDR:
		break;

	case PRU_PEERADDR:
		if (smp) {
			smp_unlock(&so->lk_socket);
			smp_lock(&lk_so_disconnect,LK_RETRY);
			smp_lock(&so->lk_socket,LK_RETRY);
		}
		if (unp->unp_conn && unp->unp_conn->unp_addr) {
			if (smp) {
				so2 = unp->unp_conn->unp_socket;
				smp_lock(&so2->lk_socket,LK_RETRY);
				smp_unlock(&lk_so_disconnect);
			}
			nam->m_len = unp->unp_conn->unp_addr->m_len;
			bcopy(mtod(unp->unp_conn->unp_addr, caddr_t),
				mtod(nam, caddr_t), (unsigned)nam->m_len);
			if (smp) smp_unlock(&so2->lk_socket);
		} else {
			if (smp) smp_unlock(&lk_so_disconnect);
		}

		break;

	case PRU_SLOWTIMO:
		break;

	default:
		/* LSC002 return error - dont panic */
		error = EOPNOTSUPP;
	}
release:
	if (m)
		m_freem(m);
	return (error);
}

/*
 *	We assign all buffering for stream sockets to the source,
 *	as that is where the flow control is implemented.
 *	Datagram sockets really use the sendspace as the maximum
 *	datagram size, and don't really want to reserve the sendspace.
 *	Their recvspace should be large enough for at least one max-size
 *	datagram plus address.
 */
#ifndef PIPSIZ
#define PIPSIZ 4096
#endif NOT PIPSIZ
int	unpst_sendspace = 32*1024;
int	unpst_recvspace = 32*1024;
int	unpdg_sendspace = 1024*2;	/* really max datagram size */
int	unpdg_recvspace = 4*1024;

int 	unp_rights;

unp_attach(so)
	struct socket *so;
{
	register struct unpcb *unp;
	int error;
	
	switch (so->so_type) {

	case SOCK_STREAM:
		error = soreserve(so, unpst_sendspace, unpst_recvspace);
		break;
	case SOCK_DGRAM:
		error = soreserve(so, unpdg_sendspace, unpdg_recvspace);
		break;
	}
	if (error)
		return (error);

	KM_ALLOC(unp, struct unpcb *, sizeof(struct unpcb), KM_PCB, KM_NOW_CL);
	if(unp == NULL)
		return(ENOBUFS);
	so->so_pcb = (caddr_t)unp;
	unp->unp_socket = so;
	return (0);
}

unp_detach(unp)
	register struct unpcb *unp;
{
	int s; /* SMP */
	struct gnode *gp;

	smp_unlock(&unp->unp_socket->lk_socket);
	smp_lock(&lk_so_disconnect,LK_RETRY);
	smp_lock(&unp->unp_socket->lk_socket,LK_RETRY);

	if (unp->unp_inode) {
		unp->unp_inode->g_socket = 0;
		gp = unp->unp_inode;
		unp->unp_inode = 0;
		
		smp_unlock(&unp->unp_socket->lk_socket);
		smp_unlock(&lk_so_disconnect);
		grele(gp);
		s = splnet(); 
		smp_lock(&lk_so_disconnect,LK_RETRY);
		smp_lock(&unp->unp_socket->lk_socket,LK_RETRY);
	}

	if (unp->unp_conn)
		unp_disconnect(unp);
	{
	struct socket *so_t = NULL;
	while (unp->unp_refs){
	   if ( unp->unp_socket != unp->unp_refs->unp_socket) {
		so_t = unp->unp_refs->unp_socket;
		 	smp_unlock(&unp->unp_socket->lk_socket); 	
		smp_lock(&so_t->lk_socket, LK_RETRY);
		unp_drop(unp->unp_refs, ECONNRESET);
		smp_unlock(&so_t->lk_socket);
		 	smp_lock(&unp->unp_socket->lk_socket, LK_RETRY);   
	   }
	   else {
		unp_drop(unp->unp_refs, ECONNRESET);
		}
	}
	}
	soisdisconnected(unp->unp_socket);
	unp->unp_socket->so_pcb = 0;
	smp_unlock(&lk_so_disconnect);
	m_freem(unp->unp_addr);
	if (unp->unp_oob)
		m_freem(unp->unp_oob);
	KM_FREE(unp, KM_PCB);
}

unp_bind(unp, nam)
	struct unpcb *unp;
	struct mbuf *nam;
{
	struct sockaddr_un *soun = mtod(nam, struct sockaddr_un *);
	register struct gnode *gp;
  	register struct nameidata *ndp = &u.u_nd;
	int error;
	struct socket *sol;
	char *m2;

	if (nam->m_len == MLEN) {
		return (EINVAL);
	}
	/* If nam is only a small mbuf, we need to allocate at least
	 * MAXPATHLEN buffer space for subsequent call to gfs_namei.
	 */
	if (nam->m_len < MLEN) {
	    KM_ALLOC(m2, char *, M_CLUSTERSZ, KM_TEMP, KM_NOWAIT|KM_CLEAR);
	    if (m2 == NULL) {
		    return(ENOBUFS);
	    }
	    bcopy(soun->sun_path, m2, (u_int)nam->m_len - sizeof(short));
 	    ndp->ni_dirp = m2;
	} else {
	    /* else we already have a cluster mbuf, just use that */
	    *(mtod(nam, caddr_t) + nam->m_len) = 0;
 	    ndp->ni_dirp = soun->sun_path;
	}
/* SHOULD BE ABLE TO ADOPT EXISTING AND wakeup() ALA FIFO's */
  	ndp->ni_nameiop = CREATE | FOLLOW;
 
	/* Sleep here, so need to unlock. */
	sol = unp->unp_socket; /* SMP */
	smp_unlock(&sol->lk_socket); /* SMP */

  	gp = gfs_namei(ndp);
	if (gp) {
		gput(gp);
		if (nam->m_len < MLEN)
		        KM_FREE(m2, KM_TEMP);
		smp_lock(&sol->lk_socket, LK_RETRY);
		return (EADDRINUSE);
	}
	if (error = u.u_error) {
		u.u_error = 0;			/* XXX */
		if (nam->m_len < MLEN)
		        KM_FREE(m2, KM_TEMP);
		smp_lock(&sol->lk_socket, LK_RETRY);
		return (error);
	}
  	gp = GMAKNODE(GFSOCK | 0777, (dev_t)0, ndp);

	if (gp == NULL) {
		error = u.u_error;		/* XXX */
		u.u_error = 0;			/* XXX */
		if (nam->m_len < MLEN)
		        KM_FREE(m2, KM_TEMP);
		smp_lock(&sol->lk_socket, LK_RETRY);
		return (error);
	}
	smp_lock(&sol->lk_socket, LK_RETRY);
	gp->g_socket = unp->unp_socket;
	unp->unp_inode = gp;
	unp->unp_addr = m_copy(nam, 0, (int)M_COPYALL);
	smp_unlock(&sol->lk_socket);
	gfs_unlock(gp);			/* but keep reference */
	if (nam->m_len < MLEN)
		KM_FREE(m2, KM_TEMP);
	smp_lock(&sol->lk_socket, LK_RETRY);
	return (0);
}

/*
 * Socket lock held coming in.  
 */
unp_connect(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	register struct sockaddr_un *soun = mtod(nam, struct sockaddr_un *);
 	register struct nameidata *ndp = &u.u_nd;
	register struct gnode *gp;
	register int error;
	register struct socket *so2;
	register struct socket *so3;
	register struct unpcb *unp;
	char *m2;

	if (nam->m_len + (nam->m_off - MMINOFF) == MLEN) {
		return (EMSGSIZE);
	}
	if (nam->m_len <= 0)
		return (EINVAL);

	/* If nam is only a small mbuf, we need to allocate at least
	 * MAXPATHLEN buffer space for subsequent call to gfs_namei.
	 */
	if (nam->m_len < MLEN) {
	    KM_ALLOC(m2, char *, M_CLUSTERSZ, KM_TEMP, KM_NOWAIT|KM_CLEAR);
	    if (m2 == NULL) {
		    return(ENOBUFS);
	    }
	    bcopy(soun->sun_path, m2, (u_int)nam->m_len - sizeof(short));
 	    ndp->ni_dirp = m2;
	} else {
	    /* else we already have a cluster mbuf, just use that */
	    *(mtod(nam, caddr_t) + nam->m_len) = 0;
 	    ndp->ni_dirp = soun->sun_path;
	}

	unp = (struct unpcb *)so->so_pcb;

  	ndp->ni_nameiop = LOOKUP | FOLLOW;
	/* Sleep here, so need to unlock. */
	smp_unlock(&so->lk_socket);
  	gp = gfs_namei(ndp);
	smp_lock(&so->lk_socket, LK_RETRY);

	if (gp == 0) {
		error = u.u_error;
		u.u_error = 0;
		if (nam->m_len < MLEN)
		    KM_FREE(m2, KM_TEMP);
		return (error);		/* XXX */
	}
	if ((gp->g_mode&GFMT) != GFSOCK) {
		error = ENOTSOCK;
		smp_unlock(&so->lk_socket);	
		goto bad_nolocks;
	}

	/* Check access permissions on the socket.  */

	smp_unlock(&so->lk_socket);	
	if ( access(gp, GREAD) || access(gp, GWRITE) ){
		error = EACCES;
		goto bad_nolocks;
	}

	smp_lock(&lk_so_disconnect, LK_RETRY);
	smp_lock(&so->lk_socket, LK_RETRY);

	so2 = gp->g_socket;
	if (so2 == 0) {
		error = ECONNREFUSED;
		goto bad;
	}
	smp_lock(&so2->lk_socket, LK_RETRY);
	if (so->so_type != so2->so_type) {
		error = EPROTOTYPE;
		smp_unlock(&so2->lk_socket);
		goto bad;
	}
	so3 = so2;
	if (so->so_proto->pr_flags & PR_CONNREQUIRED &&
	    ((so2->so_options&SO_ACCEPTCONN) == 0 ||
	     (so3 = sonewconn(so2)) == 0)) {
		error = ECONNREFUSED;
		smp_unlock(&so2->lk_socket);
		goto bad;
	}
	if (so3 != so2){
		smp_unlock(&so2->lk_socket);
		smp_lock(&so3->lk_socket,LK_RETRY);
	}
	error = unp_connect2(so, so3);
	smp_unlock(&so3->lk_socket);

bad:
	smp_unlock(&so->lk_socket);
	smp_unlock(&lk_so_disconnect);

bad_nolocks:
	gput(gp);
	if (nam->m_len < MLEN)
		KM_FREE(m2, KM_TEMP);
	smp_lock(&so->lk_socket, LK_RETRY);
	return (error);
}

unp_connect2(so, so2)
	register struct socket *so;
	register struct socket *so2;
{
	register struct unpcb *unp = sotounpcb(so);
	register struct unpcb *unp2;

	if (so2->so_type != so->so_type)
		return (EPROTOTYPE);

	unp2 = sotounpcb(so2);
	unp->unp_conn = unp2;
	switch (so->so_type) {

	case SOCK_DGRAM:
		unp->unp_nextref = unp2->unp_refs;
		unp2->unp_refs = unp;
		soisconnected(so);
		break;

	case SOCK_STREAM:
	{
		int owner = 0;
		int owner2 = 0;

		unp2->unp_conn = unp;
		smp_unlock(&so->lk_socket);
		if (smp_owner(&so->so_head->lk_socket) ){
			owner = 1;
			smp_unlock(&so->so_head->lk_socket);
		}
		if (so2->so_head){
			owner2 = 1;
			smp_lock(&so2->so_head->lk_socket, LK_RETRY);
		}
		soisconnected_locked(so2);
		if (owner2){
			owner2 = 0;
			smp_unlock(&so2->so_head->lk_socket);
		}
		if (owner){
			owner = 0;
			smp_lock(&so->so_head->lk_socket, LK_RETRY);
		}
		smp_lock(&so->lk_socket, LK_RETRY);
		soisconnected(so);
		break;
	}

	default:
		panic("unp_connect2");
	}
	return (0);
}

/*
 * Functionallly the same as soisdisconnected() but here, we have the 
 * so->head locked going in (when it exists).
 */
soisconnected_locked(so)
	register struct socket *so;
{        
	register struct socket *head = so->so_head;

	if (head) {
		if (soqremque(so, 0) == 0)
	        	panic("soisconnected");
	        soqinsque(head, so, 1);
	        sorwakeup(head);
	        wakeup((caddr_t)&head->so_timeo);
	}
        so->so_state &= ~(SS_ISCONNECTING|SS_ISDISCONNECTING);
        so->so_state |= SS_ISCONNECTED;
        wakeup((caddr_t)&so->so_timeo);
        sorwakeup(so);
        sowwakeup(so);
}

unp_disconnect(unp)
	struct unpcb *unp;
{
	register struct unpcb *unp2 = unp->unp_conn;
	struct socket *so2;
	struct socket *so;
	int s;
	struct unpcb *temp_unp;
	struct unpcb *temp_unp_nextref;

        /* Bug fix.  3.21.89.us.  Change locking so that only
         * so or so2 is locked at any one time.  Point is to
         * make unp_disconnect behave like PRU_SEND and PRU_RCVD.
         */

	if (unp2 == 0)
		return;
	s = splnet();
	so = unp->unp_socket;
	so2 = unp2->unp_socket;
	unp->unp_conn = 0; 
	switch (unp->unp_socket->so_type) {

	case SOCK_DGRAM:
		{
		int owner2 = 0;
		struct socket *so3;

		temp_unp = unp;
		temp_unp_nextref = unp->unp_nextref;
		unp->unp_nextref = 0;
		unp->unp_socket->so_state &= ~SS_ISCONNECTED;
		smp_unlock(&so->lk_socket);
		if ( !smp_owner(&so2->lk_socket) ){
			owner2 = 1;
			smp_lock(&so2->lk_socket, LK_RETRY);
		}
		if (unp2->unp_refs == temp_unp){
			unp2->unp_refs = temp_unp_nextref;
		}
		else {
			smp_unlock(&so2->lk_socket);
			unp2 = unp2->unp_refs;
			so3 = unp2->unp_socket;
			smp_lock(&so3->lk_socket, LK_RETRY);
			for (;;) {
				if (unp2 == 0)
					panic("unp_disconnect");
				if (unp2->unp_nextref == temp_unp){
					smp_unlock(&so3->lk_socket);
					smp_lock(&so2->lk_socket, LK_RETRY);	
					break;
				}
				unp2 = unp2->unp_nextref;
				smp_unlock(&so3->lk_socket);
				so3 = unp2->unp_socket;
				smp_lock(&so3->lk_socket, LK_RETRY);
			}
			unp2->unp_nextref = temp_unp_nextref;
		}
		if (owner2){
			smp_unlock(&so2->lk_socket);	
			owner2 = 0;
		}
		smp_lock(&so->lk_socket, LK_RETRY);
		break;
		}

	case SOCK_STREAM:
		{
		int owner = 0;
		soisdisconnected(unp->unp_socket);
		smp_unlock(&so->lk_socket);
		if (smp_owner(&so->so_head) ){
			owner = 1;
			smp_unlock(&so->so_head->lk_socket);
		}
		smp_lock(&so2->lk_socket, LK_RETRY);
		unp2->unp_conn = 0;
		soisdisconnected(unp2->unp_socket);
		smp_unlock(&so2->lk_socket);
		if (owner){
			owner = 0;
			smp_lock(&so->so_head->lk_socket, LK_RETRY);
		}
		smp_lock(&so->lk_socket, LK_RETRY);
		break;
		}
	}
	splx(s);
}

#ifdef notdef
unp_abort(unp)
	struct unpcb *unp;
{

	unp_detach(unp);
}
#endif

/*ARGSUSED*/
unp_usrclosed(unp)
	struct unpcb *unp;
{

}

/*
 * SMP:
 * Called with a lock held.  Note that sofree will NOT do an unlock
 * HERE if unp_drop called (indirectly) from soclose.  Unlock will
 * be done in soclose's call to sofree.
 */
unp_drop(unp, errno)
	struct unpcb *unp;
	int errno;
{
	int owner = 0;
	struct socket *so = unp->unp_socket;
	
	if ( (so->so_head) && smp_owner(&so->so_head->lk_socket) ){
		smp_unlock(&so->so_head->lk_socket);
		owner = 1;
	}
	so->so_error = errno;

	if (!smp_owner(&lk_so_disconnect)) {
		smp_unlock(&so->lk_socket);
		smp_lock(&lk_so_disconnect,LK_RETRY);
		smp_lock(&so->lk_socket, LK_RETRY);
		unp_disconnect(unp);
		smp_unlock(&lk_so_disconnect);

	} else {
		unp_disconnect(unp);
	}
	if (owner && so->so_head){
		smp_unlock(&so->lk_socket);
		smp_lock(&so->so_head->lk_socket, LK_RETRY);
		smp_lock(&so->lk_socket, LK_RETRY);
		owner = 0;
	}
	if (so->so_head) {
		so->so_pcb = (caddr_t) 0;
		m_freem(unp->unp_addr);
		KM_FREE(unp, KM_PCB);
		sofree(so);
	}
}

#ifdef notdef
unp_drain()
{

}
#endif

unp_externalize(rights)
	struct mbuf *rights;
{
	int newfds = rights->m_len / sizeof (int);
	register int i;
	register struct file **rp = mtod(rights, struct file **);
	register struct file *fp;
	register int f;

	if (newfds > ufavail()) {
		for (i = 0; i < newfds; i++) {
			fp = *rp;
			unp_discard(fp);
			*rp++ = 0;
		}
		return (EMSGSIZE);
	}
	for (i = 0; i < newfds; i++) {
		f = ufalloc(0);
		if (f < 0)
			panic("unp_externalize");
		fp = *rp;
		U_OFILE_SET(f, fp);
		smp_lock(&fp->f_lk, LK_RETRY);
		fp->f_msgcount--;
		unp_rights--;
		*(int *)rp++ = f;
		smp_unlock(&fp->f_lk);
	}
	return (0);
}

unp_internalize(rights)
	struct mbuf *rights;
{
	register struct file **rp;
	register int oldfds = rights->m_len / sizeof (int);
	register int i;
	register struct file *fp;

	rp = mtod(rights, struct file **);
	for (i = 0; i < oldfds; i++)
		if (getf(*(int *)rp++) == 0)
			return (EBADF);
	rp = mtod(rights, struct file **);
	for (i = 0; i < oldfds; i++) {
		fp = getf(*(int *)rp);
		smp_lock(&fp->f_lk, LK_RETRY);
		*rp++ = fp;
		fp->f_count++;
		fp->f_msgcount++;
		unp_rights++;
		smp_unlock(&fp->f_lk);
	}
	return (0);
}


unp_dispose(m)
	struct mbuf *m;
{
	int unp_discard();

	if (m)
		unp_scan(m, unp_discard);
}

unp_scan(m0, op)
	register struct mbuf *m0;
	int (*op)();
{
	register struct mbuf *m;
	register struct file **rp;
	register int i;
	register int qfds;

	while (m0) {
		for (m = m0; m; m = m->m_next)
			if (m->m_type == MT_RIGHTS && m->m_len) {
				qfds = m->m_len / sizeof (struct file *);
				rp = mtod(m, struct file **);
				for (i = 0; i < qfds; i++)
					(*op)(*rp++);
				break;		/* XXX, but saves time */
			}
		m0 = m0->m_act;
	}
}


unp_discard(fp)
	register struct file *fp;
{

        smp_lock(&fp->f_lk, LK_RETRY);
	fp->f_msgcount--;
	unp_rights--;
	smp_unlock(&fp->f_lk);
	closef(fp);
}
