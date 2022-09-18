/*	@(#)resscan.h	4.1	(ULTRIX)	7/2/90	*/
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
 * Description:  This file contains definitions for the name resolver
 *		 scanning routines used by the Hesiod library.  
 */
/*
 * Modification History:
 *
 * 17-May-89	logcher
 *	Created.
 *
 */

typedef struct rr {
    short type;			     /* RR type */
    short class;		     /* RR class */
    int dlen;			     /* len of data section */
    char *data;			     /* pointer to data */
} rr_t, *rr_p;

typedef struct nsmsg {
    int len;			     /* sizeof(msg) */
    int ns_off;			     /* offset to name server RRs */
    int ar_off;			     /* offset to additional RRs */
    int count;			     /* total number of RRs */
    HEADER *hd;			     /* message header */
    rr_t rr;			     /* vector of (stripped-down) RR descriptors */
} nsmsg_t, *nsmsg_p;

typedef struct retransXretry {
    short retrans;
    short retry;
} retransXretry_t, *retransXretry_p;

#define RES_INITCHECK() if(!(_res.options&RES_INIT))res_init();

extern struct state _res;
extern char *p_cdname(), *p_rr(), *p_type(), *p_class();
extern struct nsmsg *res_scan(), *resolve(), *_resolve(); 
