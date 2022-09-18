#ifndef lint
static	char	*sccsid = "@(#)ns_forw.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984-1988 by			*
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
 * Copyright (c) 1986 Regents of the University of California
 *	All Rights Reserved
 * static char sccsid[] = "@(#)ns_forw.c	4.26 (Berkeley) 3/28/88";
 */

/*
 * Modification History:
 *
 * 18-Jan-88	logcher
 *	Added BIND 4.7.2.
 *
 * 26-Jan-88	logcher
 *	Added BIND 4.7.3.
 *
 * 08-Mar-88	logcher
 *	Added a & to the bcmp with a sockaddr in ns_forw per bug fix
 *	from bind-test mailing list dated, 19-Feb-88
 *
 * 17-May-89	logcher
 *	Added BIND 4.8.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/time.h>
#ifdef ULTRIXFUNC
#include <sys/stat.h>
#endif ULTRIXFUNC
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <arpa/nameser.h>
#include "ns.h"
#include "db.h"

struct	qinfo *qhead = QINFO_NULL;	/* head of allocated queries */
struct	qinfo *retryqp = QINFO_NULL;	/* list of queries to retry */
struct	fwdinfo *fwdtab;		/* list of forwarding hosts */

int	nsid;				/* next forwarded query id */
extern int forward_only;		/* you are only a slave */
extern int errno;
extern short ns_port;

#ifdef AUTHEN
extern char *res_dotname_head();
#endif AUTHEN

time_t	retrytime();

#ifdef ULTRIXFUNC
struct fwdinfo *fwd_copy(fwd_src)
  struct fwdinfo *fwd_src;
{
	struct fwdinfo *fwd_dest = NULL;
	struct fwdinfo *src_ptr;
	struct fwdinfo *dest_ptr;
	struct fwdinfo *temp_ptr;

	for(src_ptr = fwd_src; src_ptr != NULL; src_ptr = src_ptr->next)
	{		
		if((temp_ptr = (struct fwdinfo *)
		    malloc(sizeof(struct fwdinfo))) == NULL)
			return(NULL);

		if(fwd_dest == NULL) {
			fwd_dest = dest_ptr = temp_ptr;
		} else {
			dest_ptr->next = temp_ptr;
			dest_ptr = temp_ptr;			
		}
		*dest_ptr = *src_ptr;
		dest_ptr->next = NULL;
	}
	return(fwd_dest);
}
#endif ULTRIXFUNC


/*
 * Forward the query to get the answer since its not in the database.
 * Returns FW_OK if a request struct is allocated and the query sent.
 * Returns FW_DUP if this is a duplicate of a pending request. 
 * Returns FW_NOSERVER if there were no addresses for the nameservers.
 * Returns FW_SERVFAIL on malloc error.
 * (no action is taken on errors and qpp is not filled in.)
 */
#ifdef AUTHEN
ns_forw(nsp, msg, msglen, fp, qsp, dfd, qpp, query_in, query_out, authentype_in, authenver_in, authentype_out, authenver_out, ns_ad, query_dname, query_type, query_class)
	struct databuf *nsp[];
	char *msg;
	int msglen;
	struct sockaddr_in *fp;
	struct qstream *qsp;
	int dfd;
	struct qinfo **qpp;
	int query_in;
	int query_out;
	int authentype_in;
	int authenver_in;
	int authentype_out;
	int authenver_out;
	AUTH_DAT ns_ad;
	char *query_dname;
	int query_type;
	int query_class;
{
#else AUTHEN
ns_forw(nsp, msg, msglen, fp, qsp, dfd, qpp)
	struct databuf *nsp[];
	char *msg;
	int msglen;
	struct sockaddr_in *fp;
	struct qstream *qsp;
	int dfd;
	struct qinfo **qpp;
{
#endif AUTHEN
	register struct qinfo *qp;
	HEADER *hp;
	u_short id;
	extern char *calloc();

#ifdef AUTHEN
	char *krbnameptr;
	char *dnameptr;
	int authen_len;
#endif AUTHEN

#ifdef DEBUG
	if (debug >= 3)
		fprintf(ddt,"ns_forw()\n");
#endif

	/* Don't forward if we're already working on it. */
	hp = (HEADER *) msg;
	id = hp->id;
	hp->rd = 0;
	/* Look at them all */
	for (qp = qhead; qp!=QINFO_NULL; qp = qp->q_link) {
		if (qp->q_id == id &&
		    bcmp((char *)&qp->q_from, fp, sizeof(qp->q_from)) == 0 &&
		    (qp->q_cmsglen == 0 && qp->q_msglen == msglen &&
		     bcmp((char *)qp->q_msg+2, msg+2, msglen-2) == 0) ||
		    (qp->q_cmsglen == msglen &&
		     bcmp((char *)qp->q_cmsg+2, msg+2, msglen-2) == 0)) {
#ifdef DEBUG
			if (debug >= 3)
				fprintf(ddt,"forw: dropped DUP id=%d\n", ntohs(id));
#endif
#ifdef STATS
			stats[S_DUPQUERIES].cnt++;
#endif
			return (FW_DUP);
		}
	}

	qp = qnew();
#ifdef ULTRIXFUNC
	if (nslookup(nsp, qp) == 0 && !(forward_only && fwdtab)) {
#else ULTRIXFUNC
	if (nslookup(nsp, qp) == 0) {
#endif ULTRIXFUNC

#ifdef DEBUG
		if (debug >= 2)
			fprintf(ddt,"forw: no nameservers found\n");
#endif
		qfree(qp);
		hp->rd = 1;
		return (FW_NOSERVER);
	}
	qp->q_stream = qsp;
	qp->q_curaddr = 0;
#ifdef ULTRIXFUNC
	qp->q_fwd = fwd_copy(fwdtab);
	qp->q_fwdorig = qp->q_fwd;
#else ULTRIXFUNC
	qp->q_fwd = fwdtab;
#endif ULTRIXFUNC
	qp->q_dfd = dfd;
	qp->q_id = id;
#ifdef AUTHEN
	qp->q_ad = ns_ad;
	qp->q_type_in = query_in;
	qp->q_type_out = query_out;
	qp->q_authentype_in =  authentype_in;
	qp->q_authenver_in = authenver_in;
	qp->q_authentype_out = authentype_out;
	qp->q_authenver_out = authenver_out;
	strcpy(qp->q_dname, query_dname);
	qp->q_type = query_type;
	qp->q_class = query_class;
	strcpy(qp->q_dname_curr, query_dname);
	qp->q_type_curr = query_type;
	qp->q_class_curr = query_class;
#endif AUTHEN
	hp->id = qp->q_nsid = htons((u_short)++nsid);
	hp->ancount = 0;
	hp->nscount = 0;
	hp->arcount = 0;
	if (qp->q_fwd)
		hp->rd = 1;
	else
		qp->q_addr[0].stime = tt;
	qp->q_from = *fp;
#ifdef AUTHEN
	if ((qp->q_msg = malloc(BUFSIZ)) == NULL) {
		syslog(LOG_ERR, "forw: %m");
		qfree(qp);
		return (FW_SERVFAIL);
	}
#else AUTHEN
	if ((qp->q_msg = malloc((unsigned)msglen)) == NULL) {
		syslog(LOG_ERR, "forw: %m");
		qfree(qp);
		return (FW_SERVFAIL);
	}
#endif AUTHEN
	bcopy(msg, qp->q_msg, qp->q_msglen = msglen);
#ifdef AUTHEN
	if((hp->opcode == AQUERY && authentype_out == AUTH_KRB &&
		       authenver_out == ONE)) {
		krbnameptr = res_dotname_head(Q_NEXTNAME(qp, 0));
		dnameptr = query_dname;
		if (query_type != T_SOA && query_type != T_AXFR)
			res_dotname_rmhead(&dnameptr);

		hp->arcount = htons(1);

		if((authen_len = res_mkl_krbcred(ONE,
						 "named",
						 krbnameptr,
						 Q_NEXTCRED(qp,0),
						 msg,
						 msg + msglen,
						 BUFSIZ - msglen,
						 dnameptr, query_type,
						 query_class)) < RET_OK) {
			free(krbnameptr);
			qfree(qp);
			return(FW_SERVFAIL);
		}
		free(krbnameptr);
		msglen += authen_len;
	}

#endif AUTHEN

#ifdef ULTRIXFUNC
	schedretry(qp, qp->q_fwd ? (2*RETRYBASE): retrytime(qp) );
#else ULTRIXFUNC
	schedretry(qp, retrytime(qp));
#endif ULTRIXFUNC

#ifdef DEBUG
	if (debug)
#ifdef ULTRIXFUNC
		fprintf(ddt,
		   "forw: forw -> %s %d (%d) nsid=%d id=%d %dms retry %d sec\n",
			inet_ntoa(Q_NEXTADDR(qp,0)->sin_addr),
			ds, ntohs(Q_NEXTADDR(qp,0)->sin_port),
			ntohs(qp->q_nsid), ntohs(qp->q_id),
			((qp->q_addr[0].nsdata) ? qp->q_addr[0].nsdata->d_nstime: -1),
			qp->q_time - tt.tv_sec);
#else ULTRIXFUNC
		fprintf(ddt,
		   "forw: forw -> %s %d (%d) nsid=%d id=%d %dms retry %d sec\n",
			inet_ntoa(Q_NEXTADDR(qp,0)->sin_addr),
			ds, ntohs(Q_NEXTADDR(qp,0)->sin_port),
			ntohs(qp->q_nsid), ntohs(qp->q_id),
			qp->q_addr[0].nsdata->d_nstime,
			qp->q_time - tt.tv_sec);
#endif ULTRIXFUNC
	if ( debug >= 10)
		fp_query(msg, ddt);
#endif
	if (sendto(ds, msg, msglen, 0, (struct sockaddr *)Q_NEXTADDR(qp,0),
		   sizeof(struct sockaddr_in)) < 0){
#ifdef DEBUG
	if (debug >= 5)
			fprintf(ddt,"error returning msg errno=%d\n",errno);
#endif
	}
#ifdef STATS
	stats[S_OUTPKTS].cnt++;
#endif
	if (qpp)
		*qpp = qp;
	hp->rd = 0;
	return (0);
}


/*
 * Lookup the address for each nameserver in `nsp' and add it to
 * the list saved in the qinfo structure.
 */
nslookup(nsp, qp)
	struct databuf *nsp[];
	register struct qinfo *qp;
{
	register struct namebuf *np;
	register struct databuf *dp, *nsdp;
	register struct qserv *qs;
	register int n, i;
	struct hashbuf *tmphtp;
	char *dname, *fname;
	int oldn, naddr, class, found_arr;
	time_t curtime;
	int qcomp();

#ifdef DEBUG
	if (debug >= 3)
		fprintf(ddt,"nslookup(nsp=x%x,qp=x%x)\n",nsp,qp);
#endif

	naddr = n = qp->q_naddr;
	curtime = (u_long) tt.tv_sec;
	while ((nsdp = *nsp++) != NULL) {
		class = nsdp->d_class;
		dname = nsdp->d_data;
#ifdef DEBUG
		if (debug >= 3)
			fprintf(ddt,"nslookup: NS %s c%d t%d (x%x)\n",
				dname, class, nsdp->d_type, nsdp->d_flags);
#endif
		/* don't put in people we have tried */
		for (i = 0; i < qp->q_nusedns; i++)
			if (qp->q_usedns[i] == nsdp) {
#ifdef DEBUG
				if (debug >= 2)
fprintf(ddt, "skipping used NS w/name %s\n", nsdp->d_data);
#endif DEBUG
				goto skipserver;
			}

		tmphtp = ((nsdp->d_flags & DB_F_HINT) ? fcachetab : hashtab);
		np = nlookup(dname, &tmphtp, &fname, 1);
		if (np == NULL || fname != dname) {
#ifdef DEBUG
			if (debug >= 3)
			    fprintf(ddt,"%s: not found %s %x\n",dname,fname,np);
#endif
			continue;
		}
		found_arr = 0;
		oldn = n;
		/* look for name server addresses */
		for (dp = np->n_data; dp != NULL; dp = dp->d_next) {
			if (dp->d_type != T_A || dp->d_class != class)
				continue;
			/*
			 * Don't use records that may become invalid to
			 * reference later when we do the rtt computation.
			 * Never delete our safety-belt information!
			 */
			if ((dp->d_zone == 0) &&
			    (dp->d_ttl < (curtime+900)) &&
			    !(dp->d_flags & DB_F_HINT) )
		        {
#ifdef DEBUG
				if (debug >= 3)
					fprintf(ddt,"nslookup: stale entry '%s'\n",
					    np->n_dname);
#endif
				/* Cache invalidate the NS RR's */
				if (dp->d_ttl < curtime)
					delete_all(np, class, T_A);
				n = oldn;
				break;
			}

			found_arr++;
			/* don't put in duplicates */
			qs = qp->q_addr;
			for (i = 0; i < n; i++, qs++)
				if (bcmp((char *)&qs->ns_addr.sin_addr,
				    dp->d_data, sizeof(struct in_addr)) == 0)
					goto skipaddr;
			qs->ns_addr.sin_family = AF_INET;
			qs->ns_addr.sin_port = (u_short)ns_port;
			qs->ns_addr.sin_addr = 
				    *(struct in_addr *)dp->d_data;
			qs->ns = nsdp;
			qs->nsdata = dp;
			qp->q_addr[n].nretry = 0;
#ifdef AUTHEN
			strcpy(qp->q_addr[n].ns_name,dname);
			qp->authen_msglen = 0;
#endif AUTHEN
			n++;
			if (n >= NSMAX)
				goto out;
	skipaddr:	;
		}
#ifdef DEBUG
		if (debug >= 3)
			fprintf(ddt,"nslookup: %d ns addrs\n", n);
#endif
		if (found_arr == 0 && qp->q_system == 0)
			(void) sysquery(dname, class, T_A);
skipserver:	;
	}
out:
#ifdef DEBUG
	if (debug >= 3)
		fprintf(ddt,"nslookup: %d ns addrs total\n", n);
#endif
	qp->q_naddr = n;
	if (n > 1)
		qsort((char *)qp->q_addr, n, sizeof(struct qserv), qcomp);
	return (n - naddr);
}

qcomp(qs1, qs2)
	struct qserv *qs1, *qs2;
{

	return (qs1->nsdata->d_nstime - qs2->nsdata->d_nstime);
}

/*
 * Arrange that forwarded query (qp) is retried after t seconds.
 */
schedretry(qp, t)
	struct qinfo *qp;
	time_t t;
{
	register struct qinfo *qp1, *qp2;

#ifdef DEBUG
	if (debug > 3) {
		fprintf(ddt,"schedretry(%#x, %dsec)\n", qp, t);
		if (qp->q_time)
		   fprintf(ddt,"WARNING: schedretry(%x,%d) q_time already %d\n", qp->q_time);
	}
#endif
	t += (u_long) tt.tv_sec;
	qp->q_time = t;

	if ((qp1 = retryqp) == NULL) {
		retryqp = qp;
		qp->q_next = NULL;
		return;
	}
	while ((qp2 = qp1->q_next) != NULL && qp2->q_time < t)
		qp1 = qp2;

#ifdef ULTRIXFUNC
	if(qp1 == retryqp)
		if(qp1->q_time > t)
		{
			retryqp = qp;
			retryqp->q_next= qp1;
			return;		
		}
#endif ULTRIXFUNC

	qp1->q_next = qp;
	qp->q_next = qp2;
}

/*
 * Unsched is called to remove a forwarded query entry.
 */
unsched(qp)
	struct qinfo *qp;
{
	register struct qinfo *np;

#ifdef DEBUG
	if (debug > 3) {
		fprintf(ddt,"unsched(%#x, %d )\n", qp, ntohs(qp->q_id));
	}
#endif
	if( retryqp == qp )  {
		retryqp = qp->q_next;
	} else {
		for( np=retryqp; np->q_next != QINFO_NULL; np = np->q_next ) {
			if( np->q_next != qp)
				continue;
			np->q_next = qp->q_next;	/* dequeue */
			break;
		}
	}
	qp->q_next = QINFO_NULL;		/* sanity check */
	qp->q_time = 0;
}

/*
 * Retry is called to retransmit query 'qp'.
 */
retry(qp)
	register struct qinfo *qp;
{
	register int n;
	register HEADER *hp;
#ifdef AUTHEN
	char *dnameptr;
	int authen_len;
	int m;
	char *krbnameptr;
	char *src;
	char *dest;
#endif AUTHEN

#ifdef DEBUG
	if (debug > 3)
		fprintf(ddt,"retry(x%x) id=%d\n", qp, ntohs(qp->q_id));
#endif
	if((HEADER *)qp->q_msg == NULL) {		/*** XXX ***/
		qremove(qp);
		return;
	}						/*** XXX ***/

	/* try next address */
	n = qp->q_curaddr;
	if (qp->q_fwd) {
		qp->q_fwd = qp->q_fwd->next;
		if (qp->q_fwd)
			goto found;
		/* out of forwarders, try direct queries */
	} else
		++qp->q_addr[n].nretry;
	if (!forward_only) {
		do {
			if (++n >= qp->q_naddr)
				n = 0;
			if (qp->q_addr[n].nretry < MAXRETRY)
				goto found;
		} while (n != qp->q_curaddr);
	}
	/*
	 * Give up. Can't reach destination.
	 */
	hp = (HEADER *)(qp->q_cmsg ? qp->q_cmsg : qp->q_msg);
	if (qp->q_system == PRIMING_CACHE) {
		/* Can't give up priming */
		unsched(qp);
		schedretry(qp, (time_t)60*60);	/* 1 hour */
		hp->rcode = NOERROR;	/* Lets be safe, reset the query */
		hp->qr = hp->aa = 0;
#ifdef ULTRIXFUNC
		qp->q_fwd = qp->q_fwdorig;
#else ULTRIXFUNC
		qp->q_fwd = fwdtab;
#endif ULTRIXFUNC
		for (n = 0; n < qp->q_naddr; n++)
			qp->q_addr[n].nretry = 0;

		return;
	}
#ifdef DEBUG
	if (debug >= 5)
		fprintf(ddt,"give up\n");
#endif
	n = ((HEADER *)qp->q_cmsg ? qp->q_cmsglen : qp->q_msglen);
	hp->id = qp->q_id;
	hp->qr = 1;
	hp->ra = 1;
	hp->rd = 1;
	hp->rcode = SERVFAIL;
#ifdef DEBUG
	if (debug >= 10)
		fp_query(qp->q_msg, ddt);
#endif

#ifdef AUTHEN
	authen_len = dn_skipname((u_char *)hp + sizeof(HEADER),
				 (u_char *)hp + n);

	if(hp->opcode = AQUERY && qp->q_type_in == QUERY &&
	   qp->q_type_out == AQUERY && n - (sizeof(HEADER) +
					    authen_len + AQFIXEDSZ) > 0) {
		if(fixup_rrs((char *)hp + sizeof(HEADER) + authen_len +
			     AQFIXEDSZ,
			     n - (sizeof(HEADER) + authen_len + AQFIXEDSZ),
			     (int)(sizeof(HEADER) + authen_len),
			     -(2 * sizeof(u_short))) < 0) {
			qremove(qp);
			return;
		}
		dest = authen_len + sizeof(HEADER) + (2 * sizeof(u_short)) +
		  (char *)hp;
		src = dest + (2 * sizeof(u_short));
		bcopy(src, dest, n - (src - (char *)hp));
		hp->opcode = QUERY;
		n -= 2 * sizeof(u_short);
	}

	if((hp->opcode == AQUERY && qp->q_authentype_in == AUTH_KRB &&
		       qp->q_authenver_in == ONE)) {
		dnameptr = qp->q_dname;
		if (qp->q_type != T_SOA && qp->q_type != T_AXFR)
			res_dotname_rmhead(&dnameptr);

		hp->arcount = htons(1);

		if((authen_len = res_mks_krbcred(ONE,
						 "", "",
						 qp->q_ad,
						 hp, hp + n,
						 BUFSIZ - n,
						 dnameptr, qp->q_type,
						 qp->q_class)) < RET_OK) {
			qremove(qp);
			return;
		}
		n += authen_len;
	}
#endif AUTHEN
	
	if (send_msg((char *)hp, n, qp)) {
#ifdef DEBUG
		if (debug)
			fprintf(ddt,"gave up retry(x%x) nsid=%d id=%d\n",
				qp, ntohs(qp->q_nsid), ntohs(qp->q_id));
#endif
	}
	qremove(qp);
	return;

found:
	if (qp->q_fwd == 0 && qp->q_addr[n].nretry == 0)
		qp->q_addr[n].stime = tt;
	qp->q_curaddr = n;
	hp = (HEADER *)qp->q_msg;
	hp->rd = (qp->q_fwd ? 1 : 0);
#ifdef DEBUG
	if (debug)
	    if(qp->q_addr[n].nsdata)
		fprintf(ddt,"%s(addr=%d n=%d) -> %s %d (%d) nsid=%d id=%d %dms\n",
			(qp->q_fwd ? "reforw" : "resend"),
			n, qp->q_addr[n].nretry,
			inet_ntoa(Q_NEXTADDR(qp,n)->sin_addr),
			ds, ntohs(Q_NEXTADDR(qp,n)->sin_port),
			ntohs(qp->q_nsid), ntohs(qp->q_id),
			qp->q_addr[n].nsdata->d_nstime);
	    else
		fprintf(ddt,"%s(addr=%d n=%d) -> %s %d (%d) nsid=%d id=%d\n",
			(qp->q_fwd ? "reforw" : "resend"),
			n, qp->q_addr[n].nretry,
			inet_ntoa(Q_NEXTADDR(qp,n)->sin_addr),
			ds, ntohs(Q_NEXTADDR(qp,n)->sin_port),
			ntohs(qp->q_nsid), ntohs(qp->q_id));
	if ( debug >= 10)
		fp_query(qp->q_msg, ddt);
#endif. 
	/* NOSTRICT */

#ifdef AUTHEN
	m = qp->q_msglen;

	if((hp->opcode == AQUERY && qp->q_authentype_out == AUTH_KRB &&
			qp->q_authenver_out == ONE)) {
		krbnameptr = res_dotname_head(Q_NEXTNAME(qp, n));
		dnameptr = qp->q_dname_curr;
		if (qp->q_type_curr != T_SOA && qp->q_type_curr != T_AXFR)
			res_dotname_rmhead(&dnameptr);

		hp->arcount = htons(1);

		if((authen_len = res_mkl_krbcred(ONE,
						 "named",
						 krbnameptr,
						 Q_NEXTCRED(qp,n),
						 hp,
						 m + (char *)hp,
						 BUFSIZ - m,
						 dnameptr, qp->q_type_curr,
						 qp->q_class_curr)) < RET_OK) {
			free(krbnameptr);
			qremove(qp);
			return;
		}
		free(krbnameptr);
		m += authen_len;
	}
#endif AUTHEN
	
#ifdef AUTHEN
	if (sendto(ds, qp->q_msg, m, 0,
	    (struct sockaddr *)Q_NEXTADDR(qp,n),
	    sizeof(struct sockaddr_in)) < 0){
#else AUTHEN
	if (sendto(ds, qp->q_msg, qp->q_msglen, 0,
	    (struct sockaddr *)Q_NEXTADDR(qp,n),
	    sizeof(struct sockaddr_in)) < 0){
#endif AUTHEN

#ifdef DEBUG
		if (debug > 3)
			fprintf(ddt,"error resending msg errno=%d\n",errno);
#endif
	}
	hp->rd = 0;	/* leave set to 0 for dup detection */
#ifdef STATS
	stats[S_OUTPKTS].cnt++;
#endif
	unsched(qp);
	schedretry(qp, qp->q_fwd ? (2*RETRYBASE) : retrytime(qp));

	return;
}

/*
 * Compute retry time for the next server for a query.
 * Use a minimum time of RETRYBASE (4 sec.) or twice the estimated
 * service time; * back off exponentially on retries, but place a 45-sec.
 * ceiling on retry times for now.  (This is because we don't hold a reference
 * on servers or their addresses, and we have to finish before they time out.)
 */
time_t
retrytime(qp)
register struct qinfo *qp;
{
	time_t t;
	struct qserv *ns = &qp->q_addr[qp->q_curaddr];

#ifdef DEBUG
	if (debug > 3)
		fprintf(ddt,"retrytime: nstime %dms.\n",
		    ns->nsdata->d_nstime / 1000);
#endif
	t = (time_t) MAX(RETRYBASE, 2 * ns->nsdata->d_nstime / 1000);
	t <<= ns->nretry;
	t = MIN(t, 45);			/* max. retry timeout for now */
#ifdef notdef
	if (qp->q_system)
		return ((2 * t) + 5);	/* system queries can wait. */
#endif
	return (t);
}

qflush()
{
	while (qhead)
		qremove(qhead);
	qhead = QINFO_NULL;
}

qremove(qp)
register struct qinfo *qp;
{
#ifdef DEBUG
	if(debug > 3)
		fprintf(ddt,"qremove(x%x)\n", qp);
#endif
	unsched(qp);			/* get off queue first */
	qfree(qp);
}

struct qinfo *
qfindid(id)
register u_short id;
{
	register struct qinfo *qp;

#ifdef DEBUG
	if(debug > 3)
		fprintf(ddt,"qfindid(%d)\n", ntohs(id));
#endif
	for (qp = qhead; qp!=QINFO_NULL; qp = qp->q_link) {
		if (qp->q_nsid == id)
			return(qp);
	}
#ifdef DEBUG
	if (debug >= 5)
		fprintf(ddt,"qp not found\n");
#endif
	return(NULL);
}

struct qinfo *
qnew()
{
	register struct qinfo *qp;

	if ((qp = (struct qinfo *)calloc(1, sizeof(struct qinfo))) == NULL) {
#ifdef DEBUG
		if (debug >= 5)
			fprintf(ddt,"qnew: calloc error\n");
#endif
		syslog(LOG_ERR, "forw: %m");
		exit(12);
	}
#ifdef DEBUG
	if (debug >= 5)
		fprintf(ddt,"qnew(x%x)\n", qp);
#endif
	qp->q_link = qhead;
	qhead = qp;
	return( qp );
}

qfree(qp)
struct qinfo *qp;
{
	register struct qinfo *np;
#ifdef ULTRIXFUNC
	struct fwdinfo *fwd_ptr;
	struct fwdinfo *temp_ptr;
#endif ULTRIXFUNC

#ifdef DEBUG
	if(debug > 3)
		fprintf(ddt,"qfree( x%x )\n", qp);
	if(debug && qp->q_next)
		fprintf(ddt,"WARNING:  qfree of linked ptr x%x\n", qp);
#endif
	if (qp->q_msg)
	 	free(qp->q_msg);
 	if (qp->q_cmsg)
 		free(qp->q_cmsg);
	if( qhead == qp )  {
		qhead = qp->q_link;
	} else {
		for( np=qhead; np->q_link != QINFO_NULL; np = np->q_link )  {
			if( np->q_link != qp )  continue;
			np->q_link = qp->q_link;	/* dequeue */
			break;
		}
	}
#ifdef ULTRIXFUNC
	for(fwd_ptr = qp->q_fwdorig; fwd_ptr != NULL;
		temp_ptr = fwd_ptr, fwd_ptr = fwd_ptr->next, free(temp_ptr));
#endif ULTRIXFUNC

	(void)free((char *)qp);
}
