/*
*	@(#)drivedefs.h	1.3	(ULTRIX)	1/15/86
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
*	David Metsky		14-Jan-86
*
* 001	Replaced old version with BSD 4.3 version as part of upgrade.
*
*	Based on:	drivedefs.h
*
*************************************************************************/
/*
 * drivedefs.h
 *
 * Definitions for Fortran 77 Compiler driver
 * For the VAX, running on the VAX
 *
 * UCSD Chemistry modification history:
 *
 * $Log:	drivedefs.h,v $
 * Revision 1.4  85/02/12  19:25:05  donn
 * Added 'CATNAME' to define the name of the concatenation command.
 * 
 * Revision 1.3  85/01/14  06:42:01  donn
 * Changed to use c2 as the peephole optimizer.
 * 
 * Revision 1.2  84/04/11  19:02:16  donn
 * Added Dave Wasley's fix to load the Unix library (libU77.a) first.
 * 
 */

#if HERE!=VAX || TARGET!=VAX || FAMILY!=PCC
	Wrong Definitions File!
#endif

#define PASS1NAME	"/usr/lib/f77pass1"
#define PASS2NAME	"/lib/f1"
#define PASS2OPT	"/lib/c2"
#define ASMNAME		"/bin/as"
#define LDNAME		"/bin/ld"
#define	CATNAME		"/bin/cat"
#define FOOTNAME	"/lib/crt0.o"
#define PROFFOOT	"/lib/mcrt0.o"
#define	GPRFFOOT	"/usr/lib/gcrt0.o"
#define TEMPPREF	"fort"

static char *liblist [ ] =
		{ "-lU77", "-lF77", "-lI77", "-lm", "-lc", NULL };
static char *p_liblist [ ] =
		{ "-lU77_p", "-lF77_p", "-lI77_p", "-lm_p", "-lc_p", NULL };
