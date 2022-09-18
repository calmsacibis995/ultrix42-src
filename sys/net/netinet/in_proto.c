#ifndef lint
static	char	*sccsid = "@(#)in_proto.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984-89 by			*
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
 * Modification History: /sys/netinet/in_proto.c
 *
 *      15-Aug-89       jsd
 *              Add HELLO proto entry
 *
 *      05-May-89       Michael G. Mc Menemy
 *              Add XTI Support.
 *
 *	15-Jan-88	lp
 *		Merge of final 43BSD changes.
 *
 * 28 Jan 87 -- Larry Cohen
 *	add protocol control routines to switch table.
 *
 * 13 Jan 87 -- lp
 *	Added line for EGP to inetsw.
 *
 * 24 Oct 84 -- jrs
 *	Fix definition for icmp protocol switch to facilitate access
 *	Derived from 4.2BSD, labeled:
 *		in_proto.c 6.1	83/07/29
 *
 * 09/16/85 -- Larry Cohen
 * 	Add changes for subnet routing kernel
 *
 * -----------------------------------------------------------------------
 */

#include "../h/param.h"
#include "../h/socket.h"
#include "../h/protosw.h"
#include "../h/domain.h"
#include "../h/mbuf.h"

#include "../net/netinet/in.h"
#include "../net/netinet/in_systm.h"

/*
 * TCP/IP protocol family: IP, ICMP, UDP, TCP.
 */
int	ip_output(), ip_ctloutput();
int	ip_init(),ip_slowtimo(),ip_drain();
int	ip_ifoutput(),	ip_ifinput(),	ip_ifioctl();
int	icmp_input();
int	udp_input(),udp_ctlinput();
int	udp_usrreq();
#ifdef XTI
int     udp_ctloutput();
#endif XTI
int	udp_init();
int	tcp_input(),tcp_ctlinput();
int	tcp_usrreq(), tcp_ctloutput();
int	tcp_init(),tcp_fasttimo(),tcp_slowtimo(),tcp_drain();
int	rip_input(),rip_output(), rip_ctloutput();
extern	int raw_usrreq();

#ifdef vax
/*
 * IMP protocol family: raw interface.
 * Using the raw interface entry to get the timer routine
 * in is a kludge.
 */
#include "imp.h"
#if NIMP > 0
int	rimp_output(), hostslowtimo();
extern struct domain impdomain;
#endif
#endif vax

#ifdef NSIP
int	idpip_input(), nsip_ctlinput();
#endif

struct protosw inetsw[] = {
{ 0,		&inetdomain,	0,		0,
  0,		ip_output,	0,		0,
  0,
  ip_init,	0,		ip_slowtimo,	ip_drain,
  ip_ifoutput,	ip_ifinput,	ip_ifioctl,
},
{ SOCK_DGRAM,	&inetdomain,	IPPROTO_UDP,	PR_ATOMIC|PR_ADDR,
#ifdef XTI
  udp_input,	0,		udp_ctlinput,	udp_ctloutput,
#else
  udp_input,	0,		udp_ctlinput,	ip_ctloutput,
#endif XTI
  udp_usrreq,
  udp_init,	0,		0,		0,
  0,		0,		0,
},
{ SOCK_STREAM,	&inetdomain,	IPPROTO_TCP,	PR_CONNREQUIRED|PR_WANTRCVD,
  tcp_input,	0,		tcp_ctlinput,	tcp_ctloutput,
  tcp_usrreq,
  tcp_init,	tcp_fasttimo,	tcp_slowtimo,	tcp_drain,
  0,		0,		0,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_RAW,	PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	0,	rip_ctloutput,
  raw_usrreq,
  0,		0,		0,		0,
  0,		0,		0,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_ICMP,	PR_ATOMIC|PR_ADDR,
  icmp_input,	rip_output,	0,		rip_ctloutput,
  raw_usrreq,
  0,		0,		0,		0,
  0,		0,		0,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_HELLO,	PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	0,		0,
  raw_usrreq,
  0,		0,		0,		0,
  0,		0,		0,
},
#ifdef NSIP
{ SOCK_RAW,	&inetdomain,	IPPROTO_IDP,	PR_ATOMIC|PR_ADDR,
  idpip_input,	rip_output,	nsip_ctlinput,	0,
  raw_usrreq,
  0,		0,		0,		0,
  0,		0,		0,
},
#endif NSIP
{ SOCK_RAW,	&inetdomain,	IPPROTO_EGP,	PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	0,		0,
  raw_usrreq,
  0,		0,		0,		0,
  0,		0,		0,
},
/* raw wildcard */
{ SOCK_RAW,	&inetdomain,	0,		PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	0,		rip_ctloutput,
  raw_usrreq,
  0,		0,		0,		0,
  0,		0,		0,
},
};

struct domain inetdomain =
    { AF_INET, "internet", 0, 0, 0, 
      inetsw, &inetsw[sizeof(inetsw)/sizeof(inetsw[0])] };

#ifdef vax
#if NIMP > 0
struct protosw impsw[] = {
{ SOCK_RAW,	&impdomain,	0,		PR_ATOMIC|PR_ADDR,
  0,		rimp_output,	0,		0,
  raw_usrreq,
  0,		0,		hostslowtimo,	0,
  0,		0,		0,
},
};

struct domain impdomain =
    { AF_IMPLINK, "imp", 0, 0, 0,
      impsw, &impsw[sizeof (impsw)/sizeof(impsw[0])] };
#endif
#endif vax
