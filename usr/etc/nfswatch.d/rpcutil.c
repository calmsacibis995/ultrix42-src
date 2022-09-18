#ifndef lint
static char *sccsid = "@(#)rpcutil.c	4.2	(ULTRIX)	1/25/91";
#endif
/* Based on:
 * RCSid = "$Header: /sparky/a/davy/system/nfswatch/RCS/rpcutil.c,v 3.0 91/01/23 08:23:22 davy Exp $";
 */

/*
 * rpcutil.c - routines for emulating RPC library functions without really
 *	       receiving packets.
 *
 * David A. Curry				Jeffrey C. Mogul
 * SRI International				Digital Equipment Corporation
 * 333 Ravenswood Avenue			Western Research Laboratory
 * Menlo Park, CA 94025				100 Hamilton Avenue
 * davy@erg.sri.com				Palo Alto, CA 94301
 *						mogul@decwrl.dec.com
 *
 * $Log:	rpcutil.c,v $
 * Revision 3.0  91/01/23  08:23:22  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.2  90/08/17  15:47:46  davy
 * NFSWATCH Version 2.0.
 * 
 * Revision 1.1  88/11/29  11:20:57  davy
 * NFSWATCH Release 1.0
 * 
 */
#include <sys/param.h>
#include <netinet/in.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/svc.h>
#include <errno.h>
#include <stdio.h>

#define NFSSERVER	1

#ifdef sun
#include <sys/vfs.h>
#endif /* sun */
#ifdef ultrix
#include <sys/types.h>
#include <sys/time.h>
#endif /* ultrix */
#include <nfs/nfs.h>

#include "nfswatch.h"
#include "externs.h"
#include "rpcdefs.h"

/*
 * Operations on the SVCXPRT structure.  We're only going to use
 * the one to get arguments from it.
 */
static struct xp_ops xp_ops = {
	NULL, NULL, rpcxdr_getargs, NULL, NULL, NULL
};

static SVCXPRT	*xprt;		/* the service description		*/

/*
 * setup_rpcxdr - set up for decoding RPC XDR stuff.  Sort of a svcudp_create
 *		  without the socket code.
 */
void
setup_rpcxdr()
{
	register struct svcudp_data *su;

	/*
	 * Allocate the SVCXPRT structure.
	 */
	if ((xprt = (SVCXPRT *) malloc(sizeof(SVCXPRT))) == NULL) {
		(void) fprintf(stderr, "%s: out of memory.\n", pname);
		finish(-1);
	}

	/*
	 * Allocate UDP service data.
	 */
	if ((su = (struct svcudp_data *) malloc(sizeof(struct svcudp_data))) == NULL) {
		(void) fprintf(stderr, "%s: out of memory.\n", pname);
		finish(-1);
	}

	/*
	 * This is the maximum size of a packet.
	 */
	su->su_iosz = ((UDPMSGSIZE + 3) / 4) * 4;

	/*
	 * Get a buffer to store stuff in.
	 */
	if ((rpc_buffer(xprt) = (char *) malloc(su->su_iosz)) == NULL) {
		(void) fprintf(stderr, "%s: out of memory.\n", pname);
		finish(-1);
	}

	/*
	 * Fill in the SVCXPRT structure.  This is a standard RPC routine.
	 */
	(void) xdrmem_create(&(su->su_xdrs), rpc_buffer(xprt), su->su_iosz,
		XDR_DECODE);

	xprt->xp_ops = &xp_ops;
	xprt->xp_p2 = (caddr_t) su;
	xprt->xp_verf.oa_base = su->su_verfbody;
}

/*
 * udprpc_recv - pretend we've received an RPC packet - this is sort of like
 *		 svcudp_recv.
 */
int udprpc_recv(data, length, msg, xp)
register struct rpc_msg *msg;
register u_int length;
register char *data;
SVCXPRT **xp;
{
	register XDR *xdrs;
	register struct svcudp_data *su;

	su = su_data(xprt);
	xdrs = &(su->su_xdrs);

	/*
	 * Too short.
	 */
	if (length < (4 * sizeof(u_long)))
		return(FALSE);

	if (length > truncation)
		length = truncation;

	/*
	 * Copy the data.
	 */
	(void) bcopy(data, rpc_buffer(xprt), min(length, su->su_iosz));

	xdrs->x_op = XDR_DECODE;

	/*
	 * Set the XDR routines to the start of the buffer.
	 */
	(void) XDR_SETPOS(xdrs, 0);

	/*
	 * Decode the RPC message structure.
	 */
	if (!xdr_callmsg(xdrs, msg))
		return(FALSE);

	su->su_xid = msg->rm_xid;
	*xp = xprt;

	return(TRUE);
}

/*
 * rpcxdr_getargs - called by SVC_GETARGS.
 */
static bool_t
rpcxdr_getargs(xprt, xdr_args, args_ptr)
register xdrproc_t xdr_args;
register caddr_t args_ptr;
register SVCXPRT *xprt;
{
	return((*xdr_args)(&(su_data(xprt)->su_xdrs), args_ptr));
}
