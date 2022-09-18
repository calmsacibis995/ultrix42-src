/*	@(#)resolv.h	4.1	(ULTRIX)	7/2/90	*/
/*
 * Copyright (c) 1983, 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)resolv.h	5.5 (Berkeley) 5/12/87
 */

/*
 * Global defines and variables for resolver stub.
 */


#define	MAXNS		3	/* max # name servers we'll track */
#define	MAXDNSRCH	3	/* max # default domain levels to try */
#define	LOCALDOMAINPARTS 2	/* min levels in name that is "local" */

#define	RES_TIMEOUT	4	/* seconds between retries */

struct cred_st {
	int len_ar;
	int v_circuit;
	char buf[PACKETSZ];
};
typedef struct cred_st cred_st;

struct nsinfo {
	struct sockaddr_in	addr;
	char			dname[MAXDNAME];
};
typedef struct nsinfo nsinfo_t;

struct state {
	int	retrans;	 	/* retransmition time interval */
	int	retry;			/* number of times to retransmit */
	long	options;		/* option flags - see below. */
	int	nscount;		/* number of name servers */
	nsinfo_t ns_list[MAXNS];	/* name server specific info */
#define	nsaddr	ns_list[0].addr		/* for backward compatibility */
	u_short	id;			/* current packet id */
	char	defdname[MAXDNAME];	/* default domain */
	char	*dnsrch[MAXDNSRCH+1];	/* components of domain to search */
#ifdef AUTHEN
	int	defauthtype;		/*default authentication type to use */
	int	defauthversion;		/*default authentication type to use */
#endif /* AUTHEN */
	int	mtime;			/* time structure last modified */
};

/*
 * Resolver options
 */
#define RES_INIT	0x0001		/* address initialized */
#define RES_DEBUG	0x0002		/* print debug messages */
#define RES_AAONLY	0x0004		/* authoritative answers only */
#define RES_USEVC	0x0008		/* use virtual circuit */
#define RES_PRIMARY	0x0010		/* query primary server only */
#define RES_IGNTC	0x0020		/* ignore trucation errors */
#define RES_RECURSE	0x0040		/* recursion desired */
#define RES_DEFNAMES	0x0080		/* use default domain name */
#define RES_STAYOPEN	0x0100		/* Keep TCP socket open */
#define RES_DNSRCH	0x0200		/* search up local domain tree */

#define RES_DEFAULT	(RES_RECURSE | RES_DEFNAMES | RES_DNSRCH)

extern struct state _res;
extern char *p_cdname(), *p_rr(), *p_type(), *p_class();

#ifdef AUTHEN

#ifndef RET_OK
#define RET_OK 0
#endif
#define RET_BAD -1

#define NUMMAPCREDTYPE	1
#define NUMMAPCREDVAL	1

struct map {
	char token[MAXDNAME];
	int val;
};
typedef struct map map_t;

#define KRBCRED_NOTSUPP  -2
#define KRBCRED_EXPAND   -3
#define KRBCRED_NAME     -4
#define KRBCRED_NAMECMP  -5
#define KRBCRED_CLASS    -6
#define KRBCRED_TYPE     -7
#define KRBCRED_AUTHTYPE -8
#define KRBCRED_AUTHVER  -9
#define KRBCRED_REALM    -10
#define KRBCRED_CRED     -11
#define KRBCRED_TIME     -12
#define KRBCRED_CKSUM    -13
#define KRBCRED_TOOSMALL -14
#define KRBCRED_FRESH    -15
#define KRBCRED_MKREQ    -16

#endif /* AUTHEN */







