#ifndef lint
static	char	*sccsid = "@(#)klm_lockmgr.c	4.2	10/8/90";
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
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1986 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */

/*
 *	Modification History
 *
 *
 * 08-Sep-90 Fred Glover
 *	Transfer complete fhandle definition into lockhandle construction
 *	in klm_drlock ()
 *
 * 08-Aug-89 Fred Glover
 *	Reduce remapping calls due to RPC timeouts based upon excessive
 *	time to process multiple lock request to same file
 *
 * 01-Jun-89 Fred Glover
 *	Update for nfssrc 4.0 data structure changes in klm_prot.h
 *
 * 09-Oct-88 condylis
 *	Added SMP locking for locking transaction id
 *
 *  28-09-88 Fred Glover
 *      Add transction id code for kernel/lockd communication
 *
 * 10-Jun-88 -- jaw 
 * 	add parameter to ISSIG for SMP.... this makes going to stop
 *	state atomic.
 *
 *  19-01-88 Fred Glover
 *	Add klm_drlock function to support 
 *	  ufs region locking when daemon based 
 *	  locking is enabled.
 */

/*
 * Kernel<->Network Lock-Manager Interface
 *
 * File- and Record-locking requests are forwarded (via RPC) to a
 * Network Lock-Manager running on the local machine.  The protocol
 * for these transactions is defined in /usr/src/protocols/klm_prot.x
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/stat.h"

/* files included by <rpc/rpc.h> */
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"

#include "../rpcsvc/klm_prot.h"
#include "../net/if.h"
#include "../fs/nfs/nfs.h"
#include "../fs/nfs/nfs_clnt.h"

#include "../h/mount.h"
#include "../fs/nfs/vfs.h"
#include "../fs/nfs/vnode.h"
#include "../rpc/lockmgr.h"

/* @JAA this is from sun netinet/in.h */
#define INADDR_LOOPBACK 0x7f000001

static struct sockaddr_in lm_sa;	/* talk to portmapper & lock-manager */

static talk_to_lockmgr();

extern int wakeup();

/* SMP lock for accessing client handle xid */
extern struct	lock_t	lk_rpcxid;

int klm_debug = 0;

/*
 *   Define parameters for run-time tuning:
 *
 *   Note that the policy for retries and timeouts has been modified
 *   and no longer matches the NFSSRC 4.0 reference code.  The modifications
 *   were made to resolve the V3.1 ULTRIX CLD associated with local 
 *   lock daemon wakeups:  i.e. the lock daemon cannot wake up
 *   a process which has been blocked waiting for a lock to be 
 *   released;  a kernel "retry" must take place.  Thus, the retry
 *   mechanism must retry frequently enough to be responsive in this
 *   case, yet not result in excessive overhead.  This code was modified
 *   such that:
 *		a) the process sleeps for klm_working_timeout seconds
 *		b) each retry uses a new RPC transaction number
 *		c) the general backoff algorithm in callit is avoided, 
 *		      which resulted in long delays once a process was
 *		      blocked waiting for a lock
 *
 *    The basic strategy is to avoid the remap loop when the lock daemon
 *	is present, and to avoid the backoff as long as communcation
 *	with the lock daemon is in progress
 */

int klm_backoff_timeout = 10;	/* time to wait on klm_denied_nolocks */
int klm_first_retry     =  1;		/* first attempt if klm port# known */
int klm_first_timeout   =  1;
int klm_normal_timeout  =  1;
int klm_working_retry   =  1;		/* attempts after klm_working */
int klm_working_timeout =  1;
int klm_remap_retry     =  1;
int klm_timeout_retry   = 20;

int klm_reqs=0;
int klm_succ_out=0;
int klm_succ_work=0;
int klm_to=0;
int klm_other=0;

/* Define lock mgr/Kernel transaction id variable */

int lm_xid=0;

/*
 * klm_lockctl - process a lock/unlock/test-lock request
 *
 * Calls (via RPC) the local lock manager to register the request.
 * Lock requests are cancelled if interrupted by signals.
 */
klm_lockctl(lh, ld, cmd, cred)
	register lockhandle_t *lh;
	register struct flock *ld;
	int cmd;
	struct ucred *cred;
{
	register int	error;
	klm_lockargs	args;
	klm_testrply	reply;
	u_long		xdrproc;
	xdrproc_t	xdrargs;
	xdrproc_t	xdrreply;
	extern struct   timeval time;

	klm_reqs++;

	/* initialize sockaddr_in used to talk to local processes */
	if (lm_sa.sin_port == 0) {
#ifdef notdef
		struct ifnet	*ifp;

		if ((ifp = if_ifwithafup(AF_INET)) == (struct ifnet *)NULL) {
			panic("klm_lockctl: no inet address");
		}
		lm_sa = *(struct sockaddr_in *) &(ifp->if_addr);
#else
		lm_sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		lm_sa.sin_family = AF_INET;
#endif
	}

	args.block = FALSE;
	args.exclusive = FALSE;
	args.alock.fh.n_bytes = (char *)&lh->lh_id;
	args.alock.fh.n_len = sizeof (lh->lh_id);
	args.alock.server_name = lh->lh_servername;
	args.alock.pid = (int)u.u_procp->p_pid;
	args.alock.l_offset = ld->l_start;
	args.alock.l_len = ld->l_len;
	xdrproc = KLM_LOCK;
	xdrargs = (xdrproc_t)xdr_klm_lockargs;
	xdrreply = (xdrproc_t)xdr_klm_stat;

	/* now modify the lock argument structure for specific cases */
	switch (ld->l_type) {
	case F_WRLCK:
		args.exclusive = TRUE;
		break;
	case F_UNLCK:
		xdrproc = KLM_UNLOCK;
		xdrargs = (xdrproc_t)xdr_klm_unlockargs;
		break;
	}

	switch (cmd) {
	case F_SETLKW:
		args.block = TRUE;
		break;
	case F_GETLK:
		xdrproc = KLM_TEST;
		xdrargs = (xdrproc_t)xdr_klm_testargs;
		xdrreply = (xdrproc_t)xdr_klm_testrply;
		break;
	}

requestloop:

        if (klm_debug) {
                printf("reqlp: pid:%d proc:%d %d %d\n",
                        args.alock.pid, (int) xdrproc, time.tv_sec, lm_xid);
        }

	/* send the request out to the local lock-manager and wait for reply */
	error = talk_to_lockmgr(xdrproc, xdrargs, &args, xdrreply, &reply, cred);
	if (error == ENOLCK) {
		goto ereturn;	/* no way the request could have gotten out */
	}

	/*
	 * The only other possible return values are:
	 *   klm_granted  |  klm_denied  | klm_denied_nolocks |  EINTR |
	 *   klm_would_deadlock
	 */
	switch (xdrproc) {
	case KLM_LOCK:
		switch (error) {
		case klm_granted:
			error = 0;		/* got the requested lock */
			goto ereturn;
		case klm_denied:
			if (args.block) {
				goto requestloop;	/* loop forever */
			}
			error = EACCES;		/* EAGAIN?? */
			goto ereturn;
		case klm_denied_nolocks:
			error = ENOLCK;		/* no resources available?! */
			goto ereturn;
		case EINTR:
			if (args.block)
				goto cancel;	/* cancel blocking locks */
			else
				goto requestloop;	/* loop forever */
		case klm_would_deadlock:
			error = EDEADLK;	/* blocking would result in */
			goto ereturn;           /* deadlock condition */

		}

	case KLM_UNLOCK:
		switch (error) {
		case klm_granted:
			error = 0;
			goto ereturn;
		case klm_denied:
			error = EINVAL;
			goto ereturn;
		case klm_denied_nolocks:
			goto nolocks_wait;	/* back off; loop forever */
		case EINTR:
			goto requestloop;	/* loop forever */
		}

	case KLM_TEST:
		switch (error) {
		case klm_granted:
			ld->l_type = F_UNLCK;	/* mark lock available */
			error = 0;
			goto ereturn;
		case klm_denied:
			ld->l_type = (reply.klm_testrply_u.holder.exclusive) ?
			    F_WRLCK : F_RDLCK;
			ld->l_start = reply.klm_testrply_u.holder.l_offset;
			ld->l_len = reply.klm_testrply_u.holder.l_len;
			ld->l_pid = reply.klm_testrply_u.holder.svid;
			error = 0;
			goto ereturn;
		case klm_denied_nolocks:
			goto nolocks_wait;	/* back off; loop forever */
		case EINTR:
			/* may want to take a longjmp here */
			goto requestloop;	/* loop forever */
		}
	}

/*NOTREACHED*/
nolocks_wait:
	timeout(wakeup, (caddr_t)&lm_sa, (klm_backoff_timeout * hz));
	(void) sleep((caddr_t)&lm_sa, PZERO|PCATCH);
	untimeout(wakeup, (caddr_t)&lm_sa);
	goto requestloop;	/* now try again */

cancel:

	if(klm_debug)
	          printf("klm_lockctl: request canceled for pid:%d\n",
				args.alock.pid);

	/*
	 * If we get here, a signal interrupted a rqst that must be cancelled.
	 * Change the procedure number to KLM_CANCEL and reissue the exact same
	 * request.  Use the results to decide what return value to give.
	 */

	xdrproc = KLM_CANCEL;
	error = talk_to_lockmgr(xdrproc,xdrargs, &args, xdrreply, &reply, cred);
	switch (error) {
	case klm_granted:
		error = 0;		/* lock granted */
		goto ereturn;
	case klm_denied:
		/* may want to take a longjmp here */
		error = EINTR;
		goto ereturn;
	case EINTR:
		goto cancel;	/* ignore signals til cancel succeeds */

	case klm_denied_nolocks:
		error = ENOLCK;		/* no resources available?! */
		goto ereturn;
	case ENOLCK:
		goto ereturn;
	}
/*NOTREACHED*/
ereturn:

	if(klm_debug)
	  printf("klm_lockctl: returning: err: %d\n", error);

	return(error);
}


/*
 * Send the given request to the local lock-manager.
 * If timeout or error, go back to the portmapper to check the port number.
 * This routine loops forever until one of the following occurs:
 *	1) A legitimate (not 'klm_working') reply is returned (returns 'stat').
 *
 *	2) A signal occurs (returns EINTR).  In this case, at least one try
 *	   has been made to do the RPC; this protects against jamming the
 *	   CPU if a KLM_CANCEL request has yet to go out.
 *
 *	3) A drastic error occurs (e.g., the local lock-manager has never
 *	   been activated OR cannot create a client-handle) (returns ENOLCK).
 */
static
talk_to_lockmgr(xdrproc, xdrargs, args, xdrreply, reply, cred)
	u_long xdrproc;
	xdrproc_t xdrargs;
	klm_lockargs *args;
	xdrproc_t xdrreply;
	klm_testrply *reply;
	struct ucred *cred;
{
	register CLIENT *client;
	struct timeval tmo;
	register int error;
	register int retries;

	/* set up a client handle to talk to the local lock manager */
	client = clntkudp_create(&lm_sa, (u_long)KLM_PROG, (u_long)KLM_VERS,
	    klm_first_retry, cred);
	if (client == (CLIENT *) NULL) {
		return(ENOLCK);
	}
	tmo.tv_sec = klm_first_timeout;
	tmo.tv_usec = 0;

	/* init and manage xid on behalf of lockd/kernel communication */

	smp_lock(&lk_rpcxid, LK_RETRY);
	if (!lm_xid)
		lm_xid = time.tv_sec;
	client->cl_xid = lm_xid++;
	smp_unlock(&lk_rpcxid);

	/*
	 * If cached port number, go right to CLNT_CALL().
	 * This works because timeouts go back to the portmapper to
	 * refresh the port number.
	 */
	if (lm_sa.sin_port != 0) {
		retries = klm_timeout_retry;
		goto retryloop;		/* skip first portmapper query */
	}

	for (;;) {
remaploop:

                if (klm_debug) {
                        printf ("remap: pid:%d proc:%d %d %d\n",
                                args->alock.pid, (int) xdrproc,
                                time.tv_sec, lm_xid);
                }

		/* go get the port number from the portmapper...
		 * if return 1, signal was received before portmapper answered;
		 * if return -1, the lock-manager is not registered
		 * else, got a port number
		 */
		switch(getport_loop(&lm_sa, (u_long)KLM_PROG, (u_long)KLM_VERS,
		(u_long)KLM_PROTO)) {
		case 1:
			error = EINTR;		/* signal interrupted things */
			goto out;

		case -1:
			error = ENOLCK;
			goto out;
		}

		/*
		 * If a signal occurred, pop back out to the higher
		 * level to decide what action to take.  If we just
		 * got a port number from the portmapper, the next
		 * call into this subroutine will jump to retryloop.
		 */
		if (ISSIG(u.u_procp,0)) {
			error = EINTR;
			goto out;
		}

		/* reset the lock-manager client handle */
		(void) clntkudp_init(client, &lm_sa, 
			klm_remap_retry, cred);
		tmo.tv_sec = klm_normal_timeout;
		retries    = klm_timeout_retry;

retryloop:

                if (klm_debug) {
                        printf ("retry: pid:%d proc:%d %d %d %d\n",
                                args->alock.pid, (int) xdrproc, time.tv_sec,
                                lm_xid, retries);
                }

		/* retry the request until completion, timeout, or error */
		for (;;) {

			smp_lock(&lk_rpcxid, LK_RETRY);
			client->cl_xid = lm_xid++;
			smp_unlock(&lk_rpcxid);

			error = (int) CLNT_CALL(client, xdrproc, xdrargs,
			    (caddr_t)args, xdrreply, (caddr_t)reply, tmo);

			if (klm_debug)
printf("retry: rec'd: pid:%d proce:%d [off:%d,len:%d] err:%d rs:%d %d %d\n",
				    args->alock.pid,
				    (int) xdrproc,
				    args->alock.l_offset,
				    args->alock.l_len,
				    error, (int) reply->stat, time.tv_sec,
					lm_xid);

			switch (error) {
			case RPC_SUCCESS:
				error = (int) reply->stat;
				if (error == (int) klm_working) {
					if (ISSIG(u.u_procp,0)) {
						error = EINTR;
						goto out;
					}
					/* lock-mgr is up...can wait longer */
					(void) clntkudp_init(client, &lm_sa,
					    klm_working_retry, cred);
					tmo.tv_sec = klm_working_timeout;
					retries    = klm_timeout_retry;
					klm_succ_work++;
					continue;	/* retry */
				}
				klm_succ_out++;
				goto out;	/* got a legitimate answer */

			case RPC_TIMEDOUT:

				if(klm_debug)
					printf("retry: RPC_TIMEDOUT:pid:%d retries:%d\n",
						args->alock.pid, retries);

				klm_to++;

				if (--retries > 0)
					continue;
				else
					goto remaploop;	/* ask for port# again */

			default:


				klm_other++;

				if (klm_debug) 
			printf("lock-manager: RPC error:%d sleeping:%d\n",
				  error, (klm_normal_timeout*hz));

				/* on RPC error, wait a bit and try again */
				timeout(wakeup, (caddr_t)&lm_sa,
				    (klm_normal_timeout * hz));
				error = sleep((caddr_t)&lm_sa,PZERO|PCATCH);
				untimeout(wakeup, (caddr_t)&lm_sa);
				if (error) {
				    error = EINTR;
				    goto out;
				}
				goto remaploop;	/* ask for port# again */
	    
			} /*switch*/

		} /*for*/	/* loop until timeout, error, or completion */
	} /*for*/		/* loop until signal or completion */

out:
	AUTH_DESTROY(client->cl_auth);	/* drop the authenticator */
	CLNT_DESTROY(client);		/* drop the client handle */
	return(error);
}

/*
 * Pass Record-locking requests to the local Lock-Manager daemon.
 *
 *	Note: this routine is called from ufs_rlock () as a result of an 
 *	      attempt to lock a local file AND
 * 			WHEN nfs is configured AND the daemon based
 *			region locking functionality 
 *			is enabled (kernel_locking = 0)
 */

int
klm_drlock(gp, ld, cmd, cred)
	struct gnode *gp;
	struct flock *ld;
	int cmd;
	struct ucred *cred;
{
	lockhandle_t lh;
	fhandle_t    fh;

	if(u.u_error = makefh(&fh, gp))
		return;
	lh.lh_id = fh;
	lh.lh_gp = gp;
	lh.lh_servername = hostname;
	return (klm_lockctl(&lh, ld, cmd, cred));
 
}
