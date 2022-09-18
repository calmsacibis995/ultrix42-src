#ifndef lint
static char *sccsid = "@(#)rpc.c	4.1	(ULTRIX)	7/17/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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

#include	"crash.h"

#include	<sys/smp_lock.h>

struct ucred	cred;

struct chtab {
	int	ch_timesused;
	bool_t	ch_inuse;
	CLIENT	*ch_client;
} chtable[MAXCLIENTS];
/*
 * Private data per rpc handle.  This structure is allocated by
 * clntkudp_create, and freed by cku_destroy.
 */
struct cku_private {
	u_int			 cku_flags;	/* see below */
	CLIENT			 cku_client;	/* client handle */
	int			 cku_retrys;	/* request retrys */
	struct socket		*cku_sock;	/* open udp socket */
	struct sockaddr_in	 cku_addr;	/* remote address */
	struct rpc_err		 cku_err;	/* error status */
	XDR			 cku_outxdr;	/* xdr routine for output */
	XDR			 cku_inxdr;	/* xdr routine for input */
	u_int			 cku_outpos;	/* position of in output mbuf */
	char			*cku_outbuf;	/* output buffer */
	char			*cku_inbuf;	/* input buffer */
	struct mbuf		*cku_inmbuf;	/* input mbuf */
	struct ucred		*cku_cred;	/* credentials */
/* SMP lock for protecting mbuf	data of cku_private	
 * This lock is used with CKU_BUFBUSY and CKU_BUFWANTED flags
 */
	struct	lock_t	cku_lk_outbuf;
};

/* cku_flags */
#define	CKU_TIMEDOUT	0x001
#define	CKU_BUSY	0x002
#define	CKU_WANTED	0x004
#define	CKU_BUFBUSY	0x008
#define	CKU_BUFWANTED	0x010

CLIENT client;
struct cku_private cku;

prclienthd()
{
	printf("Slot    Used     auth       ops        private    xid\n");
/*              sddd dddddddd 0xhhhhhhhh 0xhhhhhhhh 0xhhhhhhhh 0xhhhhhhhh  */
}
prclient(c, all)
	int c;
	int	all;
{
	if ((c<0) || (c>MAXCLIENTS))
		return(0);

	readsym(symsrch("_chtable"), chtable, sizeof(chtable));

	if (chtable[c].ch_timesused == 0)
		return;

	if (chtable[c].ch_inuse == 0)
		printf (" %3d ", c);
	else
		printf ("*%3d ", c);
	printf ("%8d ",chtable[c].ch_timesused);

	if (chtable[c].ch_client != NULL) {
		readmem(&client, chtable[c].ch_client, sizeof(client));
       
		printf("0x%8x ",client.cl_auth);
		printf("0x%8x ",client.cl_ops);
/*		praddr(client.cl_ops); */
		printf("0x%8x  ",client.cl_private);
		printf("0x%8x\n",client.cl_xid);
		if (all == 1) {
			if (client.cl_private != NULL) {
				readmem(&cku, client.cl_private, sizeof(cku));
				
				printf("cku flags:  %s%s%s%s%s\n",
				       cku.cku_flags & CKU_TIMEDOUT ? " time" : "",
				       cku.cku_flags & CKU_BUSY ? " busy" : "",
				       cku.cku_flags & CKU_WANTED ? " want" : "",
				       cku.cku_flags & CKU_BUFBUSY ? " bbusy" : "",
				       cku.cku_flags & CKU_BUFWANTED ? " bwant" : "");
				
				prinaddr(cku.cku_addr.sin_addr);
				printf("retrys: %3d ",cku.cku_retrys);
				printf("socket: 0x%8x ",cku.cku_sock);
				printf("cred: 0x%8x\n",cku.cku_cred);
				printf("errors: status %d errno %d why %d vers %d %d\n ",
				       cku.cku_err.re_status,cku.cku_err.re_errno,
				       cku.cku_err.re_why,cku.cku_err.re_vers.low,
				       cku.cku_err.re_vers.high);
				if (cku.cku_cred != NULL)
					print_cred(cku.cku_cred);
				
			}
		}
	}
}

pr_svcxprt(addr)
	unsigned addr;
{

	SVCXPRT xprt;


	readmem(&xprt, addr, sizeof(xprt));

	printf("Socket: 0x%8x Port %d\n",xprt.xp_sock,ntohs(xprt.xp_port));
	printf("Ops: ");
	praddr(xprt.xp_ops);

	printf(" addrlen %d\n Remote Host ", xprt.xp_addrlen);
	prinaddr(xprt.xp_raddr.sin_addr);

	printf(" verf 0x%8x p1 (rpc_buffer) 0x%8x p2 (udp_data) 0x%8x\n", 
	       xprt.xp_verf, xprt.xp_p1, xprt.xp_p2);
}

/*
 * Transport private data.
 * Kept in xprt->xp_p2.
 */
struct udp_data {
	int	ud_flags;			/* flag bits, see below */
	u_long 	ud_xid;				/* id */
	struct	mbuf *ud_inmbuf;		/* input mbuf chain */
	XDR	ud_xdrin;			/* input xdr stream */
	XDR	ud_xdrout;			/* output xdr stream */
	char	ud_verfbody[MAX_AUTH_BYTES];	/* verifier */
	struct	lock_t	u_lk_udpdata;		/* smp lock for type 2*/
						/* mbuf data  */
};

/*
 * Flags
 */
#define	UD_BUSY		0x001		/* buffer is busy */
#define	UD_WANTED	0x002		/* buffer wanted */


pr_udpdata(addr) 
	unsigned addr;
{
	struct udp_data ud;

	readmem(&ud, addr, sizeof(ud));
	printf("udp_data flags:  %s%s\n",
	       ud.ud_flags & UD_BUSY ? " busy" : " idle",
	       ud.ud_flags & UD_WANTED ? " want" : " unwanted");

	printf("Xid: %d\n",ud.ud_xid);
	printf("XDR in: 0x%8x  XDR out 0x%8x\n",
	       ud.ud_xdrin,ud.ud_xdrout);
	printf("Input mbuf: 0x%x\n", ud.ud_inmbuf);
	if (ud.ud_inmbuf != NULL)
		 prmbuf_chain(ud.ud_inmbuf);

}
extern char *nfsstr[];

pr_svcreq(addr)
	unsigned addr;
{
	struct svc_req rq;

	readmem(&rq, addr, sizeof(rq));

	printf("Prog: %d  Version: %\n",rq.rq_prog, rq.rq_vers);
	printf("Proc: %d %s\n",rq.rq_proc,nfsstr[rq.rq_proc]);

	printf("Raw Cred: 0x%8x client cred 0x%8x\n",
	       rq.rq_cred, rq.rq_clntcred);

	if (rq.rq_clntcred != NULL) 
		print_cred(rq.rq_clntcred);

	printf("SVCXPRT: 0x%8x\n",rq.rq_xprt);
	if (rq.rq_xprt != NULL)
		pr_svcxprt(rq.rq_xprt);


}

