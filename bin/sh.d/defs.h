/* @(#)defs.h	4.2 (ULTRIX) 8/13/90 */
/* Original id: defs.h	4.2	83/06/10	*/

/*
 *	UNIX shell
 */

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 ---------
 Modification History
 ~~~~~~~~~~~~~~~~~~~~
 07	David Lindner Tue Jun  5 16:24:38 EDT 1990
	- Removed declaration of interrupt function, because
	  we are now using sigvec() with the SV_INTERRUPT bit set.
	- Redefined INIO and OTIO to take higher file descriptors.
	- Removed definition of INTbuf.
	- Added signal.h, limits.h

 06	David Lindner Wed Apr 11 10:22:21 EDT 1990
	- Definition of FORKLIM for fork bug.
	- Declaration of noswap and nofork error messages.

 05	David Lindner Mon Jan 22 14:13:29 EST 1990
	- Externally declare environ variable

 04	David Lindner Mon Jul 24 14:39:16 EDT 1989
	- Declaration of calladr for badfilename bug.

 03	David Lindner Wed Jul 19 13:01:52 EDT 1989
	- Definition of boolflg.

 02	Dave Lindner Fri Jun  9 14:23:42 EDT 1989
	- Declaration of interrupt function.

 01	26-Jun-84, Greg Tarsa
	- Added Comments.
*/

/* error exits from various parts of shell */
#define ERROR	1
#define SYNBAD	2
#define SIGFAIL 3
#define SIGFLG	0200

/* command tree flags */
#define FPRS	020	/* print pid of child */
#define FINT	040	/* make immune to QUIT and INTR signals */
#define FAMP	0100	/* execute in the background */
#define FPIN	0400	/* input is a pipe */
#define FPOU	01000	/* output is a pipe */
#define FPCL	02000	/* this pipe end gets closed */
#define FCMD	04000	/* unused */

#define COMMSK	017	/* mask to obtain above values */

/* command tree types */
#define TCOM	0	/* cmdtxt < inp > outp */
#define TPAR	1	/* (cmdtxt) < inp > outp */
#define TFIL	2	/* cmdtxt | cmdtxt */
#define TLST	3	/* cmdtxt ; cmdtxt */
#define TIF	4	/* "if" stmt */
#define TWH	5	/* "while" stmt */
#define TUN	6	/* "until" stmt */
#define TSW	7	/* "case" statement */
#define TAND	8	/* cmdtxt && cmdtxt */
#define TORF	9	/* cmdtxt || cmdtxt */
#define TFORK	10	/* cmdtxt & */
#define TFOR	11	/* "for" stmt */

/* execute table */
#define SYSSET	1	/* set		*/
#define SYSCD	2	/* cd		*/
#define SYSEXEC	3	/* exec		*/
#define SYSLOGIN 4	/* login	*/
#define SYSTRAP	5	/* trap		*/
#define SYSEXIT	6	/* exit		*/
#define SYSSHFT 7	/* shift	*/
#define SYSWAIT	8	/* wait		*/
#define SYSCONT 9	/* continue	*/
#define SYSBREAK 10	/* break	*/
#define SYSEVAL 11	/* eval		*/
#define SYSDOT	12	/* .		*/
#define SYSRDONLY 13	/* readonly	*/
#define SYSTIMES 14	/* times	*/
#define SYSXPORT 15	/* export	*/
#define SYSNULL 16	/* :		*/
#define SYSREAD 17	/* read		*/
#define SYSTST	18	/* test		*/
#define	SYSUMASK 19	/* umask	*/

#include <limits.h>	/* DJL 07 */
/* used for input and output of shell */
#define INIO (OPEN_MAX-2)	/* FD to save stdin on */
#define OTIO (OPEN_MAX-1)	/* FD to save stdout on */

/*io node flags */
#define USERIO	10	/* no FD's >= this value allowed in #>'s */
#define IOUFD	017	/* mask used to obtain the user's FD */
#define IODOC	020	/* input is from a "here" document ( <<-EOF) */
#define IOPUT	040	/* output is to a file (> file) */
#define IOAPP	0100	/* append to output file (>> file) */
#define IOMOV	0200	/* descriptor is to be copied ( #>&# ) */
#define IORDW	0400	/* I/O to same file (<>) [not implemented fully] */

/* Used to access ends of I/O pipes */
#define INPIPE	0
#define OTPIPE	1

/* arg list terminator */
#define ENDARGS	0

#include	"mac.h"
#include	"mode.h"
#include	"name.h"
#include	<signal.h>		/* DJL 07 */


/* result type declarations */
#define alloc malloc
ADDRESS		alloc();
VOID		addblok();
STRING		make();
STRING		movstr();
TREPTR		cmd();
TREPTR		makefork();
NAMPTR		lookup();
VOID		setname();
VOID		setargs();
DOLPTR		useargs();
REAL		expr();
STRING		catpath();
STRING		getpath();
STRING		*scan();
STRING		mactrim();
STRING		macro();
STRING		execs();
VOID		await();
VOID		post();
STRING		copyto();
VOID		exname();
STRING		staknam();
VOID		printnam();
VOID		printflg();
VOID		prs();
VOID		prc();
VOID		setupenv();
STRING		*setenv();

/* Useful definitions */
#define attrib(n,f)	(n->namflg |= f)
#define round(a,b)	(((int)((ADR(a)+b)-1))&~((b)-1))
#define closepipe(x)	(close(x[INPIPE]), close(x[OTPIPE]))
#define eq(a,b)		(cf(a,b)==0)
#define max(a,b)	((a)>(b)?(a):(b))
#define assert(x)	;

/* temp files and io */
UFD		output;
INT		ioset;
IOPTR		iotemp;		/* files to be deleted sometime */
IOPTR		iopend;		/* documents waiting to be read at NL */

/* substitution */
INT		dolc;
STRING		*dolv;
DOLPTR		argfor;
ARGPTR		gchain;

/* stack */
#define		BLK(x)	((BLKPTR)(x))
#define		BYT(x)	((BYTPTR)(x))
#define		STK(x)	((STKPTR)(x))
#define		ADR(x)	((char*)(x))

/* stak stuff */
#include	"stak.h"

/* string constants */
MSG		atline;
MSG		readmsg;
MSG		colon;
MSG		minus;
MSG		nullstr;
MSG		sptbnl;
MSG		unexpected;
MSG		endoffile;
MSG		synmsg;

/* name tree and words */
SYSTAB		reserved;
INT		wdval;
INT		wdnum;
ARGPTR		wdarg;
INT		wdset;
BOOL		reserv;

/* prompting */
MSG		stdprompt;
MSG		supprompt;
MSG		profile;

/* built in names */
NAMNOD		fngnod;
NAMNOD		ifsnod;
NAMNOD		homenod;
NAMNOD		mailnod;
NAMNOD		pathnod;
NAMNOD		ps1nod;
NAMNOD		ps2nod;

/* special names */
MSG		flagadr;
STRING		cmdadr;
STRING		calladr;	/* DJL 04 */
STRING		exitadr;
STRING		dolladr;
STRING		pcsadr;
STRING		pidadr;

MSG		defpath;

/* names always present */
MSG		mailname;
MSG		homename;
MSG		pathname;
MSG		fngname;
MSG		ifsname;
MSG		ps1name;
MSG		ps2name;

/* transput */
CHAR		tmpout[];
STRING		tmpnam;
INT		serial;
#define		TMPNAM 7
FILE		standin;
#define input	(standin->fdes)
#define eof	(standin->feof)
INT		peekc;
STRING		comdiv;
MSG		devnull;

/*
    Flags. Comments indicate current associations
*/
#define		noexec	(1<<0)	/* -n */
#define		intflg	(1<<1)	/* -i */
#define		prompt	(1<<2)	/* ?? */
#define		setflg	(1<<3)	/* -u */
#define		errflg	(1<<4)	/* -e */
#define		ttyflg	(1<<5)	/* shows input as tty (set) or file (clear) */
#define		forked	(1<<6)	/* ?? */
#define		oneflg	(1<<7)	/* -t */
#define		rshflg	(1<<8)	/* -r */
#define		waiting	(1<<9)	/* ?? */
#define		stdflg	(1<<10)	/* -s */
#define		execpr	(1<<11)	/* -x */
#define		readpr	(1<<12)	/* -v */
#define		keyflg	(1<<13)	/* -k */
#define		batchflg	(1<<14)	/* ?? */
#define 	boolflg	(1<<15) /* DJL 03 */

INT		flags;

/* error exits from various parts of shell */
#include	<setjmp.h>
jmp_buf		subshell;
jmp_buf		errshell;

/* fault handling */
#include	"brkincr.h"
POS		brkincr;

#define MINTRAP	0
#define MAXTRAP	32

#define INTR	2
#define QUIT	3
#define MEMF	11
#define ALARM	14
#define KILL	15
#define TRAPSET	2
#define SIGSET	4
#define SIGMOD	8

VOID		fault();
BOOL		trapnote;
STRING		trapcom[];
BOOL		trapflg[];
BOOL		trapjmp[];

/* name tree and words */
extern STRING		*environ;	/* DJL 05 */
CHAR		numbuf[];
MSG		export;
MSG		readonly;

/* execflgs */
INT		exitval;
BOOL		execbrk;
INT		loopcnt;
INT		breakcnt;

/* messages */
MSG		mailmsg;
MSG		coredump;
MSG		badopt;
MSG		badparam;
MSG		badsub;
MSG		nospace;
MSG		notfound;
MSG		badtrap;
MSG		baddir;
MSG		badshift;
MSG		illegal;
MSG		restricted;
MSG		execpmsg;
MSG		notid;
MSG		wtfailed;
MSG		badcreate;
MSG		piperr;
MSG		badopen;
MSG		badnum;
MSG		arglist;
MSG		txtbsy;
MSG		toobig;
MSG		badexec;
MSG		notfound;
MSG		badfile;
MSG		nofork;		/* DJL 06 */
MSG		noswap;

extern address	end[];

/*
 * DJL 06
 * Fork constant
 */
#define FORKLIM 32

#include	"ctype.h"

