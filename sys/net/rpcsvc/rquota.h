/*	@(#)rquota.h	4.1	(ULTRIX)	7/2/90	*/

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
 * remote quota inquiry protocol
 */

#define RQUOTAPROG	100011
#define RQUOTAVERS_ORIG	1
#define RQUOTAVERS	1

/*
 * inquire about quotas for uid (assume AUTH_UNIX)
 *	input - getquota_args			(xdr_getquota_args)
 *	output - getquota_rslt			(xdr_getquota_rslt)
 */
#define RQUOTAPROC_GETQUOTA		1	/* get quota */
#define RQUOTAPROC_GETACTIVEQUOTA	2	/* get only active quotas */

/*
 * args to RQUOTAPROC_GETQUOTA and RQUOTAPROC_GETACTIVEQUOTA
 */
struct getquota_args {
	char *gqa_pathp;		/* path to filesystem of interest */
	int gqa_uid;			/* inquire about quota for uid */
};

/*
 * remote quota structure
 */
struct rquota {
	int rq_bsize;			/* block size for block counts */
	bool_t rq_active;		/* indicates whether quota is active */
	u_long rq_bhardlimit;		/* absolute limit on disk blks alloc */
	u_long rq_bsoftlimit;		/* preferred limit on disk blks */
	u_long rq_curblocks;		/* current block count */
	u_long rq_fhardlimit;		/* absolute limit on allocated files */
	u_long rq_fsoftlimit;		/* preferred file limit */
	u_long rq_curfiles;		/* current # allocated files */
	u_long rq_btimeleft;		/* time left for excessive disk use */
	u_long rq_ftimeleft;		/* time left for excessive files */
};	

enum gqr_status {
	Q_OK = 1,			/* quota returned */
	Q_NOQUOTA = 2,			/* noquota for uid */
	Q_EPERM = 3			/* no permission to access quota */
};

struct getquota_rslt {
	enum gqr_status gqr_status;	/* discriminant */
	struct rquota gqr_rquota;	/* valid if status == Q_OK */
};

extern bool_t xdr_getquota_args();
extern bool_t xdr_getquota_rslt();
extern bool_t xdr_rquota();
