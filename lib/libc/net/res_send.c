#ifndef lint
static	char	*sccsid = "@(#)res_send.c	4.3	(ULTRIX)	2/14/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
static char sccsid[] = "@(#)res_send.c	6.18 (Berkeley) 11/7/87";
 */

/*
 * Modification History:
 *
 * 11-Aug-89	sue
 *	Changed buffer sizes from PACKETSZ to HES_BUFMAX.  Moved 
 *	assignment of cred[ns].v_circuit into the retry if.
 *
 */


/*
 * Send query to name server and wait for reply.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <hesiod.h>
#ifdef AUTHEN
#include <krb.h>
#endif AUTHEN

#if defined(lint) && !defined(DEBUG)
#define DEBUG
#endif

extern int errno;

cred_st cred[MAXNS];  

#ifdef AUTHEN
struct sockaddr_in authen_from;
int fromlen = sizeof(struct sockaddr_in);
#endif AUTHEN

static int sd = -1;	/* socket used for virtual circuit communications */
static int sv = -1;	/* socket used for datagram communications */
static struct sockaddr no_addr;


#ifndef FD_SET
#define	NFDBITS		32
#define	FD_SETSIZE	32
#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#endif

#define KEEPOPEN (RES_USEVC|RES_STAYOPEN)


res_send(buf, buflen, answer, anslen)
	char *buf;
	int buflen;
	char *answer;
	int anslen;
{
	register int n;
	int retry, v_circuit, resplen, ns;
	int gotsomewhere = 0, connected = 0;
	u_short id, len;
	char *cp;
	fd_set dsmask;
	struct timeval timeout;
	HEADER *hp = (HEADER *) buf;
	HEADER *anhp = (HEADER *) answer;
	struct iovec iov[2];
	int terrno = ETIMEDOUT;
	char junk[512];

	char *sendbuf;
	int sendbuflen;

#ifdef AUTHEN
	char *qb;
        char dname[MAXDNAME];
	int msgtype;
	int msgclass;
	int authtype;
	int authversion;
	CREDENTIALS krbcred_st;
	char *dnptr;
	char *krbnameptr;
	extern char *res_dotname_head();
#endif AUTHEN

#ifdef DEBUG
	if (_res.options & RES_DEBUG) {
		printf("res_send()\n");
		p_query(buf);
	}
#endif DEBUG
	if (!(_res.options & RES_INIT))
		if (res_init() == -1) {
			return(-1);
		}
	v_circuit = (_res.options & RES_USEVC) || buflen > HES_BUFMAX;
	id = hp->id;
	/*
	 * Send request, RETRY times, or until successful
	 */

	for (retry = _res.retry; retry > 0; retry--) {
	   for (ns = 0; ns < _res.nscount; ns++) {
#ifdef AUTHEN
	        if (hp->opcode == AQUERY && retry == _res.retry) {

			bcopy(buf, cred[ns].buf, buflen);

			qb = cred[ns].buf + sizeof(HEADER);

			if ((n = dn_expand((char *)cred[ns].buf,
					   cred[ns].buf + buflen,
					   qb, dname, MAXDNAME)) < 0) {
				return (-1);
			}
			dnptr = dname;
			res_dotname_rmhead(&dnptr);

			qb += n;

			msgtype = _getshort(qb);
			qb += sizeof(u_short);
			msgclass = _getshort(qb);
			qb += sizeof(u_short);

			authtype = _getshort(qb);
			qb += sizeof(u_short);
			authversion = _getshort(qb);
			qb += sizeof(u_short);			

			krbnameptr = res_dotname_head((_res.ns_list + ns)->dname);
			if (authtype == AUTH_KRB && authversion == ONE) {
				if((cred[ns].len_ar = res_mkl_krbcred(ONE,
						     "named",
						     krbnameptr,
						     &krbcred_st,
						     cred[ns].buf,
						     cred[ns].buf + buflen,
						     HES_BUFMAX - buflen,
						     dnptr, msgtype, msgclass))
				   < RET_OK) {
#ifdef DEBUG
					if (_res.options & RES_DEBUG)
						perror("bad krb cred call %d",
						       cred[ns].len_ar);
#endif DEBUG					
					free(krbnameptr);
					return(-1);
				}
				free(krbnameptr);
		        }
			else if(authtype == AUTH_NONE) {
				cred[ns].len_ar = 0;
			} else {
				/* authtype != AUTH_NONE */
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror("bad auth type");
#endif DEBUG
				return(-1);
			}

			sendbuf = cred[ns].buf;
			sendbuflen = cred[ns].len_ar + buflen;
		} else {
#endif AUTHEN
		        if (retry == _res.retry) {
				cred[ns].v_circuit = (_res.options & RES_USEVC) || (sendbuflen) > HES_BUFMAX;
		                cred[ns].len_ar = 0;
		                sendbuf = buf;
		                sendbuflen = buflen;
			      }
#ifdef AUTHEN
		      }
#endif AUTHEN


#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("Querying server (# %d) address = %s\n", ns+1,
			      inet_ntoa(_res.ns_list[ns].addr.sin_addr));
#endif DEBUG
		if (cred[ns].v_circuit) {
			int truncated = 0;

			/*
			 * Use virtual circuit.
			 */
			if (sv < 0) {
				sv = socket(AF_INET, SOCK_STREAM, 0);
				if (sv < 0) {
					terrno = errno;
#ifdef DEBUG
					if (_res.options & RES_DEBUG)
					    perror("socket failed");
#endif DEBUG
					continue;
				}
				if (connect(sv, &(_res.ns_list[ns].addr),
				   sizeof(struct sockaddr)) < 0) {
					terrno = errno;
#ifdef DEBUG
					if (_res.options & RES_DEBUG)
					    perror("connect failed");
#endif DEBUG
					(void) close(sv);
					sv = -1;
					continue;
				}
			}
			/*
			 * Send length & message
			 */
			len = htons((u_short)sendbuflen);
			iov[0].iov_base = (caddr_t)&len;
			iov[0].iov_len = sizeof(len);
			iov[1].iov_base = sendbuf;
			iov[1].iov_len = sendbuflen;
			if (writev(sv, iov, 2) != sizeof(len) + sendbuflen) {
				terrno = errno;
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror("write failed");
#endif DEBUG
				(void) close(sv);
				sv = -1;
				continue;
			}
			/*
			 * Receive length & response
			 */
			cp = answer;
			len = sizeof(short);
			while (len != 0 &&
			    (n = read(sv, (char *)cp, (int)len)) > 0) {
				cp += n;
				len -= n;
			}
			if (n <= 0) {
				terrno = errno;
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror("read failed");
#endif DEBUG
				(void) close(sv);
				sv = -1;
				continue;
			}
			cp = answer;
			if ((resplen = ntohs(*(u_short *)cp)) > anslen) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					fprintf(stderr, "response truncated\n");
#endif DEBUG
				len = anslen;
				truncated = 1;
			} else
				len = resplen;
			while (len != 0 &&
			   (n = read(sv, (char *)cp, (int)len)) > 0) {
				cp += n;
				len -= n;
			}
			if (n <= 0) {
				terrno = errno;
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror("read failed");
#endif DEBUG
				(void) close(sv);
				sv = -1;
				continue;
			}
			if (truncated) {
				/*
				 * Flush rest of answer
				 * so connection stays in synch.
				 */
				anhp->tc = 1;
				len = resplen - anslen;
				while (len != 0) {
					n = (len > sizeof(junk) ?
					    sizeof(junk) : len);
					if ((n = read(sv, junk, n)) > 0)
						len -= n;
					else
						break;
				}
			}
#ifdef AUTHEN
			authen_from = _res.ns_list[ns].addr;
#endif AUTHEN
		} else {
			/*
			 * Use datagrams.
			 */
			if (sd < 0)
				sd = socket(AF_INET, SOCK_DGRAM, 0);
#if	BSD >= 43
			if (_res.nscount == 1 || retry == _res.retry) {
				/*
				 * Don't use connect if we might
				 * still receive a response
				 * from another server.
				 */
				if (connected == 0) {
					if (connect(sd, &_res.ns_list[ns].addr,
					    sizeof(struct sockaddr)) < 0) {
#ifdef DEBUG
						if (_res.options & RES_DEBUG)
							perror("connect");
#endif DEBUG
						continue;
					}
					connected = 1;
				}
				if (send(sd, sendbuf, sendbuflen, 0)
				    != sendbuflen) {
#ifdef DEBUG
					if (_res.options & RES_DEBUG)
						perror("send");
#endif DEBUG
					continue;
				}
			} else
#endif BSD
				if (sendto(sd, sendbuf, sendbuflen, 0,
					   &_res.ns_list[ns].addr,
					   sizeof(struct sockaddr))
				    != sendbuflen) {
#ifdef DEBUG
					if (_res.options & RES_DEBUG)
						perror("sendto");
#endif DEBUG
					continue;
				}

			/*
			 * Wait for reply
			 */
			timeout.tv_sec = (_res.retrans << (_res.retry - retry))
				/ _res.nscount;
			if (timeout.tv_sec <= 0)
				timeout.tv_sec = 1;
			timeout.tv_usec = 0;
wait:
			FD_ZERO(&dsmask);
			FD_SET(sd, &dsmask);
			n = select(sd+1, &dsmask, (fd_set *)NULL,
				(fd_set *)NULL, &timeout);
			if (n < 0) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror("select");
#endif DEBUG
				continue;
			}
			if (n == 0) {
				/*
				 * timeout
				 */
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					printf("timeout\n");
#endif DEBUG
				/*
				 * Disconnect if we want to listen
				 * for responses from more than one server.
				 */
				if (_res.nscount > 1 && connected) {
					(void) connect(sd, &no_addr,
					    sizeof(no_addr));
					connected = 0;
				}
				gotsomewhere = 1;
				continue;
			}
#ifdef AUTHEN
			if ((resplen = recvfrom(sd, answer, anslen, 0,
						&authen_from, &fromlen)) <= 0) {
#else /* AUTHEN */
			if ((resplen = recv(sd, answer, anslen, 0)) <= 0) {
#endif AUTHEN

#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror("recvfrom");
#endif DEBUG
				continue;
			}
			gotsomewhere = 1;
			if (id != anhp->id) {
				/*
				 * response from old query, ignore it
				 */
#ifdef DEBUG
				if (_res.options & RES_DEBUG) {
					printf("old answer:\n");
					p_query(answer);
				}
#endif DEBUG
				goto wait;
			}
			if (!(_res.options & RES_IGNTC) && anhp->tc) {
				/*
				 * get rest of answer
				 */
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					printf("truncated answer\n");
#endif DEBUG
				(void) close(sd);
				sd = -1;
				/*
				 * retry decremented on continue
				 * to desired starting value
				 */
				retry = _res.retry + 1;
				cred[ns].v_circuit = 1;
				ns--;
				continue;
			}
		}
#ifdef DEBUG
		if (_res.options & RES_DEBUG) {
			printf("got answer:\n");
			p_query(answer);
		}
#endif DEBUG
		errno = 0;
		/*
		 * We are going to assume that the first server is preferred
		 * over the rest (i.e. it is on the local machine) and only
		 * keep that one open.
		 */
		if ((_res.options & KEEPOPEN) == KEEPOPEN && ns == 0) {
			return (resplen);
		} else {
		        if (sv >= 0) {
			        (void) close(sv);
			        sv = -1;
		        }

			if (sd >= 0) {
			        (void) close(sd);
			        sd = -1;
			}
		  return (resplen);
		}
	   }
	}
	if (sv >= 0) {
		(void) close(sv);
		sv = -1;
	}
	if (sd >= 0) {
		(void) close(sd);
		sd = -1;
	}
	if (v_circuit == 0)
		if (gotsomewhere == 0)
			errno = ECONNREFUSED;
		else
			errno = ETIMEDOUT;
	else
		errno = terrno;
	return (-1);
}

/*
 * This routine is for closing the socket if a virtual circuit is used and
 * the program wants to close it.  This provides support for endhostent()
 * which expects to close the socket.
 *
 * This routine is not expected to be user visible.
 */
_res_close()
{
	if (sd != -1) {
		(void) close(sd);
		sd = -1;
	}

	if (sv != -1) {
		(void) close(sv);
		sv = -1;
	}
}
