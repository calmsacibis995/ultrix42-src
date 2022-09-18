/*
*	@(#)fio.h	1.2	(ULTRIX)	1/16/86
*/

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

/************************************************************************
*
*			Modification History
*
*	David Metsky		10-Jan-86
*
* 001	Replaced old version with BSD 4.3 version as part of upgrade.
*
*	Based on:	fio.h		5.1		6/7/85
*
*************************************************************************/

/*
 * f77 file i/o common definitions
 */

#include "fiodefs.h"

#define LOCAL		static
#define err(f,n,s)	{if(f) return(errno=n); else fatal(n,s);}
#define not_legal(u)	(u>=MXUNIT || u<0)
#define GET(x)		if((x=(*getn)())<0) return(x)
#define VAL(x)		(x!='\n'?x:' ')
#define PUT(x)		{if(n=(*putn)(x)) return(n);}
#define lcase(s)	((s >= 'A') && (s <= 'Z') ? s+('a'-'A') : s)

#define MAXINTLENGTH	32	/* to accomodate binary format */

long ftell();

extern int errno;
extern ioflag init;
extern flag reading,external,sequential,formatted;
extern int (*getn)(),(*putn)(),(*ungetn)();	/*for formatted io*/
extern FILE *cf;	/*current file structure*/
extern unit *curunit;	/*current unit structure */
extern int lunit;	/*current logical unit*/
extern char *lfname;	/*current filename*/
extern unit units[];	/*logical units table*/
extern int recpos;		/*position in current record*/
extern ftnint recnum;		/*current record number*/
extern int reclen;		/* current record length */
extern int (*doed)(), (*doned)();
extern int (*dorevert)(), (*donewrec)(), (*doend)(), (*dotab)();
extern ioflag cblank, cplus, tab, elist, signit, errflag, endflag;
extern char *fmtbuf, *icptr, *icend, *fmtptr;
extern int scale;
extern int cursor;
extern int radix;
extern struct ioiflg	ioiflg_;
