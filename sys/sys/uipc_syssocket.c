#ifdef dontinclude
#ifndef lint
static char *sccsid = "@(#)uipc_syssocket.c	4.1	ULTRIX	7/2/90";
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

/* ------------------------------------------------------------------------
 * Modification History: /sys/sys/uipc_syssocket.c
 *
 *	R. Bhanukitsiri - 12/31/89
 *		Merge in VAX V3.0:
 *		Comment out this module since no one currently uses it.
 *
 *	John Forecast	- 08/06/86
 *		Bug fixes and add MBUF chain and ioctl routines
 *
 *	John Forecast	- 08/22/86
 *		Sending an MBUF chain to a socket which failed to connect
 *		causes the system to crash.
 *
 * -----------------------------------------------------------------------
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/sysproc.h"
#include "../h/file.h"
#include "../h/inode.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/un.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/stat.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../net/if.h"

#define CHECKERRNO	(cusysproc->sp_errno ? -1 : 0)

/*
 * System process socket operation routines.
 */

struct mbuf *ssockargs(name, namelen, type)
caddr_t name;
int namelen,type;
{
	register struct mbuf *m = m_get(M_WAIT, type);

	m->m_len = namelen;
	bcopy(name, mtod(m, caddr_t), namelen);
	return (m);
}

/*
 * Create a system socket.
 */
ssocket(af, type, protocol)
int af,type,protocol;
{
	int i = findsysprocio();

	if (i != -1) {
/* please check this larry cohen or ed ferris */
#ifdef bad_number_of_args
		cusysproc->sp_errno = socreate(af, &cusysproc->sp_io[i], type, protocol, cusysproc->sp_uid);
#else
		cusysproc->sp_errno = socreate(af, &cusysproc->sp_io[i], type, protocol);
#endif bad_number_of_args
	}
	else cusysproc->sp_errno = EMFILE;
	return (i);
}

/*
 * Bind a name to a system socket.
 */
sbind(io, name, namelen)
int io;
caddr_t name;
int namelen;
{
	struct mbuf *nam;

	nam = ssockargs(name, namelen, MT_SONAME);
	cusysproc->sp_errno = sobind(cusysproc->sp_io[io], nam);
	m_free(nam);
	return (CHECKERRNO);
}

/*
 * Listen on a system socket.
 */
slisten(io, backlog)
int io;
int backlog;
{
	return (solisten(cusysproc->sp_io[io], backlog));
}

/*
 * Close a system socket.
 */
sclose(io)
int io;
{
	int error = soclose(cusysproc->sp_io[io]);

	if (error == 0)
		cusysproc->sp_io[io] = 0;
	return (error);
}

/*
 * Accept a connection on a system socket.
 */
saccept(io, name, namelen)
int io;
caddr_t name;
int *namelen;
{
	register struct mbuf *nam;
	register struct socket *so = (struct socket *)cusysproc->sp_io[io];
	struct socket *aso;
	int i,len,s = splnet();

	if ((i = findsysprocio()) == -1) {
		cusysproc->sp_errno = EMFILE;
		splx(s);
		return (-1);
	}
	if ((so->so_options & SO_ACCEPTCONN) == 0) {
		cusysproc->sp_errno = EINVAL;
		splx(s);
		return (-1);
	}
	if ((so->so_state & SS_NBIO) && so->so_qlen == 0) {
		cusysproc->sp_errno = EWOULDBLOCK;
		splx(s);
		return (-1);
	}
	while (so->so_qlen == 0 && so->so_error == 0) {
		if (so->so_state & SS_CANTRCVMORE) {
			so->so_error = ECONNABORTED;
			break;
		}
		sleepsysproc((caddr_t)&so->so_timeo);
	}
	if (cusysproc->sp_errno = so->so_error) {
		splx(s);
		return (-1);
	}
	aso = so->so_q;
	if (soqremque(aso, 1) == 0)
		panic("saccept");
	cusysproc->sp_io[i] = (caddr_t)aso;
	nam = m_get(M_WAIT, MT_SONAME);
	soaccept(aso, nam);
	if (name) {
		len = min(*namelen, nam->m_len);
		bcopy(mtod(nam, caddr_t), name, len);
		*namelen = len;
	}
	m_free(nam);
	splx(s);
	return (i);
}

/*
 * Initiate a connection on a system socket.
 */
sconnect(io, name, namelen)
int io;
caddr_t name;
int namelen;
{
	register struct socket *so = (struct socket *)cusysproc->sp_io[io];
	struct mbuf *nam;
	int s;

	nam = ssockargs(name, namelen, MT_SONAME);
	s = splnet();
	if ((cusysproc->sp_errno = soconnect(so, nam)) == 0) {
		if ((so->so_state & (SS_NBIO|SS_ISCONNECTING)) != (SS_NBIO|SS_ISCONNECTING)) {
			while ((so->so_state & SS_ISCONNECTING) && so->so_error == 0)
				sleepsysproc((caddr_t)&so->so_timeo);
			cusysproc->sp_errno = so->so_error;
			so->so_error = 0;
		} else cusysproc->sp_errno = EINPROGRESS;
	}
	splx(s);
	m_free(nam);
	return (CHECKERRNO);
}

/*
 * Set socket option on a system socket.
 */
ssetsockopt(io, level, optname, optval, optlen)
int io;
int level,optname;
caddr_t optval;
int optlen;
{
	register struct mbuf *m = 0,*clbuf;

	if (optval) {
		m = m_get(M_WAIT, MT_SOOPTS);
		if (optlen > MLEN) {
			MCLGET(m, clbuf);
			if (clbuf == 0) {
				m_free(m);
				return (ENOBUFS);
			}
		}
		bcopy(optval, mtod(m, caddr_t), optlen);
		m->m_len = optlen;
	}
	return (sosetopt(cusysproc->sp_io[io], level, optname, m));
}

/*
 * Get socket option from a system socket.
 */
sgetsockopt(io, level, optname, optval, optlen)
int io;
int level,optname;
caddr_t optval;
int *optlen;
{
	struct mbuf *m;
	int len = 0;

	if (optval)
		len = *optlen;
	if ((cusysproc->sp_errno = sogetopt(cusysproc->sp_io[io], level, optname, &m)) == 0) {
		if (optval && len && m) {
			*optlen = min(len, m->m_len);
			bcopy(mtod(m, caddr_t), optval, *optlen);
		}
	}
	if (m)
		m_free(m);
	return (CHECKERRNO);
}

/*
 * Get socket name from system socket.
 */
sgetsockname(io, name, namelen)
int io;
caddr_t name;
int *namelen;
{
	register struct mbuf *m = m_getclr(M_WAIT, MT_SONAME);
	register struct socket *so = (struct socket *)cusysproc->sp_io[io];

	if ((cusysproc->sp_errno = (*so->so_proto->pr_usrreq)(so, PRU_SOCKADDR, 0, m, 0)) == 0) {
		*namelen = min(*namelen, m->m_len);
		bcopy(mtod(m, caddr_t), name, *namelen);
	}
	m_free(m);
	return (CHECKERRNO);
}

/*
 * Get name of peer from system socket.
 */
sgetpeername(io, name, namelen)
int io;
caddr_t name;
int *namelen;
{
	register struct mbuf *m;
	register struct socket *so = (struct socket *)cusysproc->sp_io[io];

	if ((so->so_state & SS_ISCONNECTED) == 0) {
		cusysproc->sp_errno = ENOTCONN;
		return (-1);
	}
	m = m_getclr(M_WAIT, MT_SONAME);
	if ((cusysproc->sp_errno = (so->so_proto->pr_usrreq)(so, PRU_PEERADDR, 0, m, 0)) == 0) {
		*namelen = min(*namelen, m->m_len);
		bcopy(mtod(m, caddr_t), name, *namelen);
	}
	m_free(m);
	return (CHECKERRNO);
}

/*
 * Send data to a specified destination on a system socket.
 */
ssendto(io, buf, len, flags, to, tolen)
int io;
caddr_t buf,to;
int len,flags,tolen;
{
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = to;
	msg.msg_namelen = tolen;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;

	aiov.iov_base = buf;
	aiov.iov_len = len;

	return (ssendmsg(io, &msg, flags));
}

/*
 * Send data on a system socket.
 */
ssend(io, buf, len, flags)
int io;
caddr_t buf;
int len,flags;
{
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;

	aiov.iov_base = buf;
	aiov.iov_len = len;

	return (ssendmsg(io, &msg, flags));
}

/*
 * Send message on system socket.
 */
ssendmsg(io, msg, flags)
int io;
struct msghdr *msg;
int flags;
{
	struct uio auio;
	struct mbuf *to = 0,*rights = 0;
	register struct iovec *iov;
	register int i;
	int len,bytessent = -1;

	auio.uio_iov = iov = msg->msg_iov;
	auio.uio_iovcnt = msg->msg_iovlen;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_offset = 0;
	auio.uio_resid = 0;

	for (i = 0; i < msg->msg_iovlen; i++) {
		if (iov->iov_len < 0) {
			cusysproc->sp_errno = EINVAL;
			return (-1);
		}
		auio.uio_resid += iov->iov_len;
		iov++;
	}

	if (msg->msg_name)
		to = ssockargs(msg->msg_name, msg->msg_namelen, MT_SONAME);
	if (msg->msg_accrights)
		rights = ssockargs(msg->msg_accrights, msg->msg_accrightslen, MT_RIGHTS);
	
	len = auio.uio_resid;
	if ((cusysproc->sp_errno = sosend(cusysproc->sp_io[io], to, &auio, flags, rights)) == 0)
		bytessent = len - auio.uio_resid;
	
	if (to)
		m_free(to);
	if (rights)
		m_free(rights);
	return(bytessent);
}

/*
 * Send a preallocated MBUF chain on a system socket.
 */
ssendm(io, m, flags, aname, rightsp)
int io;
struct mbuf *m;
int flags;
struct mbuf *aname;
struct mbuf *rightsp;
{
    register struct socket *so = (struct socket *)cusysproc->sp_io[io];
    
    if (so->so_state & SS_CANTSENDMORE)
    {
	m_freem(m);
	cusysproc->sp_errno = EPIPE;
    }
    else cusysproc->sp_errno = (*so->so_proto->pr_usrreq)(so, (flags & MSG_OOB) ? PRU_SENDOOB : PRU_SEND, m, aname, rightsp);
}

/*
 * Receive data from a specified source on a system socket.
 */
srecvfrom(io, buf, len, flags, from, fromlen)
int io;
caddr_t buf,from;
int len,flags,*fromlen;
{
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = from;
	msg.msg_namelen = *fromlen;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;

	aiov.iov_base = buf;
	aiov.iov_len = len;

	return (srecvit(io, &msg, flags, fromlen, 0));
}

/*
 * Receive data from a system socket.
 */
srecv(io, buf, len, flags)
int io;
caddr_t buf;
int len,flags;
{
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;

	aiov.iov_base = buf;
	aiov.iov_len = len;

	return (srecvit(io, &msg, flags, 0, 0));
}

/*
 * Receive message from a system socket.
 */
srecvmsg(io, msg, flags)
int io;
struct msghdr *msg;
int flags;
{
	return(srecvit(io, msg, flags, &msg->msg_namelen, &msg->msg_accrightslen));
}

/*
 * Local routine to receive data from a system socket.
 */
static srecvit(io, msg, flags, namelenp, rightslenp)
int io;
struct msghdr *msg;
int flags,*namelenp,*rightslenp;
{
	struct uio auio;
	struct mbuf *from,*rights;
	register struct iovec *iov;
	register int i;
	int len,bytesrcvd = -1;

	auio.uio_iov = iov = msg->msg_iov;
	auio.uio_iovcnt = msg->msg_iovlen;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_offset = 0;
	auio.uio_resid = 0;

	for (i = 0; i < msg->msg_iovlen; i++) {
		if (iov->iov_len < 0) {
			cusysproc->sp_errno = EINVAL;
			return (-1);
		}
		auio.uio_resid += iov->iov_len;
		iov++;
	}

	len = auio.uio_resid;
	if ((cusysproc->sp_errno = soreceive( cusysproc->sp_io[io], &from, &auio, flags, &rights)) == 0)
		bytesrcvd = len - auio.uio_resid;
	
	if (msg->msg_name) {
		*namelenp = 0;
		if (from != 0 && msg->msg_namelen > 0) {
			*namelenp = min(msg->msg_namelen, from->m_len);
			bcopy(mtod(from, caddr_t), msg->msg_name, *namelenp);
		}
	}

	if (msg->msg_accrights) {
		*rightslenp = 0;
		if (rights != 0 && msg->msg_accrightslen > 0) {
			*rightslenp = min(msg->msg_accrightslen, rights->m_len);
			bcopy(mtod(rights, caddr_t), msg->msg_accrights, *rightslenp);
		}
	}
	if (from)
		m_free(from);
	if (rights)
		m_free(rights);
	return (bytesrcvd);
}

/*
 * Return the next available message as an MBUF chain from a system socket.
 */
struct mbuf *srecvm(io, flags, aname, rightsp)
int io,flags;
struct mbuf **aname,**rightsp;
{
    register struct mbuf *m,*n;
    register struct socket *so = (struct socket *)cusysproc->sp_io[io];
    register struct protosw *pr = so->so_proto;
    
    if (aname)
	*aname = 0;
    if (rightsp)
	*rightsp = 0;
    if (flags & MSG_OOB)
    {
	m = m_get(M_WAIT, MT_DATA);
	if (cusysproc->sp_errno = (*pr->pr_usrreq)(so, PRU_RCVOOB, m, 0, 0))
	{
	    m_freem(m);
	    m = 0;
	}
    }
    else
    {
	if (m = so->so_rcv.sb_mb)
	{
	    if (pr->pr_flags & PR_ADDR)
	    {
		if (aname)
		{
		    *aname = m;
		    sbfree(&so->so_rcv, m);
		    so->so_rcv.sb_mb = m = m->m_act;
		}
		else m = sbdroprecord(&so->so_rcv);
	    }
	    if (m->m_type == MT_RIGHTS)
	    {
		if (rightsp)
		{
		    *rightsp = m;
		    sbfree(&so->so_rcv, m);
		    so->so_rcv.sb_mb = m = m->m_act;
		}
		else m = sbdroprecord(&so->so_rcv);
	    }
	    n = m;
	    so->so_rcv.sb_mb = m->m_act;
	    so->so_state &= ~SS_RCVATMARK;
	    while (n)
	    {
		sbfree(&so->so_rcv, n);
		if (so->so_oobmark)
		{
		    so->so_oobmark -= n->m_len;
		    if (so->so_oobmark == 0)
			so->so_state |= SS_RCVATMARK;
		}
		n = n->m_next;
	    }
	    if (so->so_proto->pr_flags & PR_WANTRCVD)
		(*so->so_proto->pr_usrreq)(so, PRU_RCVD, 0, 0, 0);
	}
	else
	{
	    cusysproc->sp_errno = 0;
	    if ((so->so_state & SS_ISCONNECTED) == 0 &&
		(so->so_proto->pr_flags & PR_CONNREQUIRED))
		cusysproc->sp_errno = ENOTCONN;
	}
    }
    return (m);
}

/*
 * Unselect a system process.
 */
sunselect()
{
	wakeup((caddr_t)&selwait);
}

/*
 * Select from a number of sockets.
 */
sselect(nio, readio, writeio, exceptio, tv)
int nio,*readio,*writeio,*exceptio;
struct timeval *tv;
{
	register int i,ibits,obits,n;
	register struct socket *so;
	struct timeval atv;
	int readbits = 0, writebits = 0,s;

	if (readio)
		readbits = *readio;
	if (writeio)
		writebits = *writeio;

	if (tv) {
		atv = *tv;
		if (itimerfix(&atv)) {
			cusysproc->sp_errno = EINVAL;
			return (-1);
		}
		s = splhigh();
		timevaladd(&atv, &time);
		splx(s);
	}

	ibits = obits = n = 0;
	for (i = 0; i < nio; i++) {
		if (so = (struct socket *)cusysproc->sp_io[i]) {
			if (readbits & (1 << i)) {
				if (soreadable(so)) {
					ibits |= 1 << i;
					n++;
				} else so->so_rcv.sb_flags |= SB_SEL|SB_COLL;
			}
			if (writebits & (1 << i)) {
				if (sowriteable(so)) {
					obits |= 1 << i;
					n++;
				} else so->so_snd.sb_flags |= SB_SEL|SB_COLL;
			}
		}
	}
	s = splhigh();
	if ((n == 0) || (tv && (time.tv_sec < atv.tv_sec || (time.tv_sec == atv.tv_sec && time.tv_usec < atv.tv_usec)))) {
		if (tv)
			timeout(sunselect, cusysproc, hzto(&atv));
		sleepsysproc((caddr_t)&selwait);
		if (tv)
			untimeout(sunselect, cusysproc);
	}
	splx(s);
	if (readio)
		*readio = ibits;
	if (writeio)
		*writeio = obits;
	return (n);
}

/*
 * Perform an I/O control function on a system socket.
 */
sioctl(io, cmd, data)
int io,cmd;
caddr_t data;
{
    register struct socket *so = (struct socket *)cusysproc->sp_io[io];
    
    switch (cmd)
    {
	case FIONBIO:
	    if (*(int *)data)
		so->so_state |= SS_NBIO;
	    else
		so->so_state &= ~SS_NBIO;
	    return (0);
	    
	case FIOASYNC:
	    if (*(int *)data)
		so->so_state |= SS_ASYNC;
	    else
		so->so_state &= ~SS_ASYNC;
	    return (0);
	    
	case FIONREAD:
	    *(int *)data = so->so_rcv.sb_cc;
	    return (0);
	    
	case SIOCATMARK:
	    *(int *)data = (so->so_state & SS_RCVATMARK) != 0;
	    return (0);
	    
	default:
	    return ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL, cmd, data, 0));
    }
}
#endif dontinclude
