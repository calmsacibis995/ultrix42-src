#ifndef lint
static	char	*sccsid = "@(#)resolve.c	4.2	(ULTRIX)	9/4/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1984-1989 by			*
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
 * Author: Win Treese - MIT Athena
 */
/*
 * Modification History:
 *
 * 11-Aug-89	sue
 *	Increased buffers from PACKETSZ to HES_BUFMAX.  Removed flag
 *	set RES_IGNTC because we do not want to ignore truncation
 *	errors.
 *
 * 17-May-89	logcher
 *	Created.
 */

#define _RESOLVE_C_

#include <strings.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <resscan.h>
#include <hesiod.h>

#if defined(lint) && !defined(DEBUG)
#define DEBUG
#endif

#define DEF_RETRANS 4
#define DEF_RETRY 3

#ifdef AUTHEN
extern struct sockaddr_in authen_from;
#endif AUTHEN

extern int errno;

static caddr_t
rr_scan(cp, rr)
    char *cp;
    rr_t *rr;
{
    register int n;

    if ((n = dn_skip(cp)) < 0) {
        errno = EINVAL;
        return((char *)NULL);
    }

    cp += n;
    rr->type = _getshort(cp);
    cp += sizeof(u_short/*type*/);

    rr->class = _getshort(cp);
    cp += sizeof(u_short/*class*/) + sizeof(u_long/*ttl*/);

    rr->dlen = (int)_getshort(cp);
    rr->data = cp + sizeof(u_short/*dlen*/);

    return(rr->data + rr->dlen);
}

#ifdef AUTHEN
nsmsg_p
res_scan(msg, length, authtype, authversion, nameser)
    char *msg;
    int length;
    int authtype;
    int authversion;
    int nameser;
{
#else /* AUTHEN */
nsmsg_p
res_scan(msg)
    char *msg;
{
#endif AUTHEN
    static char bigmess[sizeof(nsmsg_t) + sizeof(rr_t)*((HES_BUFMAX-sizeof(HEADER))/RRFIXEDSZ)];
    static char datmess[HES_BUFMAX-sizeof(HEADER)];
    register char *cp;
    register rr_t *rp;
    register HEADER *hp;
    register char *data = datmess;
    register int n, n_an, n_ns, n_ar, nrec;
    register nsmsg_t *mess = (nsmsg_t *)bigmess;

#ifdef AUTHEN
    int m;
    char *ocp;
    int	authenticated = 0;
    char dnbuf[MAXDNAME];
    char *dnptr;
    int type_query;
    int class_query;
    int authtypemsg;
    int authversionmsg;
    int read_autheninfo;
    char *krbnameptr;
    extern char *res_dotname_head();
#endif AUTHEN

    hp = (HEADER *)msg;
    cp = msg + sizeof(HEADER);
    n_an = ntohs(hp->ancount);
    n_ns = ntohs(hp->nscount);
    n_ar = ntohs(hp->arcount);
    nrec = n_an + n_ns + n_ar;

    mess->len = 0;
    mess->hd = hp;
    mess->ns_off = n_an;
    mess->ar_off = n_an + n_ns;
    mess->count = nrec;
    rp = &mess->rr;

#ifdef AUTHEN
    read_autheninfo = 0;
#endif AUTHEN

    /* skip over questions */
    if (n = ntohs(hp->qdcount)) {
        while (--n >= 0) {
            register int i;
#ifdef AUTHEN 
	    if(hp->opcode == AQUERY) {
		    if((m = dn_skip(cp)) < 0)
			    return((nsmsg_t *)NULL);

		    cp += m + (2 * sizeof(u_short));
		    authtypemsg = _getshort(cp);
		    cp += sizeof(u_short);
		    authversionmsg = _getshort(cp);
		    cp += sizeof(u_short);
		    if(authtypemsg != authtype || authversionmsg !=authversion)
			    return((nsmsg_t *)NULL);
	    } else {
		    if ((i = dn_skip(cp)) < 0)
			    return((nsmsg_t *)NULL);

		    cp += i + (sizeof(u_short/*type*/) +
			       sizeof(u_short/*class*/));		    
	    }
#else
            if ((i = dn_skip(cp)) < 0)
                return((nsmsg_t *)NULL);

	    cp += i + (sizeof(u_short/*type*/) + sizeof(u_short/*class*/));
#endif AUTHEN
        }
    }

    /* scan answers */
    if (n = n_an) {
        while (--n >= 0) {
#ifdef AUTHEN
	    if(n + 1 == n_an) {
	      read_autheninfo = 1;
	      if((m = dn_expand(hp, hp + length, cp,
				dnbuf, sizeof(dnbuf))) < 0)
		return((nsmsg_t *)NULL);
	    }
#endif AUTHEN
            if ((cp = rr_scan(cp, rp)) == NULL)
                return((nsmsg_t *)NULL);
#ifdef AUTHEN
	    if(n + 1 == n_an) {
		    type_query = rp->type;
		    class_query = rp->class;
	    }
#endif AUTHEN
            (void) strncpy(data, rp->data, rp->dlen);
            rp->data = data;
            data += rp->dlen;
            *data++ = '\0';
            rp++;
        }
    }

    /* scan name servers */
    if (n = n_ns) {
        while (--n >= 0) {
#ifdef AUTHEN
	    if(n + 1 == n_ns && !read_autheninfo) {
	      if((m = dn_expand(hp, hp + length, cp,
				dnbuf, sizeof(dnbuf))) < 0)
		return((nsmsg_t *)NULL);
	    }
#endif AUTHEN
            if ((cp = rr_scan(cp, rp)) == NULL)
                return((nsmsg_t *)NULL);
#ifdef AUTHEN
	    if(n + 1 == n_ns && !read_autheninfo) {
		    type_query = rp->type;
		    class_query = rp->class;
	    }
#endif AUTHEN
            (void) strncpy(data, rp->data, rp->dlen);
            rp->data = data;
            data += rp->dlen;
            *data++ = '\0';
            rp++;
        }
    }

    /* scan additional records */
    if (n = n_ar) {
        while (--n >= 0) {
#ifdef AUTHEN
	    ocp = cp;
#endif AUTHEN
            if ((cp = rr_scan(cp, rp)) == NULL)
                return((nsmsg_t *)NULL);
#ifdef AUTHEN
	    dnptr = dnbuf;
	    if(type_query != T_SOA && type_query != T_AXFR)
	      res_dotname_rmhead(&dnptr);

	    krbnameptr = res_dotname_head(_res.ns_list[nameser].dname);

	    if (authtype == AUTH_KRB && authversion == ONE)
		    if(res_rds_krbcred(authversion,
				       "named",
				       krbnameptr,
				       NULL, hp, ocp, cp - ocp,
				       dnptr, type_query, class_query
				       ) >= RET_OK)
			    authenticated = 1;

	    free(krbnameptr);

            (void) bcopy(rp->data, data, rp->dlen);
#else AUTHEN
            (void) strncpy(data, rp->data, rp->dlen);
#endif AUTHEN
            rp->data = data;
            data += rp->dlen;
            *data++ = '\0';
            rp++;
        }
    }

    mess->len = (int)cp - (int)msg;

#ifdef AUTHEN
    if (authtype != AUTH_NONE && !authenticated){
#ifdef DEBUG
	    if (_res.options & RES_DEBUG)
		    printf(" bad authentication of query.\n");
#endif	    
	    return((nsmsg_t *)NULL);
    }
#endif AUTHEN

    return(mess);
}

/*
 * Resolve name into data records
 */

nsmsg_p
_resolve(name, class, type, patience)
    char *name;
    int class, type;
    retransXretry_t patience;
{
    static char qbuf[HES_BUFMAX], abuf[HES_BUFMAX];
    register int n;
    register long res_options = _res.options;
    register int res_retrans = _res.retrans;
    register int res_retry = _res.retry;

#ifdef DEBUG
    if (_res.options & RES_DEBUG)
        printf("_resolve: class = %d, type = %d\n", class, type);
#endif

    if (class < 0 || type < 0) {
        errno = EINVAL;
        return((nsmsg_t *)NULL);
    }

    n = res_mkquery(QUERY, name, class, type, (char *)0, 0, NULL, qbuf, HES_BUFMAX);
    if (n < 0) {
        errno = EMSGSIZE;
        return((nsmsg_t *)NULL);
    }

    _res.retrans = (patience.retrans ? patience.retrans : DEF_RETRANS);
    _res.retry = (patience.retry ? patience.retry : DEF_RETRY);

    n = res_send(qbuf, n, abuf, HES_BUFMAX);

    _res.options = res_options;
    _res.retrans = res_retrans;
    _res.retry = res_retry;

    if (n < 0) {
        errno = ECONNREFUSED;
        return((nsmsg_t *)NULL);
    }

#ifdef AUTHEN
    return(res_scan(abuf, n, AUTH_NONE, T_CRED, -1));
#else AUTHEN
    return(res_scan(abuf));
#endif AUTHEN
}



#ifdef AUTHEN
/*
 * Resolve name into data records
 */

nsmsg_p
auth_resolve(name, class, type, patience)
    char *name;
    int class, type;
    retransXretry_t patience;
{
    register int n;
    register long res_options = _res.options;
    register int res_retrans = _res.retrans;
    register int res_retry = _res.retry;
    static char qbuf[HES_BUFMAX], abuf[HES_BUFMAX];
    char *qb;
    int version;
    int looper;
    int nameser;

#ifdef DEBUG
    if (_res.options & RES_DEBUG)
        printf("_resolve: class = %d, type = %d\n", class, type);
#endif

    if (class < 0 || type < 0) {
        errno = EINVAL;
        return((nsmsg_t *)NULL);
    }

    n = res_mkquery(AQUERY, name, class, type, (char *)0, 0, NULL,
		    qbuf, HES_BUFMAX);
    if (n < 0) {
        errno = EMSGSIZE;
        return((nsmsg_t *)NULL);
    }

    qb = qbuf;
    qb += sizeof(HEADER);
    qb += dn_skip(qb);
    qb += sizeof(u_short);
    qb += sizeof(u_short);
    type = _getshort(qb);
    qb += sizeof(u_short);
    version = _getshort(qb);

    _res.retrans = (patience.retrans ? patience.retrans : DEF_RETRANS);
    _res.retry = (patience.retry ? patience.retry : DEF_RETRY);

    n = res_send(qbuf, n, abuf, HES_BUFMAX);

    for(looper = 0; looper < _res.nscount; looper++) {
	    if (authen_from.sin_addr.s_addr ==
		_res.ns_list[looper].addr.sin_addr.s_addr)
		    break;
    }

    if(looper == _res.nscount)
	    return((nsmsg_t *)NULL);
    else
	    nameser = looper;

    _res.options = res_options;
    _res.retrans = res_retrans;
    _res.retry = res_retry;

    if (n < 0) {
        errno = ECONNREFUSED;
        return((nsmsg_t *)NULL);
    }
    
    return(res_scan(abuf, n, type, version, nameser));
}
#endif AUTHEN


/*
 * Skip over a compressed domain name. Return the size or -1.
 */
static
dn_skip(comp_dn)
	char *comp_dn;
{
	register char *cp;
	register int n;

	cp = comp_dn;
	while (n = *cp++) {
		/*
		 * check for indirection
		 */
		switch (n & INDIR_MASK) {
		case 0:		/* normal case, n == len */
			cp += n;
			continue;
		default:	/* illegal type */
			return (-1);
		case INDIR_MASK:	/* indirection */
			cp++;
		}
		break;
	}
	return (cp - comp_dn);
}
