/*
 * @(#)sh.local.h	4.1  (ULTRIX)        7/17/90
 */
/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: sh.local.h,v 1.4 86/07/11 10:44:55 dce Exp $ */
/*
 * Modification history:	sh.edit.h
 *
 * 001 - Gary A. Gaudet Wed Dec 20 16:44:45 EST 1989
 *	Increased BUFSIZ to 2048
 */

/*
 * This file defines certain local parameters
 * A symbol should be defined in Makefile for local conditional
 * compilation, e.g. IIASA or ERNIE, to be tested here and elsewhere.
 */

/*
 * Fundamental definitions which may vary from system to system.
 *
 *	BUFSIZ		The i/o buffering size; also limits word size
 *	SHELLPATH	Where the shell will live; initalizes $shell
 *	MAILINTVL	How often to mailcheck; more often is more expensive
 *	OTHERSH		Shell for scripts which don't start with #
 */

/* 001 - GAG */
#define	BUFSIZ	2048		/* default buffer size */
#define	SHELLPATH	"/bin/csh"
#define	OTHERSH		"/bin/sh"
#define FORKSLEEP	10	/* delay loop on non-interactive fork failure */
#define	MAILINTVL	600	/* 10 minutes */

/*
 * The shell moves std in/out/diag and the old std input away from units
 * 0, 1, and 2 so that it is easy to set up these standards for invoked
 * commands.
 */
#ifdef DEBUG
#undef NOFILE
#define NOFILE 56
#endif
#define	FSHTTY	(NOFILE-5)	/* /dev/tty when manip pgrps */
#define	FSHIN	(NOFILE-4)	/* Preferred desc for shell input */
#define	FSHOUT	(NOFILE-3)	/* ... shell output */
#define	FSHDIAG	(NOFILE-2)	/* ... shell diagnostics */
#define	FOLDSTD	(NOFILE-1)	/* ... old std input */

#ifdef IIASA
#undef	OTHERSH
#endif

#ifdef vax
#define	copy(to, from, size)	bcopy(from, to, size)
#endif

#ifdef PROF
#define	exit(n)	done(n)
#endif
