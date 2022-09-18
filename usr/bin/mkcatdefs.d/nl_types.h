/* @(#)nl_types.h	4.1	(ULTRIX)	12/6/90	*/

/************************************************************************
 *									*
 *         Copyright (c) Digital Equipment Corporation, 1990		*
 *									*
 *   All Rights Reserved.  Unpublished rights  reserved  under  the	*
 *   copyright laws of the United States.				*
 *									*
 *   The software contained on this media  is  proprietary  to  and	*
 *   embodies  the  confidential  technology  of  Digital Equipment	*
 *   Corporation.  Possession, use, duplication or dissemination of	*
 *   the  software and media is authorized only pursuant to a valid	*
 *   written license from Digital Equipment Corporation.		*
 *									*
 *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  by	*
 *   the U.S. Government is subject to restrictions as set forth in	*
 *   Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  or  in  FAR	*
 *   52.227-19, as applicable.						*
 *									*
 ************************************************************************/

/*
 * @OSF_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log:	nl_types.h,v $
 * Revision 1.3  90/04/27  23:07:05  devrcs
 * 	Updated to latest AIX code.
 * 	[90/04/16  13:48:49  tom]
 * 
 * Revision 1.2  90/03/13  21:23:13  mbrown
 * 	AIX merge first cut - new file.
 * 	[90/02/12  18:25:34  tom]
 * 
 */
/* @(#)nl_types.h       1.19  com/inc,3.1,9013 3/4/90 19:29:35 */
/*
 * COMPONENT_NAME: INC
 *
 * FUNCTIONS: nl_types.h
 *
 * ORIGINS: 27
 *
 * This module contains IBM CONFIDENTIAL code. -- (IBM
 * Confidential Restricted when combined with the aggregated
 * modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 1988, 1989
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 */

#ifndef _NL_TYPES_H_
#define _NL_TYPES_H_

#include "standards.h"

#ifdef _XOPEN_SOURCE
typedef int nl_item;
typedef struct catalog_descriptor *nl_catd;

#define NL_SETD 	1  				

#ifdef _NO_PROTO
extern nl_catd catopen();
extern char  *catgets();
extern int catclose();
#else
extern nl_catd catopen(char *, int );
extern char  *catgets(nl_catd , int , int , char *);
extern int catclose(nl_catd );
#endif /* _NO_PROTO */
#endif /* _XOPEN_SOURCE */

#ifdef _OSF_SOURCE
typedef struct catalog_descriptor CATD;

#ifdef	_NO_PROTO
nl_catd NLcatopen();
char *	NLgetamsg();
#else
nl_catd NLcatopen(char *, int);
extern char *NLgetamsg(char*, int, int, char*);
#endif	/* _NO_PROTO */


#ifdef _CAT_MACRO
#ifdef _NO_PROTO
char *fcatgets();
char *NLfcatgets();
char *fcatgetmsg();
#else
extern char   *fcatgets(nl_catd,int,int,char*);
extern char *NLfcatgets(nl_catd,int,int,char*);
extern char *fcatgetmsg(nl_catd,int,int,char*,int);
#endif	/* _NOPROTO */
#else
#ifdef _NO_PROTO
char *catgets();
char *NLcatgets();
char *catgetmsg();
#else
extern char   *catgets(nl_catd, int, int, char *);
extern char *NLcatgets(nl_catd, int, int, char *);
extern char *catgetmsg(nl_catd, int, int, char*, int);
#endif
#endif

#ifndef _MESG_H
#include "mesg.h"
#endif
#endif /* _OSF_SOURCE */

#endif /* _NL_TYPES_H_ */
