#ifndef lint
static	char	*sccsid = "@(#)rpc_prot.c	4.1	7/2/90";
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
 * rpc_prot.c
 *
 * This set of routines implements the rpc message definition,
 * its serializer and some common rpc utility routines.
 * The routines are meant for various implementations of rpc -
 * they are NOT for the rpc client or rpc service implementations!
 * Because authentication stuff is easy and is part of rpc, the opaque
 * routines are also in this program.
 *
 *	History:
 *
 * 12-11-87	Robin L. and Larry C. and Ricky P.
 *	Added new kmalloc memory allocation to system.
 *
 */

#ifdef KERNEL
#include "../h/param.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"
#include "../netinet/in.h"
#else
#include <sys/param.h>
#include "types.h"	/* <> */
#include "xdr.h"	/* <> */
#include "auth.h"	/* <> */
#include "clnt.h"	/* <> */
#include "rpc_msg.h"	/* <> */
#include <netinet/in.h>
#endif

/* * * * * * * * * * * * * * XDR Authentication * * * * * * * * * * * */

struct opaque_auth _null_auth;

/*
 * XDR an opaque authentication struct
 * (see auth.h)
 */
bool_t
xdr_opaque_auth(xdrs, ap)
	register XDR *xdrs;
	register struct opaque_auth *ap;
{

	if (xdr_enum(xdrs, &(ap->oa_flavor)))
		return (xdr_bytes(xdrs, &ap->oa_base,
			&ap->oa_length, MAX_AUTH_BYTES));
	return (FALSE);
}

#ifndef KERNEL
/*
 * XDR a DES key.
 */
bool_t
xdr_des_block(xdrs, blkp)
	register XDR *xdrs;
	register des_block *blkp;
{
	return (xdr_opaque(xdrs, (caddr_t)blkp, sizeof(des_block)));
}
#endif !KERNEL

/* * * * * * * * * * * * * * XDR RPC MESSAGE * * * * * * * * * * * * * * * */

/*
 * XDR the MSG_ACCEPTED part of a reply message union
 */
bool_t 
xdr_accepted_reply(xdrs, ar)
	register XDR *xdrs;   
	register struct accepted_reply *ar;
{

	/* personalized union, rather than calling xdr_union */
	if (! xdr_opaque_auth(xdrs, &(ar->ar_verf)))
		return (FALSE);
	if (! xdr_enum(xdrs, (enum_t *)&(ar->ar_stat)))
		return (FALSE);
	switch (ar->ar_stat) {

	case SUCCESS:
		return ((*(ar->ar_results.proc))(xdrs, ar->ar_results.where));
	
	case PROG_MISMATCH:
		if (! xdr_u_long(xdrs, &(ar->ar_vers.low)))
			return (FALSE);
		return (xdr_u_long(xdrs, &(ar->ar_vers.high)));
	}
	return (TRUE);  /* TRUE => open ended set of problems */
}

/*
 * XDR the MSG_DENIED part of a reply message union
 */
bool_t 
xdr_rejected_reply(xdrs, rr)
	register XDR *xdrs;
	register struct rejected_reply *rr;
{

	/* personalized union, rather than calling xdr_union */
	if (! xdr_enum(xdrs, (enum_t *)&(rr->rj_stat)))
		return (FALSE);
	switch (rr->rj_stat) {

	case RPC_MISMATCH:
		if (! xdr_u_long(xdrs, &(rr->rj_vers.low)))
			return (FALSE);
		return (xdr_u_long(xdrs, &(rr->rj_vers.high)));

	case AUTH_ERROR:
		return (xdr_enum(xdrs, (enum_t *)&(rr->rj_why)));
	}
	return (FALSE);
}

#define	RNDUP(x)  ((((x) + BYTES_PER_XDR_UNIT - 1) / BYTES_PER_XDR_UNIT) \
		   * BYTES_PER_XDR_UNIT)

static struct xdr_discrim reply_dscrm[3] = {
	{ (int)MSG_ACCEPTED, xdr_accepted_reply },
	{ (int)MSG_DENIED, xdr_rejected_reply },
	{ __dontcare__, NULL_xdrproc_t } };

/*
 * XDR a reply message
 */
bool_t
xdr_replymsg(xdrs, rmsg)
	register XDR *xdrs;
	register struct rpc_msg *rmsg;
{
	register long *buf;
	register struct accepted_reply *ar;
	register struct opaque_auth *oa;

	if (xdrs->x_op == XDR_ENCODE &&
	    rmsg->rm_reply.rp_stat == MSG_ACCEPTED &&
	    rmsg->rm_direction == REPLY &&
	    (buf = XDR_INLINE(xdrs, 6 * BYTES_PER_XDR_UNIT +
	    rmsg->rm_reply.rp_acpt.ar_verf.oa_length)) != NULL) {
		IXDR_PUT_LONG(buf, rmsg->rm_xid);
		IXDR_PUT_ENUM(buf, rmsg->rm_direction);
		IXDR_PUT_ENUM(buf, rmsg->rm_reply.rp_stat);
		ar = &rmsg->rm_reply.rp_acpt;
		oa = &ar->ar_verf;
		IXDR_PUT_ENUM(buf, oa->oa_flavor);
		IXDR_PUT_LONG(buf, oa->oa_length);
		if (oa->oa_length) {
			bcopy(oa->oa_base, buf, oa->oa_length);
			buf += (oa->oa_length +
				BYTES_PER_XDR_UNIT - 1) /
				sizeof (long);
		}
		/*
		 * stat and rest of reply, copied from xdr_accepted_reply
		 */
		IXDR_PUT_ENUM(buf, ar->ar_stat);
		switch (ar->ar_stat) {

		case SUCCESS:
			return ((*(ar->ar_results.proc))
				(xdrs, ar->ar_results.where));
	
		case PROG_MISMATCH:
			if (! xdr_u_long(xdrs, &(ar->ar_vers.low)))
				return (FALSE);
			return (xdr_u_long(xdrs, &(ar->ar_vers.high)));
		}
		return (TRUE);
	}
	if (xdrs->x_op == XDR_DECODE &&
	    (buf = XDR_INLINE(xdrs, 3 * BYTES_PER_XDR_UNIT)) != NULL) {
		rmsg->rm_xid = IXDR_GET_LONG(buf);
		rmsg->rm_direction = IXDR_GET_ENUM(buf, enum msg_type);
		if (rmsg->rm_direction != REPLY) {
			return (FALSE);
		}
		rmsg->rm_reply.rp_stat = IXDR_GET_ENUM(buf, enum reply_stat);
		if (rmsg->rm_reply.rp_stat != MSG_ACCEPTED) {
			if (rmsg->rm_reply.rp_stat == MSG_DENIED) {
				return (xdr_rejected_reply(xdrs,
					&rmsg->rm_reply.rp_rjct));
			}
			return (FALSE);
		}
		ar = &rmsg->rm_reply.rp_acpt;
		oa = &ar->ar_verf;
		buf = XDR_INLINE(xdrs, 2 * BYTES_PER_XDR_UNIT);
		if (buf != NULL) {
			oa->oa_flavor = IXDR_GET_ENUM(buf, enum_t);
			oa->oa_length = IXDR_GET_LONG(buf);
		} else {
			if (xdr_enum(xdrs, &oa->oa_flavor) == FALSE ||
			    xdr_u_int(xdrs, &oa->oa_length) == FALSE) {
				return (FALSE);
			}
		}
		if (oa->oa_length) {
			if (oa->oa_length > MAX_AUTH_BYTES) {
				return (FALSE);
			}
			if (oa->oa_base == NULL) {
			        mem_alloc(oa->oa_base, caddr_t, oa->oa_length, KM_RPC);
			}
			buf = XDR_INLINE(xdrs, RNDUP(oa->oa_length));
			if (buf == NULL) {
				if (xdr_opaque(xdrs, oa->oa_base,
				    oa->oa_length) == FALSE) {
					return (FALSE);
				}
			} else {
				bcopy(buf, oa->oa_base, oa->oa_length);
				/* no real need....
				buf += RNDUP(oa->oa_length) / sizeof (long);
				*/
			}
		}
		/*
		 * stat and rest of reply, copied from
		 * xdr_accepted_reply
		 */
		if (! xdr_enum(xdrs, (enum_t *)&ar->ar_stat)) {
			return(FALSE);
		}
		switch (ar->ar_stat) {

		case SUCCESS:
			return ((*(ar->ar_results.proc))
				(xdrs, ar->ar_results.where));

		case PROG_MISMATCH:
			if (! xdr_u_long(xdrs, &(ar->ar_vers.low)))
				return (FALSE);
			return (xdr_u_long(xdrs, &(ar->ar_vers.high)));
		}
		return (TRUE);
	}
	if (
	    xdr_u_long(xdrs, &(rmsg->rm_xid)) && 
	    xdr_enum(xdrs, (enum_t *)&(rmsg->rm_direction)) &&
	    (rmsg->rm_direction == REPLY) )
		return (xdr_union(xdrs, (enum_t *)&(rmsg->rm_reply.rp_stat),
		    (caddr_t)&(rmsg->rm_reply.ru), reply_dscrm, NULL_xdrproc_t));
	return (FALSE);
}

/*
 * XDR a call message
 */
bool_t
xdr_callmsg(xdrs, cmsg)
	register XDR *xdrs;
	register struct rpc_msg *cmsg;
{
	register long *buf;
	register struct opaque_auth *oa;

	if (xdrs->x_op == XDR_ENCODE) {
		if (cmsg->rm_call.cb_cred.oa_length > MAX_AUTH_BYTES) {
			return (FALSE);
		}
		if (cmsg->rm_call.cb_verf.oa_length > MAX_AUTH_BYTES) {
			return (FALSE);
		}
		buf = XDR_INLINE(xdrs, 8 * BYTES_PER_XDR_UNIT
			+ RNDUP(cmsg->rm_call.cb_cred.oa_length)
			+ 2 * BYTES_PER_XDR_UNIT
			+ RNDUP(cmsg->rm_call.cb_verf.oa_length));
		if (buf != NULL) {
			IXDR_PUT_LONG(buf, cmsg->rm_xid);
			IXDR_PUT_ENUM(buf, cmsg->rm_direction);
			if (cmsg->rm_direction != CALL) {
				return (FALSE);
			}
			IXDR_PUT_LONG(buf, cmsg->rm_call.cb_rpcvers);
			if (cmsg->rm_call.cb_rpcvers != RPC_MSG_VERSION) {
				return (FALSE);
			}
			IXDR_PUT_LONG(buf, cmsg->rm_call.cb_prog);
			IXDR_PUT_LONG(buf, cmsg->rm_call.cb_vers);
			IXDR_PUT_LONG(buf, cmsg->rm_call.cb_proc);
			oa = &cmsg->rm_call.cb_cred;
			IXDR_PUT_ENUM(buf, oa->oa_flavor);
			IXDR_PUT_LONG(buf, oa->oa_length);
			if (oa->oa_length) {
				bcopy(oa->oa_base, buf, oa->oa_length);
				buf += RNDUP(oa->oa_length) / sizeof (long);
			}
			oa = &cmsg->rm_call.cb_verf;
			IXDR_PUT_ENUM(buf, oa->oa_flavor);
			IXDR_PUT_LONG(buf, oa->oa_length);
			if (oa->oa_length) {
				bcopy(oa->oa_base, buf, oa->oa_length);
				/* no real need....
				buf += RNDUP(oa->oa_length) / sizeof (long);
				*/
			}
			return (TRUE);
		}
	}
	if (xdrs->x_op == XDR_DECODE) {
		buf = XDR_INLINE(xdrs, 8 * BYTES_PER_XDR_UNIT);
		if (buf != NULL) {
			cmsg->rm_xid = IXDR_GET_LONG(buf);
			cmsg->rm_direction = IXDR_GET_ENUM(buf, enum msg_type);
			if (cmsg->rm_direction != CALL) {
				return (FALSE);
			}
			cmsg->rm_call.cb_rpcvers = IXDR_GET_LONG(buf);
			if (cmsg->rm_call.cb_rpcvers != RPC_MSG_VERSION) {
				return (FALSE);
			}
			cmsg->rm_call.cb_prog = IXDR_GET_LONG(buf);
			cmsg->rm_call.cb_vers = IXDR_GET_LONG(buf);
			cmsg->rm_call.cb_proc = IXDR_GET_LONG(buf);
			oa = &cmsg->rm_call.cb_cred;
			oa->oa_flavor = IXDR_GET_ENUM(buf, enum_t);
			oa->oa_length = IXDR_GET_LONG(buf);
			if (oa->oa_length) {
				if (oa->oa_length > MAX_AUTH_BYTES) {
					return (FALSE);
				}
				if (oa->oa_base == NULL) {
			               mem_alloc(oa->oa_base, caddr_t, oa->oa_length, KM_RPC);
				}
				buf = XDR_INLINE(xdrs, RNDUP(oa->oa_length));
				if (buf == NULL) {
					if (xdr_opaque(xdrs, oa->oa_base,
					    oa->oa_length) == FALSE) {
						return (FALSE);
					}
				} else {
					bcopy(buf, oa->oa_base, oa->oa_length);
					/* no real need....
					buf += RNDUP(oa->oa_length) /
						sizeof (long);
					*/
				}
			}
			oa = &cmsg->rm_call.cb_verf;
			buf = XDR_INLINE(xdrs, 2 * BYTES_PER_XDR_UNIT);
			if (buf == NULL) {
				if (xdr_enum(xdrs, &oa->oa_flavor) == FALSE ||
				    xdr_u_int(xdrs, &oa->oa_length) == FALSE) {
					return (FALSE);
				}
			} else {
				oa->oa_flavor = IXDR_GET_ENUM(buf, enum_t);
				oa->oa_length = IXDR_GET_LONG(buf);
			}
			if (oa->oa_length) {
				if (oa->oa_length > MAX_AUTH_BYTES) {
					return (FALSE);
				}
				if (oa->oa_base == NULL) {
				       mem_alloc(oa->oa_base, caddr_t, oa->oa_length, KM_RPC);
				}
				buf = XDR_INLINE(xdrs, RNDUP(oa->oa_length));
				if (buf == NULL) {
					if (xdr_opaque(xdrs, oa->oa_base,
					    oa->oa_length) == FALSE) {
						return (FALSE);
					}
				} else {
					bcopy(buf, oa->oa_base, oa->oa_length);
					/* no real need...
					buf += RNDUP(oa->oa_length) /
						sizeof (long);
					*/
				}
			}
			return (TRUE);
		}
	}
	if (
	    xdr_u_long(xdrs, &(cmsg->rm_xid)) &&
	    xdr_enum(xdrs, (enum_t *)&(cmsg->rm_direction)) &&
	    (cmsg->rm_direction == CALL) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_rpcvers)) &&
	    (cmsg->rm_call.cb_rpcvers == RPC_MSG_VERSION) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_prog)) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_vers)) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_proc)) &&
	    xdr_opaque_auth(xdrs, &(cmsg->rm_call.cb_cred)) )
	    return (xdr_opaque_auth(xdrs, &(cmsg->rm_call.cb_verf)));
	return (FALSE);
}

/*
 * Serializes the "static part" of a call message header.
 * The fields include: rm_xid, rm_direction, rpcvers, prog, and vers.
 * The rm_xid is not really static, but the user can easily munge on the fly.
 */
bool_t
xdr_callhdr(xdrs, cmsg)
	register XDR *xdrs;
	register struct rpc_msg *cmsg;
{

	cmsg->rm_direction = CALL;
	cmsg->rm_call.cb_rpcvers = RPC_MSG_VERSION;
	if (
	    (xdrs->x_op == XDR_ENCODE) &&
	    xdr_u_long(xdrs, &(cmsg->rm_xid)) &&
	    xdr_enum(xdrs, (enum_t *)&(cmsg->rm_direction)) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_rpcvers)) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_prog)) )
	    return (xdr_u_long(xdrs, &(cmsg->rm_call.cb_vers)));
	return (FALSE);
}

/* ************************** Client utility routine ************* */

static void
accepted(acpt_stat, error)
	register enum accept_stat acpt_stat;
	register struct rpc_err *error;
{

	switch (acpt_stat) {

	case PROG_UNAVAIL:
		error->re_status = RPC_PROGUNAVAIL;
		return;

	case PROG_MISMATCH:
		error->re_status = RPC_PROGVERSMISMATCH;
		return;

	case PROC_UNAVAIL:
		error->re_status = RPC_PROCUNAVAIL;
		return;

	case GARBAGE_ARGS:
		error->re_status = RPC_CANTDECODEARGS;
		return;

	case SYSTEM_ERR:
		error->re_status = RPC_SYSTEMERROR;
		return;

	case SUCCESS:
		error->re_status = RPC_SUCCESS;
		return;
	}
	/* something's wrong, but we don't know what ... */
	error->re_status = RPC_FAILED;
	error->re_lb.s1 = (long)MSG_ACCEPTED;
	error->re_lb.s2 = (long)acpt_stat;
}

static void 
rejected(rjct_stat, error)
	register enum reject_stat rjct_stat;
	register struct rpc_err *error;
{

	switch (rjct_stat) {

	case RPC_VERSMISMATCH:
		error->re_status = RPC_VERSMISMATCH;
		return;

	case AUTH_ERROR:
		error->re_status = RPC_AUTHERROR;
		return;
	}
	/* something's wrong, but we don't know what ... */
	error->re_status = RPC_FAILED;
	error->re_lb.s1 = (long)MSG_DENIED;
	error->re_lb.s2 = (long)rjct_stat;
}

/*
 * given a reply message, fills in the error
 */
void
_seterr_reply(msg, error)
	register struct rpc_msg *msg;
	register struct rpc_err *error;
{

	/* optimized for normal, SUCCESSful case */
	switch (msg->rm_reply.rp_stat) {

	case MSG_ACCEPTED:
		if (msg->acpted_rply.ar_stat == SUCCESS) {
			error->re_status = RPC_SUCCESS;
			return;
		};
		accepted(msg->acpted_rply.ar_stat, error);
		break;

	case MSG_DENIED:
		rejected(msg->rjcted_rply.rj_stat, error);
		break;

	default:
		error->re_status = RPC_FAILED;
		error->re_lb.s1 = (long)(msg->rm_reply.rp_stat);
		break;
	}
	switch (error->re_status) {

	case RPC_VERSMISMATCH:
		error->re_vers.low = msg->rjcted_rply.rj_vers.low;
		error->re_vers.high = msg->rjcted_rply.rj_vers.high;
		break;

	case RPC_AUTHERROR:
		error->re_why = msg->rjcted_rply.rj_why;
		break;

	case RPC_PROGVERSMISMATCH:
		error->re_vers.low = msg->acpted_rply.ar_vers.low;
		error->re_vers.high = msg->acpted_rply.ar_vers.high;
		break;
	}
}
