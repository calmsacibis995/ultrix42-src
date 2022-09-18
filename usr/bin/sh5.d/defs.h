/* @(#)defs.h	4.1 (ULTRIX) 7/17/90 */

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
 * 005 - David Lindner Thu Dec 14 13:19:02 EST 1989
 *	 Added LOGNAME for POSIX compliance
 *
 * 004 - David Lindner Fri Nov  3 10:42:08 EST 1989
 *	 Redefined MAXTRAP to be 32.
 *
 * 003 - David Lindner 26-Jul-89
 *	 Fixed badfilename bug.
 *
 * 002 - Gary A. Gaudet, Wed Nov  9 10:24:49 EST 1988
 *	 MIPS portability and bug fixes
 *
 * 001 - Gary Gaudet for Andy Gadsby 09-mar-88
 *	 i18n version of csh
 *
 *
 *
 */

/*
 *	UNIX shell
 */

/*
 * DJL 005
 * maximum length of the logname
 */
#define LOGNAME_MAX 8

/* error exits from various parts of shell */
#define 	ERROR		1
#define 	SYNBAD		2
#define 	SIGFAIL 	2000
#define	 	SIGFLG		0200

/* command tree */
#define 	FPRS		0x0100
#define 	FINT		0x0200
#define 	FAMP		0x0400
#define 	FPIN		0x0800
#define 	FPOU		0x1000
#define 	FPCL		0x2000
#define 	FCMD		0x4000
#define 	COMMSK		0x00F0
#define		CNTMSK		0x000F

#define 	TCOM		0x0000
#define 	TPAR		0x0010
#define 	TFIL		0x0020
#define 	TLST		0x0030
#define 	TIF			0x0040
#define 	TWH			0x0050
#define 	TUN			0x0060
#define 	TSW			0x0070
#define 	TAND		0x0080
#define 	TORF		0x0090
#define 	TFORK		0x00A0
#define 	TFOR		0x00B0
#define		TFND		0x00C0

/* execute table */
#define 	SYSSET		1
#define 	SYSCD		2
#define 	SYSEXEC		3

#ifdef RES	/*	include login code	*/
#define 	SYSLOGIN	4
#else
#define 	SYSNEWGRP 	4
#endif

#define 	SYSTRAP		5
#define 	SYSEXIT		6
#define 	SYSSHFT 	7
#define 	SYSWAIT		8
#define 	SYSCONT 	9
#define 	SYSBREAK	10
#define 	SYSEVAL 	11
#define 	SYSDOT		12
#define 	SYSRDONLY 	13
#define 	SYSTIMES 	14
#define 	SYSXPORT	15
#define 	SYSNULL 	16
#define 	SYSREAD 	17
#define		SYSTST		18

#ifndef RES	/*	exclude umask code	*/
#define 	SYSUMASK 	20
#define 	SYSULIMIT 	21
#endif

#define 	SYSECHO		22
#define		SYSHASH		23
#define		SYSPWD		24
#define 	SYSRETURN	25
#define		SYSUNS		26
#define		SYSMEM		27
#define		SYSTYPE  	28

/* used for input and output of shell */
#define 	INIO 		19

/*io nodes*/
#define 	USERIO		10
#define 	IOUFD		15
#define 	IODOC		16
#define 	IOPUT		32
#define 	IOAPP		64
#define 	IOMOV		128
#define 	IORDW		256
#define		IOSTRIP		512
#define 	INPIPE		0
#define 	OTPIPE		1

/* arg list terminator */
#define 	ENDARGS		0

#include	"mac.h"
#include	"mode.h"
#include	"name.h"
#include	<signal.h>


/*	error catching */
extern int 		errno;

/* result type declarations */

#define 	alloc 		malloc

extern CHTYPE				*alloc();
extern CHTYPE				*make();
extern CHTYPE				*movstr();
extern CHTYPE				*movstrn();
extern struct trenod	*cmd();
extern struct trenod	*makefork();
extern struct namnod	*lookup();
extern struct namnod	*findnam();
extern struct dolnod	*useargs();
extern float			expr();
extern CHTYPE				*catpath();
extern CHTYPE				*getpath();
extern CHTYPE				*nextpath();
extern CHTYPE				**scan();
extern CHTYPE				*mactrim();
extern CHTYPE				*macro();
extern CHTYPE				*execs();
extern int				exname();
extern CHTYPE				*staknam();
extern int				printnam();
extern int				printro();
extern int				printexp();
extern CHTYPE				**setenv();
extern long				time();

#define 	attrib(n,f)		(n->namflg |= f)
#define 	round(a,b)		(((int)(((CHTYPE *)(a)+b)-1))&~((b)-1))
#define 	closepipe(x)	(close(x[INPIPE]), close(x[OTPIPE]))
#define 	eq(a,b)			(cf(a,b)==0)
#define 	max(a,b)		((a)>(b)?(a):(b))
#define 	assert(x)		;

/* temp files and io */
extern int				output;
extern int				ioset;
extern struct ionod		*iotemp;	/* files to be deleted sometime */
extern struct ionod		*fiotemp;	/* function files to be deleted sometime */
extern struct ionod		*iopend;	/* documents waiting to be read at NL */
extern struct fdsave	fdmap[];


/* substitution */
extern int				dolc;
extern CHTYPE				**dolv;
extern struct dolnod	*argfor;
extern struct argnod	*gchain;

/* stak stuff */
#include		"stak.h"

/* string constants */
extern CHTYPE				*atline;
extern CHTYPE				*readmsg;
extern CHTYPE				*colon;
extern CHTYPE				*minus;
extern CHTYPE				*nullstr;
extern CHTYPE				*sptbnl;
extern CHTYPE				*unexpected;
extern CHTYPE				*endoffile;
extern CHTYPE				*synmsg;

/* name tree and words */
extern struct sysnod	reserved[];
extern int				no_reserved;
extern struct sysnod	commands[];
extern int				no_commands;

extern int				wdval;
extern int				wdnum;
extern int				fndef;
extern int				nohash;
extern struct argnod	*wdarg;
extern int				wdset;
extern BOOL				reserv;

/* prompting */
extern CHTYPE				*stdprompt;
extern CHTYPE				*supprompt;
extern CHTYPE				*profile;
extern CHTYPE				*sysprofile;

/* built in names */
extern struct namnod	fngnod;
extern struct namnod	cdpnod;
extern struct namnod	ifsnod;
extern struct namnod	homenod;
extern struct namnod	mailnod;
extern struct namnod	pathnod;
extern struct namnod	ps1nod;
extern struct namnod	ps2nod;
extern struct namnod	mchknod;
extern struct namnod	acctnod;
extern struct namnod	mailpnod;

/* special names */
extern CHTYPE				flagadr[];
extern CHTYPE				*pcsadr;
extern CHTYPE				*pidadr;
extern CHTYPE				*cmdadr;
extern CHTYPE                           *calladr;       /* DJL 003 */

extern CHTYPE				*defpath;

/* names always present */
extern CHTYPE				*mailname;
extern CHTYPE				*homename;
extern CHTYPE				*pathname;
extern CHTYPE				*cdpname;
extern CHTYPE				*ifsname;
extern CHTYPE				*ps1name;
extern CHTYPE				*ps2name;
extern CHTYPE				*mchkname;
extern CHTYPE				*acctname;
extern CHTYPE				*mailpname;

/* transput */
extern CHTYPE				tmpout[];
extern CHTYPE				*tmpnam;
extern int				serial;

#define		TMPNAM 		7

extern struct fileblk	*standin;

#define 	input		(standin->fdes)
#define 	eof			(standin->feof)

extern int				peekc;
extern int				peekn;
extern CHTYPE				*comdiv;
extern CHTYPE				*devnull;

/* flags */
#define		noexec		01
#define		sysflg		01
#define		intflg		02
#define		prompt		04
#define		setflg		010
#define		errflg		020
#define		ttyflg		040
#define		forked		0100
#define		oneflg		0200
#define		rshflg		0400
#define		waiting		01000
#define		stdflg		02000
#define		STDFLG		's'
#define		execpr		04000
#define		readpr		010000
#define		keyflg		020000
#define		hashflg		040000
#define		nofngflg	0200000
#define		exportflg	0400000

extern long				flags;
extern int				rwait;	/* flags read waiting */

/* error exits from various parts of shell */
#include	<setjmp.h>
extern jmp_buf			subshell;
extern jmp_buf			errshell;

/* fault handling */
#include	"brkincr.h"

extern unsigned			brkincr;
#define 	MINTRAP		0
#define 	MAXTRAP		32		/* DJL 004 */

#define 	TRAPSET		2
#define 	SIGSET		4
#define 	SIGMOD		8
#define 	SIGCAUGHT	16

extern void				done();		/* DAG */
extern void				fault();	/* DAG */
extern BOOL				trapnote;
extern CHTYPE				*trapcom[];
extern BOOL				trapflg[];

/* name tree and words */
extern char				**environ;
extern CHTYPE				numbuf[];
extern CHTYPE				*export;
extern CHTYPE				*duperr;
extern CHTYPE				*readonly;

/* execflgs */
extern int				exitval;
extern int				retval;
extern BOOL				execbrk;
extern int				loopcnt;
extern int				breakcnt;
extern int				funcnt;

/* messages */
extern CHTYPE				*mailmsg;
extern CHTYPE				*coredump;
extern CHTYPE				*badopt;
extern CHTYPE				*badparam;
extern CHTYPE				*unset;
extern CHTYPE				*badsub;
extern CHTYPE				*nospace;
extern CHTYPE				*nostack;
extern CHTYPE				*notfound;
extern CHTYPE				*badtrap;
extern CHTYPE				*baddir;
extern CHTYPE				*badshift;
extern CHTYPE				*restricted;
extern CHTYPE				*execpmsg;
extern CHTYPE				*notid;
extern CHTYPE 			*badulimit;
extern CHTYPE				*wtfailed;
extern CHTYPE				*badcreate;
extern CHTYPE				*nofork;
extern CHTYPE				*noswap;
extern CHTYPE				*piperr;
extern CHTYPE				*badopen;
extern CHTYPE				*badnum;
extern CHTYPE				*arglist;
extern CHTYPE				*txtbsy;
extern CHTYPE				*toobig;
extern CHTYPE				*badexec;
extern CHTYPE				*badfile;
extern CHTYPE				*badreturn;
extern CHTYPE				*badexport;
extern CHTYPE				*badunset;
extern CHTYPE				*nohome;
extern CHTYPE				*badperm;

/*	'builtin' error messages	*/

extern CHTYPE				*btest;
extern CHTYPE				*badop;

/*	fork constant	*/

#define 	FORKLIM 	32

extern address			end[];

#include	"ctype.h"

extern int				wasintr;	/* used to tell if break or delete is hit
		   					 * while executing a wait
							 */
extern int				eflag;


/*
 * Find out if it is time to go away.
 * `trapnote' is set to SIGSET when fault is seen and
 * no trap has been set.
 */

#define		sigchk()	if (trapnote & SIGSET)	\
							exitsh(exitval ? exitval : SIGFAIL)

#define 	exitset()	retval = exitval
