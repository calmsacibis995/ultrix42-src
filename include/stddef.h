/*	@(#)stddef.h	4.2	(ULTRIX)	8/9/90	*/
/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: stddef.h,v 2010.5.1.5 89/11/29 22:41:35 bettina Exp $ */
/* 4.1.5 Common definitions */

#ifndef __STDDEF_H
#define __STDDEF_H


typedef int ptrdiff_t;         /* result type of subtracting two pointers */ 
#ifndef	_SIZE_T_
#define	_SIZE_T_
typedef unsigned int size_t;   /* result type of the sizeof operator */
#endif
#ifndef _WCHAR_T_
#define _WCHAR_T_
typedef unsigned int  wchar_t;         /* type of a wide character */
#endif

#ifndef NULL
#define NULL 0
#endif
			       /* byte offset to structure member */
#define offsetof(s_name,m_name) (size_t)&(((s_name*)0))->m_name

#endif

/* $Log:	stddef.h,v $
 * Revision 2010.5.1.5  89/11/29  22:41:35  bettina
 * 2.10 BETA2
 * 
 * Revision 2010.1  89/09/26  20:48:09  lai
 * *** empty log message ***
 * 
 * Revision 1.4  89/09/21  14:46:48  bettina
 * updating copyright
 * 
 * Revision 1.3  89/09/15  13:27:44  lai
 * removed check for STDC, since this is a ANSI file anyways
 * 
*/
