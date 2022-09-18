#ifndef lint
static CHTYPE *sccsid = "@(#)defs.c	4.1 (Ultrix) 7/17/90";
#endif lint

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
/*
 *
 *   Modification History:
 *
 * 002 - David Lindner Wed Jul 26 10:19:31 EDT 1989
 *	 Fixed badfilename bug
 *
 * 001 - Gary Gaudet for Andy Gadsby 09-mar-88
 *	i18n version of csh
 *
 *
 *
 */

/*
 *	UNIX shell
 */

#include 		<setjmp.h>
#include		"mode.h"
#include		"name.h"
#include		<sys/param.h>

/* temp files and io */

int				output = 2;
int				ioset;
struct ionod	*iotemp;	/* files to be deleted sometime */
struct ionod	*fiotemp;	/* function files to be deleted sometime */
struct ionod	*iopend;	/* documents waiting to be read at NL */
struct fdsave	fdmap[NOFILE];

/* substitution */
int				dolc;
CHTYPE			**dolv;
struct dolnod	*argfor;
struct argnod	*gchain;


/* name tree and words */
int				wdval;
int				wdnum;
int				fndef;
int				nohash;
struct argnod	*wdarg;
int				wdset;
BOOL			reserv;

/* special names */
CHTYPE			*pcsadr;
CHTYPE			*pidadr;
CHTYPE			*cmdadr;
CHTYPE			*calladr;	/* DJL 002 */

/* transput */ 
CHTYPE 			*tmpnam;
int 			serial; 
int 			peekc;
int 			peekn;
CHTYPE 			*comdiv;

long			flags;
int				rwait;	/* flags read waiting */

/* error exits from various parts of shell */
jmp_buf			subshell;
jmp_buf			errshell;

/* fault handling */
BOOL			trapnote;

/* execflgs */
int				exitval;
int				retval;
BOOL			execbrk;
int				loopcnt;
int				breakcnt;
int 			funcnt;

int				wasintr;	/* used to tell if break or delete is hit
				   			   while executing a wait
							*/

int				eflag;

/* The following stuff is from stak.h	*/

CHTYPE 			*stakbas;
CHTYPE			*staktop;
CHTYPE			*stakbot;
CHTYPE			*stakbsy;
CHTYPE 			*brkend;
