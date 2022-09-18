/* @(#)protosw.h	4.4	(ULTRIX)	1/31/91 */

/************************************************************************
 *									*
 *		      Copyright (c) 1983, 1989 by			*
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
 *	Matt Thomas - 08/20/90						*
 *		Add support for OSI					*
 *									*
 *	Matt Thomas - 12/10/89						*
 *		Add new ctlinput for event processing.			*
 *									*
 *      Michael G. Mc Menemy - 05/03/89					*
 *              Add XTI support.					*
 *									*
 *	Larry Cohen  -  03/12/85					*
 *		Add new field to protosw structure to keep track of	*
 *		device state.  Requested by the DECnet folks.		*
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes  				*
 *									*
 ************************************************************************/

/*	protosw.h	6.2	83/09/19	*/

/*
 * Protocol switch table.
 *
 * Each protocol has a handle initializing one of these structures,
 * which is used for protocol-protocol and system-protocol communication.
 *
 * A protocol is called through the pr_init entry before any other.
 * Thereafter it is called every 200ms through the pr_fasttimo entry and
 * every 500ms through the pr_slowtimo for timer based actions.
 * The system will call the pr_drain entry if it is low on space and
 * this should throw away any non-critical data.
 *
 * Protocols pass data between themselves as chains of mbufs using
 * the pr_input and pr_output hooks.  Pr_input passes data up (towards
 * UNIX) and pr_output passes it down (towards the imps); control
 * information passes up and down on pr_ctlinput and pr_ctloutput.
 * The protocol is responsible for the space occupied by any the
 * arguments to these entries and must dispose it.
 *
 * The userreq routine interfaces protocols to the system and is
 * described below.
 */
struct protosw {
	short	pr_type;		/* socket type used for */
	struct	domain  *pr_domain;	/* domain protocol a member of */
	short	pr_protocol;		/* protocol number */
	short	pr_flags;		/* see below */
/* protocol-protocol hooks */
	int	(*pr_input)();		/* input to protocol (from below) */
	int	(*pr_output)();		/* output to protocol (from above) */
	int	(*pr_ctlinput)();	/* control input (from below) */
	int	(*pr_ctloutput)();	/* control output (from above) */
/* user-protocol hook */
	int	(*pr_usrreq)();		/* user request: see list below */
/* utility hooks */
	int	(*pr_init)();		/* initialization hook */
	int	(*pr_fasttimo)();	/* fast timeout (200ms) */
	int	(*pr_slowtimo)();	/* slow timeout (500ms) */
	int	(*pr_drain)();		/* flush any excess space possible */
/* interface hooks */
	int	(*pr_ifoutput)();	/* proto specific mods to output data*/
	int	(*pr_ifinput)();	/* proto specific mods to input data*/
	int	(*pr_ifioctl)();	/* proto specific mods for ioctl */
	int	(*pr_ifstate)();	/* proto specific - state change */
};

#define	PR_SLOWHZ	2		/* 2 slow timeouts per second */
#define	PR_FASTHZ	5		/* 5 fast timeouts per second */

/*
 * Values for pr_flags
 */
#define	PR_ATOMIC	0x01		/* exchange atomic messages only */
#define	PR_ADDR		0x02		/* addresses given with messages */
/* in the current implementation, PR_ADDR needs PR_ATOMIC to work */
#define	PR_CONNREQUIRED	0x04		/* connection required by protocol */
#define	PR_WANTRCVD	0x08		/* want PRU_RCVD calls */
#define	PR_RIGHTS	0x10		/* passes capabilities */
#define	PR_INTERNAL	0x20		/* inaccessible by socket(2) call */
#define	PR_OOBADDR	0x40		/* address given with OOB messages */

/*
 * The arguments to usrreq are:
 *	(*protosw[].pr_usrreq)(up, req, m, nam, opt);
 * where up is a (struct socket *), req is one of these requests,
 * m is a optional mbuf chain containing a message,
 * nam is an optional mbuf chain containing an address,
 * and opt is a pointer to a socketopt structure or nil.
 * The protocol is responsible for disposal of the mbuf chain m,
 * the caller is responsible for any space held by nam and opt.
 * A non-zero return from usrreq gives an
 * UNIX error number which should be passed to higher level software.
 */
#define	PRU_ATTACH		0	/* attach protocol to up */
#define	PRU_DETACH		1	/* detach protocol from up */
#define	PRU_BIND		2	/* bind socket to address */
#define	PRU_LISTEN		3	/* listen for connection */
#define	PRU_CONNECT		4	/* establish connection to peer */
#define	PRU_ACCEPT		5	/* accept connection from peer */
#define	PRU_DISCONNECT		6	/* disconnect from peer */
#define	PRU_SHUTDOWN		7	/* won't send any more data */
#define	PRU_RCVD		8	/* have taken data; more room now */
#define	PRU_SEND		9	/* send this data */
#define	PRU_ABORT		10	/* abort (fast DISCONNECT, DETATCH) */
#define	PRU_CONTROL		11	/* control operations on protocol */
#define	PRU_SENSE		12	/* return status into m */
#define	PRU_RCVOOB		13	/* retrieve out of band data */
#define	PRU_SENDOOB		14	/* send out of band data */
#define	PRU_SOCKADDR		15	/* fetch socket's address */
#define	PRU_PEERADDR		16	/* fetch peer's address */
#define	PRU_CONNECT2		17	/* connect two sockets */
/* begin for protocols internal use */
#define	PRU_FASTTIMO		18	/* 200ms timeout */
#define	PRU_SLOWTIMO		19	/* 500ms timeout */
#define	PRU_PROTORCV		20	/* receive from below */
#define	PRU_PROTOSEND		21	/* send to below */
#define PRU_GETSOCKOPT		22	/* getsockopt */
#define PRU_SETSOCKOPT		23	/* setsockopt */

#define	PRU_NREQ		23

#define PRU_REQLIST { \
	"ATTACH",	"DETACH",	"BIND",		"LISTEN", \
	"CONNECT",	"ACCEPT",	"DISCONNECT",	"SHUTDOWN", \
	"RCVD",		"SEND",		"ABORT",	"CONTROL", \
	"SENSE",	"RCVOOB",	"SENDOOB",	"SOCKADDR", \
	"PEERADDR",	"CONNECT2",	"FASTTIMO",	"SLOWTIMO", \
	"PROTORCV",	"PROTOSEND",	"GETSOCKOPT",	"SETSOCKOPT", \
}
/* prurequests is now defined in ../sys/uipc_socket.c */
extern char *prurequests[]; 

/*
 * The arguments to the ctlinput routine are
 *	(*protosw[].pr_ctlinput)(cmd, arg);
 * where cmd is one of the commands below, and arg is
 * an optional argument (caddr_t).
 *
 * N.B. The IMP code, in particular, pressumes the values
 *      of some of the commands; change with extreme care.
 * TODO:
 *	spread out codes so new ICMP codes can be
 *	accomodated more easily
 */
#define	PRC_IFDOWN		0	/* interface transition */
#define	PRC_ROUTEDEAD		1	/* select new route if possible */
#define	PRC_QUENCH		4	/* some said to slow down */
#define	PRC_MSGSIZE		5	/* message size forced drop */
#define	PRC_HOSTDEAD		6	/* normally from IMP */
#define	PRC_HOSTUNREACH		7	/* ditto */
#define	PRC_UNREACH_NET		8	/* no route to network */
#define	PRC_UNREACH_HOST	9	/* no route to host */
#define	PRC_UNREACH_PROTOCOL	10	/* dst says bad protocol */
#define	PRC_UNREACH_PORT	11	/* bad port # */
#define	PRC_UNREACH_NEEDFRAG	12	/* IP_DF caused drop */
#define	PRC_UNREACH_SRCFAIL	13	/* source route failed */
#define	PRC_REDIRECT_NET	14	/* net routing redirect */
#define	PRC_REDIRECT_HOST	15	/* host routing redirect */
#define	PRC_REDIRECT_TOSNET	16	/* redirect for type of service & net */
#define	PRC_REDIRECT_TOSHOST	17	/* redirect for tos & host */
#define	PRC_TIMXCEED_INTRANS	18	/* packet lifetime expired in transit */
#define	PRC_TIMXCEED_REASS	19	/* lifetime expired on reass q */
#define	PRC_PARAMPROB		20	/* header incorrect */
#define	PRC_NEWADDRSET		21	/* change in node address(es) */
#define	PRC_EVENT		22	/* event notification */
#define	PRC_NMADD		23	/* declare module to network mgmt */
#define PRC_CTICONNRQST		24	/* rcvd a connect request */
#define PRC_CTIFLOWCTLCHG	25	/* change in flow control status */
#define PRC_CTIDISCONNRQST	26	/* rcvd a disconnect request */

#define	PRC_NCMDS		26

#define PRC_REQLIST { \
	"IFDOWN", "ROUTEDEAD", "#2", "#3", \
	"QUENCH", "MSGSIZE", "HOSTDEAD", "HOSTUNREACH", \
	"NET-UNREACH", "HOST-UNREACH", "PROTO-UNREACH", "PORT-UNREACH", \
	"FRAG-UNREACH", "SRCFAIL-UNREACH", "NET-REDIRECT", "HOST-REDIRECT", \
	"TOSNET-REDIRECT", "TOSHOST-REDIRECT", "TX-INTRANS", "TX-REASS", \
	"PARAMPROB", "NEWADDRSET", "EVENT", "NWADD", "CTICONNRQST", \
	"CTIFLOWCTLCHG", "CTIDISCONNRQST" \
}
/* prcrequests is defined in ../net/if.c */
extern char *prcrequests[];

#ifdef KERNEL
extern	struct protosw *pffindproto(), *pffindtype();
#endif

/*
 * The arguments to ctloutput are:
 *	(*protosw[].pr_ctloutput)(req, so, level, optname, optval);
 * req is one of the actions listed below, so is a (struct socket *),
 * level is an indication of which protocol layer the option is intended.
 * optname is a protocol dependent socket option request,
 * optval is a pointer to a mbuf-chain pointer, for value-return results.
 * The protocol is responsible for disposal of the mbuf chain *optval
 * if supplied,
 * the caller is responsible for any space held by *optval, when returned.
 * A non-zero return from usrreq gives an
 * UNIX error number which should be passed to higher level software.
 */
#define	PRCO_GETOPT	0
#define	PRCO_SETOPT	1
#define PRCO_PIF	2
#define PRCO_NWMGT	3
#define PRCO_XTIMAPSTATE 4
#define PRCO_XTIMAPINFO  5
#define PRCO_XTICHKADDR  6
#define PRCO_XTIREJECT   7
#define PRCO_XTICOPYTP   8
#define PRCO_XTIUNBIND   9
#define PRCO_TRACE	 10
#define	PRCO_NCMDS	 11

#ifdef PRCOREQUESTS
char	*prcorequests[] = {
	"GETOPT", "SETOPT",
	"PIF", "NWMGT",
	"XTIMAPSTATE", "XTIMAPINFO",
	"XTICHKADDR", "XTIREJECT",
	"XTICOPYTP", "XTIUNBIND",
	"TRACE",
};
#endif


