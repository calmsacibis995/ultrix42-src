#ifndef lint
static char *sccsid = "@(#)svc_tcp.c	4.2      ULTRIX  2/28/91";
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
/*	Change Log
 * 
 *	02/21/91	jhw	Picked up SUN V4.0 select code changes
 *
 * 	04/13/89	jhw 	Integration to SUN 4.0
 *
 *	08/04/89	jhw	removed bindresvport usage for V3.0 binary
 *				compatibility
 */

/*
 * svc_tcp.c, Server side for TCP/IP based RPC. 
 *
 *
 * Actually implements two flavors of transporter -
 * a tcp rendezvouser (a listner and connection establisher)
 * and a record/tcp stream.
 */

#include <stdio.h>
#include <rpc/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/svc.h>
extern bool_t abort();
extern errno;

/*
 * Ops vector for TCP/IP based rpc service handle
 */
static bool_t		svctcp_recv();
static enum xprt_stat	svctcp_stat();
static bool_t		svctcp_getargs();
static bool_t		svctcp_reply();
static bool_t		svctcp_freeargs();
static void		svctcp_destroy();

static struct xp_ops svctcp_op = {
	svctcp_recv,
	svctcp_stat,
	svctcp_getargs,
	svctcp_reply,
	svctcp_freeargs,
	svctcp_destroy
};

/*
 * Ops vector for TCP/IP rendezvous handler
 */
static bool_t		rendezvous_request();
static enum xprt_stat	rendezvous_stat();

static struct xp_ops svctcp_rendezvous_op = {
	rendezvous_request,
	rendezvous_stat,
	abort,
	abort,
	abort,
	svctcp_destroy
};

static int readtcp(), writetcp();
static SVCXPRT *makefd_xprt();

struct tcp_rendezvous { /* kept in xprt->xp_p1 */
	u_int sendsize;
	u_int recvsize;
};

struct tcp_conn {  /* kept in xprt->xp_p1 */
	enum xprt_stat strm_stat;
	u_long x_id;
	XDR xdrs;
	char verf_body[MAX_AUTH_BYTES];
};

/*
 * Usage:
 *	xprt = svctcp_create(sock, send_buf_size, recv_buf_size);
 *
 * Creates, registers, and returns a (rpc) tcp based transporter.
 * Once *xprt is initialized, it is registered as a transporter
 * see (svc.h, xprt_register).  This routine returns
 * a NULL if a problem occurred.
 *
 * If sock<0 then a socket is created, else sock is used.
 * If the socket, sock is not bound to a port then svctcp_create
 * binds it to an arbitrary port.  The routine then starts a tcp
 * listener on the socket's associated port.  In any (successful) case,
 * xprt->xp_sock is the registered socket number and xprt->xp_port is the
 * associated port number.
 *
 * Since tcp streams do buffered io similar to stdio, the caller can specify
 * how big the send and receive buffers are via the second and third parms;
 * 0 => use the system default.
 */
SVCXPRT *
svctcp_create(sock, sendsize, recvsize)
	register int sock;
	u_int sendsize;
	u_int recvsize;
{
	bool_t madesock = FALSE;
	register SVCXPRT *xprt;
	register struct tcp_rendezvous *r;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);

	if (sock == RPC_ANYSOCK) {
		if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
			perror("svctcp_.c - udp socket creation problem");
			return ((SVCXPRT *)NULL);
		}
		madesock = TRUE;
	}
	addr.sin_addr.s_addr = 0;
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
/*	REMOVED FOR V3.0 BINARY COMPATIBILTY			*/
/*       if (bindresvport(sock, &addr)) {			*/
/*	        addr.sin_port = 0;				*/
/*	        (void)bind(sock, (struct sockaddr *)&addr, len);*/
/*       }							*/
        (void)bind(sock, (struct sockaddr *)&addr, len);
	if ((getsockname(sock, (caddr_t)&addr, &len) != 0)  ||
	    (listen(sock, 2) != 0)) {
		perror("svctcp_.c - cannot getsockname or listen");
		if (madesock)
		       (void)close(sock);
		return ((SVCXPRT *)NULL);
	}
	r = (struct tcp_rendezvous *)mem_alloc(0, 0, sizeof(*r), 0);
	if (r == NULL) {
		fprintf(stderr, "svctcp_create: out of memory\n");
		return (NULL);
	}
	r->sendsize = sendsize;
	r->recvsize = recvsize;
	xprt = (SVCXPRT *)mem_alloc(0, 0, sizeof(SVCXPRT), 0);
	if (xprt == NULL) {
		fprintf(stderr, "svctcp_create: out of memory\n");
		return (NULL);
	}
	xprt->xp_p2 = NULL;
	xprt->xp_p1 = (caddr_t)r;
	xprt->xp_verf = _null_auth;
	xprt->xp_ops = &svctcp_rendezvous_op;
	xprt->xp_port = ntohs(addr.sin_port);
	xprt->xp_sock = sock;
	xprt_register(xprt);
	return (xprt);
}

/*
 * Like svtcp_create(), except the routine takes any *open* UNIX file
 * descriptor as its first input.
 */
SVCXPRT *
svcfd_create(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{

	return (makefd_xprt(fd, sendsize, recvsize));
}

static SVCXPRT *
makefd_xprt(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{
	register SVCXPRT *xprt;
	register struct tcp_conn *cd;
 
	xprt = (SVCXPRT *)mem_alloc(0, 0, sizeof(SVCXPRT), 0);
	if (xprt == (SVCXPRT *)NULL) {
		fprintf(stderr, "svc_tcp: makefd_xprt: out of memory\n");
		goto done;
	}
	cd = (struct tcp_conn *)mem_alloc(0, 0, sizeof(struct tcp_conn), 0);
	if (cd == (struct tcp_conn *)NULL) {
		fprintf(stderr, "svc_tcp: makefd_xprt: out of memory\n");
		mem_free(xprt, sizeof(SVCXPRT));
		xprt = (SVCXPRT *)NULL;
		goto done;
	}
	cd->strm_stat = XPRT_IDLE;
	xdrrec_create(&(cd->xdrs), sendsize, recvsize,
	    (caddr_t)xprt, readtcp, writetcp);
	xprt->xp_p2 = NULL;
	xprt->xp_p1 = (caddr_t)cd;
	xprt->xp_verf.oa_base = cd->verf_body;
	xprt->xp_addrlen = 0;
	xprt->xp_ops = &svctcp_op;  /* truely deals with calls */
	xprt->xp_port = 0;  /* this is a connection, not a rendezvouser */
	xprt->xp_sock = fd;
	xprt_register(xprt);
    done:
	return (xprt);
}

static bool_t
rendezvous_request(xprt)
	register SVCXPRT *xprt;
{
	int sock;
	struct tcp_rendezvous *r;
	struct sockaddr_in addr;
	int len;

	r = (struct tcp_rendezvous *)xprt->xp_p1;
    again:
	len = sizeof(struct sockaddr_in);
	if ((sock = accept(xprt->xp_sock, (struct sockaddr *)&addr,
	    &len)) < 0) {
		if (errno == EINTR)
			goto again;
	       return (FALSE);
	}
	/*
	 * make a new transporter (re-uses xprt)
	 */
	xprt = makefd_xprt(sock, r->sendsize, r->recvsize);
	xprt->xp_raddr = addr;
	xprt->xp_addrlen = len;
	return (FALSE); /* there is never an rpc msg to be processed */
}

static enum xprt_stat
rendezvous_stat()
{

	return (XPRT_IDLE);
}

static void
svctcp_destroy(xprt)
	register SVCXPRT *xprt;
{
	register struct tcp_conn *cd = (struct tcp_conn *)xprt->xp_p1;

	xprt_unregister(xprt);
	(void)close(xprt->xp_sock);
	if (xprt->xp_port != 0) {
		/* a rendezvouser socket */
		xprt->xp_port = 0;
	} else {
		/* an actual connection socket */
		XDR_DESTROY(&(cd->xdrs));
	}
	mem_free((caddr_t)cd, sizeof(struct tcp_conn));
	mem_free((caddr_t)xprt, sizeof(SVCXPRT));
}

/*
 * All read operations timeout after 35 seconds.
 * A timeout is fatal for the connection.
 */
static struct timeval wait_per_try = { 35, 0 };

/*
 * reads data from the tcp conection.
 * any error is fatal and the connection is closed.
 * (And a read of zero bytes is a half closed stream => error.)
 */
static int
readtcp(xprt, buf, len)
	register SVCXPRT *xprt;
	caddr_t buf;
	register int len;
{
	register int sock = xprt->xp_sock;
        fd_set mask;
        fd_set readfds;

        FD_ZERO(&mask);
        FD_SET(sock, &mask);
	do {
		readfds = mask;
		if (select(_rpc_dtablesize(), &readfds, NULL, NULL, &wait_per_try) <= 0) {
			if (errno == EINTR)
				continue;
			goto fatal_err;
		}
       } while (!FD_ISSET(sock, &readfds));
        if ((len = read(sock, buf, len)) > 0) {
                return (len);
        }
fatal_err:
	((struct tcp_conn *)(xprt->xp_p1))->strm_stat = XPRT_DIED;
	return (-1);
}

/*
 * writes data to the tcp connection.
 * Any error is fatal and the connection is closed.
 */
static int
writetcp(xprt, buf, len)
	register SVCXPRT *xprt;
	caddr_t buf;
	int len;
{
	register int i, cnt;

	for (cnt = len; cnt > 0; cnt -= i, buf += i) {
		if ((i = write(xprt->xp_sock, buf, cnt)) < 0) {
			((struct tcp_conn *)(xprt->xp_p1))->strm_stat =
			    XPRT_DIED;
			return (-1);
		}
	}
	return (len);
}

static enum xprt_stat
svctcp_stat(xprt)
	SVCXPRT *xprt;
{
	register struct tcp_conn *cd =
	    (struct tcp_conn *)(xprt->xp_p1);

	if (cd->strm_stat == XPRT_DIED)
		return (XPRT_DIED);
	if (! xdrrec_eof(&(cd->xdrs)))
		return (XPRT_MOREREQS);
	return (XPRT_IDLE);
}

static bool_t
svctcp_recv(xprt, msg)
	SVCXPRT *xprt;
	register struct rpc_msg *msg;
{
	register struct tcp_conn *cd =
	    (struct tcp_conn *)(xprt->xp_p1);
	register XDR *xdrs = &(cd->xdrs);

	xdrs->x_op = XDR_DECODE;
	(void)xdrrec_skiprecord(xdrs);
	if (xdr_callmsg(xdrs, msg)) {
		cd->x_id = msg->rm_xid;
		return (TRUE);
	}
	return (FALSE);
}

static bool_t
svctcp_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{

	return ((*xdr_args)(&(((struct tcp_conn *)(xprt->xp_p1))->xdrs), args_ptr));
}

static bool_t
svctcp_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	register XDR *xdrs =
	    &(((struct tcp_conn *)(xprt->xp_p1))->xdrs);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_args)(xdrs, args_ptr));
}

static bool_t
svctcp_reply(xprt, msg)
	SVCXPRT *xprt;
	register struct rpc_msg *msg;
{
	register struct tcp_conn *cd =
	    (struct tcp_conn *)(xprt->xp_p1);
	register XDR *xdrs = &(cd->xdrs);
	register bool_t stat;

	xdrs->x_op = XDR_ENCODE;
	msg->rm_xid = cd->x_id;
	stat = xdr_replymsg(xdrs, msg);
	(void)xdrrec_endofrecord(xdrs, TRUE);
	return (stat);
}
