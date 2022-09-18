/*
 * 	@(#)ansi_compat.h	4.4	(ULTRIX)	10/8/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1990 by			        *
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 *   To avoid namespace pollution when using the ULTRIX header files under the
 * DEC ANSI compiler, all user-visible header files were modifed to reference
 * ANSI-style predefined macro name rather than their traditional names
 * (__ultrix vice ultrix).  Every file which accesses a predefined macro name
 * must include this file before any other files are included or the macros
 * are tested.
 *
 *   In strict ANSI mode, the appropriate ANSI-style macros are already
 * defined and the redefinitions in this file will not be seen.  When using
 * pcc, the traditional macro names are defined and this file will define
 * ANSI-style equivalents of the traditional names.  When using the DEC C
 * compiler, both the traditional and ANSI predefined macro names are
 * available so the definitions in this file are not made visible.
 *
 */


#if !defined(__STDC__) && !defined(__DECC) && !defined(__ANSI_COMPAT)

#define __ANSI_COMPAT

#ifdef ultrix
#define	__ultrix      ultrix
#endif

#ifdef unix
#define	__unix        unix
#endif

#ifdef bsd4_2
#define	__bsd4_2      bsd4_2
#endif

#ifdef vax
#define __vax 	      vax
#endif

#ifdef VAX
#define __VAX 	      VAX
#endif

#ifdef mips
#define	__mips        mips
#endif

#ifdef host_mips
#define	__host_mips   host_mips
#endif

#ifdef MIPSEL
#define	__MIPSEL      MIPSEL
#endif

#ifdef MIPSEB
#define	__MIPSEB      MIPSEB
#endif

#ifdef SYSTEM_FIVE
#define	__SYSTEM_FIVE SYSTEM_FIVE
#endif

#ifdef POSIX
#define	__POSIX       POSIX
#endif

#ifdef GFLOAT
#define __GFLOAT	GFLOAT
#endif

#ifdef LANGUAGE_C
#define	__LANGUAGE_C  LANGUAGE_C
#endif

#ifdef vaxc
#define __vaxc	 vaxc
#define __VAXC   VAXC
#define __vax11c vax11c
#define __VAX11C VAX11C
#endif

#ifdef MOXIE
#define __MOXIE   MOXIE
#endif

#ifdef ULTRIX022
#define __ULTRIX022 ULTRIX022
#endif

#endif
