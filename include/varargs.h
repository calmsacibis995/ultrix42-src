/*	@(#)varargs.h	4.3	(ULTRIX)	9/4/90		*/
#include <ansi_compat.h>
#ifdef __vax
/*	varargs.h	4.1	83/05/03	*/

#ifndef	_VA_LIST_
#define	_VA_LIST_
typedef char *va_list;
#endif	/* _VA_LIST_ */
# define va_dcl int va_alist;
# define va_start(list) list = (char *) &va_alist
# define va_end(list)
# define va_arg(list,mode) ((mode *)(list += sizeof(mode)))[-1]
#endif /* __vax */
#ifdef __mips
/* --------------------------------------------------- */
/* | Copyright (c) 1986 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                            | */
/* --------------------------------------------------- */
/* $Header: varargs.h,v 2010.3.1.4 89/11/29 22:41:37 bettina Exp $ */

#ifndef	_VARARGS_
#define	_VARARGS_	1


/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_VA_LIST_
#define	_VA_LIST_
typedef char *va_list;
#endif	/* _VA_LIST_ */
#define va_dcl int va_alist;
#define va_start(list) list = (char *) &va_alist
#define va_end(list)
#ifdef __u370
#define va_arg(list, mode) ((mode *)(list = \
	(char *) ((int)list + 2*sizeof(mode) - 1 & -sizeof(mode))))[-1]
#else
#ifdef __host_mips
#ifdef lint /* complains about constant in conditional context */
#define va_arg(list, mode) ((mode *)(list += sizeof(mode)))[-1]
#else /* !lint */
#ifndef MS_STAMP
#define ___INCLUDING___
#include <stamp.h>      /* get the version numbers */
#endif

#if MS_STAMP<2 || (MS_STAMP==2 && LS_STAMP<10)
#define va_arg(list, mode) ((mode *)(list = \
	(char *) (sizeof(mode) > 4 ? ((int)list + 2*8 - 1) & -8 \
				   : ((int)list + 2*4 - 1) & -4)))[-1]
#else
/* this works for structures also */
#define va_arg(list, mode) ((mode *)(list =\
 (char *) ((((int)list + (__builtin_alignof(mode)<=4?3:7)) &\
 (__builtin_alignof(mode)<=4?-4:-8))+sizeof(mode))))[-1]
/*  +++++++++++++++++++++++++++++++++++++++++++
    Because of parameter passing conventions in C:
    use mode=int for char, and short types
    use mode=double for float types
    use a pointer for array types
    +++++++++++++++++++++++++++++++++++++++++++ */

#endif

#ifdef ___INCLUDING___  /* did we include stamp.h? */
#undef MS_STAMP         /* undo the definitions to clean up cpp */
#undef LS_STAMP
#undef ___INCLUDING___
#endif

#endif /* lint */
#else
#define va_arg(list, mode) ((mode *)(list += sizeof(mode)))[-1]
#endif
#endif


#endif	/* _VARARGS_ */
#endif /* __mips */
