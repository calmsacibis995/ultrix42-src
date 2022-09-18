
# ifndef lint
static char *sccsid = "@(#)prot_freeall.c	4.1	(ULTRIX)	7/2/90";
# endif not lint

/****************************************************************
 *								*
 *  Licensed to Digital Equipment Corporation, Maynard, MA	*
 *		Copyright 1985 Sun Microsystems, Inc.		*
 *			All rights reserved.			*
 *								*
 ****************************************************************/
/**/
/*
 *	Modification history:
 *	~~~~~~~~~~~~~~~~~~~~
 *
 *	revision			comments
 *	--------	-----------------------------------------------
 *
 *	01-Jun-89	Fred Glover
 *			New file from nfssrc 4.0
 *
 *
 *	
 */

/*
 * prot_freeall.c consists of subroutines that implement the
 * DOS-compatible file sharing services for PC-NFS
 */

#include <stdio.h>
#include <sys/file.h>
#include "prot_lock.h"
#include "priv_prot.h"


extern int debug;
extern int grace_period;
extern char *xmalloc();
extern void xfree();
extern void zap_all_locks_for();
extern bool_t obj_cmp();
char *malloc();

void *
proc_nlm_freeall(Rqstp, Transp)
	struct svc_req *Rqstp;
	SVCXPRT *Transp;
{
	nlm_notify	req;
/*
 * Allocate space for arguments and decode them
 */

	req.name = NULL;
	if (!svc_getargs(Transp, xdr_nlm_notify, &req)) {
		svcerr_decode(Transp);
		return;
	}

	if (debug) {
		printf("proc_nlm_freeall from %s\n",
			req.name);
	}
	destroy_client_shares(req.name);
	zap_all_locks_for(req.name);

	free(req.name);
	svc_sendreply(Transp, xdr_void, NULL);
}
