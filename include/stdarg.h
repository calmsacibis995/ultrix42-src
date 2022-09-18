/*	@(#)stdarg.h	4.3	(ULTRIX)	9/4/90	*/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: stdarg.h,v 2010.6.1.5 89/11/29 22:41:27 bettina Exp $ */
/*	@(#)stdarg.h	1.1	*/

/* 4.8 Variable arguments */

#include <ansi_compat.h>
#ifndef __STDARG_H
#define __STDARG_H
 
#ifndef _VA_LIST_
#define _VA_LIST_
typedef char *va_list;
#endif /* _VA_LIST_ */

#define va_end(list)

#ifdef __host_mips

	/* va_start makes list point past the parmN */
#define va_start(list, parmN) (list = ((va_list)&parmN + sizeof(parmN)))

        /* va_arg aligns list and points past data */
#define va_arg(list, mode) ((mode *)(list =\
 (va_list) ((((int)list + (__builtin_alignof(mode)<=4?3:7)) &\
 (__builtin_alignof(mode)<=4?-4:-8))+sizeof(mode))))[-1]

/*  +++++++++++++++++++++++++++++++++++++++++++
    Because of parameter passing conventions in C:
    use mode=int for char, and short types
    use mode=double for float types
    use a pointer for array types
    +++++++++++++++++++++++++++++++++++++++++++ */

#else /* vax */

#define va_start(list, parmN) list = (va_list)((int)&parmN + sizeof(parmN))
#define va_arg(list, mode) ((mode *)(list += sizeof(mode)))[-1]

#endif

#endif

