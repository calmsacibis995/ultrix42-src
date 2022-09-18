/*	@(#)limits.h	4.4	(ULTRIX)	2/28/91	*/
/************************************************************************
 *									*
 *			Copyright (c) 1985, 1987, 1988 by		*
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
 *			Modification History
 *
 *	David L Ballenger,  24-May-1984
 * 001	Increase size of SYS_NMLN to have more descriptive system name
 *	and a better chance at handling most node names.
 *
 * 002  Increase OPEN_MAX to 64 or NOFILE if defined
 *
 * 003	David L Ballenger, 30-Sep-1985
 *	Change ARG_MAX, PIPE_BUF, and PIPE_MAX to correspond to the 
 *	correct ULTRIX values.
 *
 * 004	David L Ballenger, 17-Oct-1985
 *	Remove #undef's at end of file so that #define's using those
 *	macros will work.
 *
 * 005	Mark A. Parenti, 3-Sep-1987
 *	Add defines for POSIX compliance.
 *	Change UID_MAX from 60000 to 32000. The uid field is a short
 *	throughout the kernel and as such can only handle up to 32767.
 *	The 60000 is a remnant from System V which uses an unsigned
 *	short.  This change eases POSIX compliance in regards to error
 *	checking in the setuid() system call.
 *
 * 006 Andy gadsby, 26-Jan-1988
 *	Added defines for internationalization.
 *
 * 007 Mark Parenti, 05-Aug-1988
 *	More defines from POSIX, ANSI-C, and X/OPEN
 *
 * 008	Jon Reeves, 01-Jun-1989
 *	Add MB_LEN_MAX from ANSI and POSIX.  Spacing adjustments to
 *	avoid warnings when included with other headers.
 *
 * 009	Martin Hills, 12-Jun-1989
 *	Added NL_NMAX for X/Open conformance.
 *
 * 010	Reeves, 15-Jun-1989
 *	Correct HUGE_VAL, DBL_DIG, DBL_MAX to reflect reality.
 *
 * 011	Reeves, 12-Jul-1989
 *	Add NZERO for X/Open; the description in XPG3's limits.h is
 *	misleading -- this value was determined with the help of nice(3).
 *
 * 012	prs,	24-Jul-1989
 *	Changed value of LINK_MAX from 1000 to max short -1.
 *
 * 013	David Lindner 31-Oct-1989
 *	Changed NL_TEXTMAX to POSIX soon to be constant LINE_MAX 2048.
 *
 * 014	reeves, 1989-Sep-21
 *	Commented #else/#endif tags for ANSI; cast UINT_MAX; protected
 *	dumb X/Open DBL_MIN/FLT_MIN tags.
 *
 * 015	reeves, 07-Dec-1989
 *	Namespace protection.
 *
 * 016	dlong, 05-Feb-1990
 *	Increase PASS_MAX.
 *
 * 017  DECwest ANSI 3.1.3.2 dls 1990 Mar 22
 *      Changed use of "POSIX" to "_POSIX_SOURCE"
 *	Added "f" suffixes to defined floats if "__STDC__" instead of (float) casts
 *	Similarly "u" for (unsigned).
 *
 * 018	DECwest ANSI 3.1.3.2 dls 8-jun-90
 *	Changed use of _POSIX_SOURCE to POSIX on CHILD_MAX (was done in
 *	change 017).
 *
 * 019	DECwest ANSI mt 1990 July 03
 *	Define INT_MIN as (-INT_MAX -1) so it has the type signed int.
 *
 ***********************************************************************/

/*
	limits.h -- /usr/group Standard environmental limits
				(BRL UNIX System V emulation version)

	last edit:	85/02/24	D A Gwyn

	SCCS ID:	@(#)limits.h	1.1

	WARNING!  The ANSI C Standard is likely to change some of these,
	especially the floating-point definitions, which were botched.
*/


#ifndef	_LIMITS_H_
#define	_LIMITS_H_			/* once-only latch */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#if defined(__pdp11)
#define	ARG_MAX		5120		/* max length of arguments to exec */
#else	/* 4.2BSD vax, gould */
#ifdef __vax
#define	ARG_MAX		10240		/* max length of arguments to exec */
#endif	/* __vax */
#ifdef __mips
#define	ARG_MAX		20480		/* max length of arguments to exec */
#endif	/* __mips */
#endif
#if defined(__gcos)
#define	CHAR_BIT	9		/* # of bits in a "char" */
#else
#define	CHAR_BIT	8		/* # of bits in a "char" */
#endif

#if defined(__pdp11) || defined(__vax) || defined(__mips) /* "plain" char is signed */
#define	CHAR_MIN	(-128)		/* min integer value of a "char" */
#define	CHAR_MAX	127		/* max integer value of a "char" */
#define UCHAR_MAX	255
#define SCHAR_MIN	CHAR_MIN	/* ANSI signed "char" min	*/
#define SCHAR_MAX	CHAR_MAX	/* ANSI signed "char" max	*/
#else					/* "plain" char is unsigned */
#define	CHAR_MAX	255		/* max integer value of a "char" */
#define	CHAR_MIN	0		/* min integer value of a "char" */
#endif

#ifndef	__POSIX				/* Not needed in __POSIX mode because */
					/* guaranteed to have minimum	*/
#define	CHILD_MAX	25		/* max # of processes per user id */
#endif

#if defined(_POSIX_SOURCE) /* Withdrawn in XPG3 */
#define	CLK_TCK 60 /* # of clock ticks per second */
	/* Also defined in time.h for POSIX */
#endif

#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) /* XPG4: TBW */
#if defined(__u3b) || defined(__u3b5) || defined(__mips)
#define	DBL_DIG 15 /* digits of precision of a "double" */
#define	DBL_MAX 1.7976931348623157e+308 /* max decimal value of a "double" */
#endif
#if defined(__u370) || defined(__gould)
#define	DBL_DIG		16		/* digits of precision of a "double" */
#define	DBL_MAX	0.7237005577332262e+76	/* max decimal value of a "double" */
#endif
#if defined(__gcos)
#define	DBL_DIG		18		/* digits of precision of a "double" */
#define	DBL_MAX	2.9387358770557187699e-39	/* max decimal value of a "double" */
#endif
#endif

#ifdef	_XOPEN_SOURCE	/* don't subject everyone to this */ /* XPG4: TBW */
#define	DBL_MIN		(-DBL_MAX)	/* min decimal value of a "double" */
#endif	/* _XOPEN_SOURCE */

#if !defined(_POSIX_SOURCE) /* Withdrawn in XPG3 */
#define	FCHR_MAX	1048576		/* max size of a file in bytes */
#endif

#if defined(__u3b) || defined(__u3b5) || defined(__mips)
# if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) /* XPG4: TBW */
#define	FLT_DIG	6 /* digits of precision of a "float" */
#ifdef __STDC__
#define	FLT_MAX	3.40282347e+38f /* max decimal value of a "float" */
#else
#define	FLT_MAX	((float)3.40282347e+38) /* max decimal value of a "float" */
#endif
# endif
# if !defined(_POSIX_SOURCE)
/* Error indicator from math library */
#define	HUGE_VAL	1.8e+308
#endif
#endif

#if defined(__pdp11) || defined(__vax)
# if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) /* XPG4: TBW */
#define	FLT_DIG 6 /* digits of precision of a "float" */
#ifdef __STDC__
#define	FLT_MAX	1.701411733192644299e+38f /* max decimal value of a "float" */
#else
#define	FLT_MAX	((float)1.701411733192644299e+38) /* max decimal value of a "float" */
#endif

# endif
#if	defined(__GFLOAT) || CC$gfloat
#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
#define	DBL_MAX 8.9884656743115790e+307 /* max decimal value of a "double" */
#define	DBL_DIG 15 /* digits of precision of a "double" */
#endif
# if !defined(_POSIX_SOURCE)
#define HUGE_VAL	8.9884656743115790e+307 
# endif
#else /* GFLOAT */
# if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
#define	DBL_MAX 1.701411834604692293e+38 /* max decimal value of a "double" */
#define	DBL_DIG 16 /* digits of precision of a "double" */
# endif
# if !defined(_POSIX_SOURCE)
#define HUGE_VAL	1.701411834604692293e+38 
# endif
#endif	/* __GFLOAT */
#endif /* __pdp11/__VAX */

#if defined(__u370) || defined(__gould)
#define	FLT_DIG		6		/* digits of precision of a "float" */
#define	FLT_MAX	0.7237005145e+76	/* max decimal value of a "float" */
#endif
#if defined(__gcos)
#define	FLT_DIG		8		/* digits of precision of a "float" */
#define	FLT_MAX	1.7014118219281863150e+38	/* max decimal value of a "float" */
#endif
#ifdef	_XOPEN_SOURCE	/* don't subject everyone to this */
#define	FLT_MIN		(-FLT_MAX)	/* min decimal value of a "float" */
#endif	/* _XOPEN_SOURCE */

#if !defined(_POSIX_SOURCE) /* Withdrawn in XPG3 */
#define	LOCK_MAX	0 /* for now */	/* max # of entries in system lock table */
#endif

/* 4.2BSD actually has no link limit; i.e. it is 65535 then a black hole results. */
#define	LINK_MAX	32766		/* max # of links to a single file */

#define	SHRT_MIN	(-32768)	/* min decimal value of a "short" */
#define	SHRT_MAX	32767		/* max decimal value of a "short" */
#ifdef __STDC__
#define	USHRT_MAX	65535u		/* min value of a "u_short" */
#else
#define	USHRT_MAX	((unsigned)65535)	/* min value of a "u_short" */
#endif
#define INT_MIN		(-INT_MAX -1)	/* min decimal value of an "int" */
#define INT_MAX		2147483647	/* max decimal value of an "int" */
#if __STDC__ == 1
#define	UINT_MAX	4294967295u	/* max value of an "u_int" */
#else
#define	UINT_MAX	((unsigned)4294967295)	/* max value of an "u_int" */
#endif
#define	LONG_MIN	INT_MIN		/* min decimal value of a "long" */
#define	LONG_MAX	INT_MAX		/* max decimal value of a "long" */
#define	ULONG_MAX	UINT_MAX	/* max decimal value of a "u_long" */

#if !defined(_POSIX_SOURCE) /* Withdrawn in XPG3 */
#define	USI_MAX		UINT_MAX	/* max decimal value of an "unsigned" */
#endif

#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
#define	WORD_BIT	32		/* # of bits in a "word" or "int" */
#define	LONG_BIT	32		/* # of bits in a "long" (X/OPEN)*/
#endif

/*
	Note: NAME_MAX is also used in <sys/dir.h>, which has its own
	local definition (under a different name) due to POSIX name scope 
	rules.
 */
#if defined(__pdp11)
#define	NAME_MAX	14		/* max # of characters in a file name */
#else	/* 4.2BSD vax, gould */
#define	NAME_MAX	255		/* max # of characters in a file name */
#endif
#define	NGROUPS_MAX	32		/* max # of groups */
#define	MAX_INPUT	256		/* max # of bytes in terminal input queue*/
#define	MAX_CANON	256		/* max # of bytes in term canon input line*/

#if !defined(_POSIX_SOURCE) /* Withdrawn in XPG3 */
#define	MAX_CHAR	256		/* max # of bytes in term canon input line (X/OPEN) */
#endif

#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)
#define	OPEN_MAX	64		/* max # of files a process can have open - OBSOLETE, sysconf() interface should be used */
#endif

#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) /* XPG4: TBW */
#define	PASS_MAX	16		/* max # of characters in a password */
#endif

#if defined(__pdp11)
#define	PATH_MAX	256		/* max # of characters in a path name */
#else	/* 4.2BSD vax, gould */
#define	PATH_MAX	1024		/* max # of characters in a path name */
#endif

#if !defined(_POSIX_SOURCE)
#define	PID_MAX		30000		/* max value for a process ID */
#endif

#if !defined(_POSIX_SOURCE) /* Withdrawn in XPG3 */
#define	SYSPID_MAX	2		/* max value for a system proc ID (X/OPEN) */
#endif

/* 4.2BSD appear to have no natural pipe size. */
#define	PIPE_BUF	4096		/* max # bytes atomic in write to pipe */

#if !defined(_POSIX_SOURCE)
#define	PIPE_MAX	4096		/* max # bytes written to a pipe in a write */
#endif

/* The following is pure invention. */
#if !defined(_POSIX_SOURCE) /* Withdrawn in XPG3 */
#define	PROC_MAX	100		/* max # of simultaneous processes */
#endif

#if !defined(_POSIX_SOURCE)
#if defined(__u3b) || defined(__u3b5)
#define	STD_BLK		1024		/* # bytes in a physical I/O block */
#endif
#if defined(__pdp11)
#define	STD_BLK		512		/* # bytes in a physical I/O block */
#endif
#if defined(__vax) || defined(__gould) || defined(__mips)	/* 4.2BSD; your mileage may vary */
#define	STD_BLK		8192		/* # bytes in a physical I/O block */
#endif
#if defined(__u370)
#define	STD_BLK		4096		/* # bytes in a physical I/O block */
#endif
/* SYS_NMLN is also defined in utsname.h for namespace reasons */
#define	SYS_NMLN	32		/* # of chars in uname-returned strings */
#endif

/*
   The following is pure invention.  Note: spacing must correspond to
   matching definition in stdio.h for TMP_MAX and param.h for NZERO.
 */
#if !defined(_POSIX_SOURCE)
#define	SYS_OPEN	200		/* max # of files open on system */
#endif
#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) /* XPG4: TMP_MAX TBW */
#define	TMP_MAX 17576 /* max # of calls to tmpnam(3S) before */
					/* recycling of names occurs */
#define	NZERO 20
					/* default nice value (as seen */
					/* by nice(3), not intrinsically) */
#endif /* !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) */

#ifndef _POSIX_SOURCE
#define	UID_MAX		32000		/* max value for a user or group ID */
#endif /* _POSIX_SOURCE */
/*
 * Internationalization constants
 */
#define	MB_LEN_MAX	1	/* max number of bytes in a multibyte	*/
				/* character, any locale: placeholder	*/

#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
#define NL_ARGMAX	9	/* max value of digits in calls to	*/
				/* nl_scanf(3S) and nl_printf(3S)	*/

#define NL_MSGMAX	32767	/* max message number			*/

#define NL_NMAX		2	/* max n-to-1 bytes in mapping chars	*/

#define NL_SETMAX	255	/* max set number			*/

#define NL_TEXTMAX	2048	/* max no. of bytes in a message string	*/
				/* set to POSIX LINE_MAX which does 	*/
				/* not yet exist			*/
#endif

#if !defined(_POSIX_SOURCE)
#define NL_LBLMAX	32767	/* max number of labels in catalogue	*/
#endif

#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
#define NL_LANGMAX	32 	/* max number of bytes in LANG name	*/
				/* 32 is meant to be sufficently large  */
				/* to allow sensible names without      */
				/* wasting memory in setlocale		*/
#endif

/*
 *	POSIX minimum values
 *
 *	These values are the MINIMUM allowable by POSIX. Actual values
 *	for Ultrix-32 are defined above.
 */
#define	_POSIX_ARG_MAX	 4096	/* Length of arguments for exec()	*/
#define	_POSIX_CHILD_MAX 6	/* Number of simultaneous procs per uid */
#define	_POSIX_LINK_MAX	 8	/* Number of file links 		*/
#define	_POSIX_MAX_CANON 255	/* Number of bytes in a terminal canon	*/
				/* input queue				*/
#define	_POSIX_MAX_INPUT 255	/* Number of bytes for which space is	*/
				/* guaranteed in terminal input queue	*/
#define	_POSIX_NAME_MAX	 14	/* Number of bytes in a filename	*/
#define	_POSIX_NGROUPS_MAX 0	/* Number of allowable supplementary 	*/
				/* group ID's				*/
#define	_POSIX_OPEN_MAX  16	/* Number of files open at one time	*/
				/* by a given process.			*/
#define	_POSIX_PATH_MAX	 255	/* Number of bytes in a pathname	*/
#define	_POSIX_PIPE_BUF	 512	/* Number of bytes that is guaranteed	*/
				/* to be written atomically when 	*/
				/* writing to a pipe.			*/
#endif	/* _LIMITS_H_ */
