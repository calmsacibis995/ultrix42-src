#ifdef CSHEDIT
 /*
  * sccsid = "@(#)sh.edit.h	4.1	(ULTRIX)	7/17/90";
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

/*
 * Modification history:	sh.edit.h
 *
 * This file contains header info for the command line edit functionality
 * added to the C shell.
 *
 * 002 - Gary A. Gaudet Wed Dec 20 16:44:45 EST 1989
 *	Increased BUFSIZ and TBUFSIZE to 2048
 *	Changed QUOTE and TRIM to 0200 and 0377
 *	removed unused function vputc()
 *
 * 22-Dec-87	afd
 *	Eliminated the define const "OVERBUF".  Its not needed since I
 *	made the edit code "8 bit clean".  We never set nor clear the 8th bit.
 *	
 */

#include <sys/param.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <sgtty.h>

extern	int errno;
extern char cmdline[]; /* Original command line from csh that we want to edit */
extern int emacs;      /* Flag for emacs edit mode */
extern int editevent;  /* event number to edit */

typedef	int	line;
typedef	short	bool;

/*
 * Definitions of editor parameters and limits
 */
#define	TABS		8	/* tab size */
#define	LBSIZE		2048 	/* Line Buffer size (1 screen-full) */
#define	ONMSZ		64	/* Option name size */
#define	VBSIZE		128	/* Partial line max size in visual */
/* 002 - GAG */
#define TCBUFSIZE	2048	/* Max entry size in termcap, see
					   also termlib and termcap */
#define	TUBECOLS	80 	/* default size of edit line. */

/*
 * Attention is the interrupt character (normally 0177 -- delete).
 * Quit is the quit signal (normally FS -- control-\) and quits open/visual.
 */
#define	ATTN	(-2)
#define	QUIT	('\\' & 037)

/* 002 - GAG */
#define	BUFSIZ	2048
#define	NULL	0
#define	EOF	-1

/*
 * Character constants and bits
 */
#define	QUOTE	0400
#define	TRIM	0377
#undef CTRL
#define	CTRL(c)	('c' & 037)
#define	NL	CTRL(j)
#define	CR	CTRL(m)
#define	DELETE	0177		/* See also ATTN, QUIT */
#define	ESCAPE	033

/*
 * Macros
 */
#define	CP(a, b)	((void)(strcpy(a, b)))
#define	copy(a,b,c)	Copy((char *) a, (char *) b, c)
#define	eq(a, b)	((a) && (b) && strcmp(a, b) == 0)
#define	outchar(c)	(*Outchar)(c)
#define	ungetchar(c)	peekc = c

#define	CATCH		vcatch = 1; if (setjmp(vreslab) == 0) {
#define	ONERR		} else { vcatch = 0;
#define	ENDCATCH	} vcatch = 0;

#define	INF		30000
#define	beep		obeep
#define	vputp(cp, cnt)	tputs(cp, cnt, vputch)
#define	vputc(c)	putch(c)

/*
 * Miscellaneous variables.
 */
char	genbuf[MAXBSIZE]; /* Working buffer when manipulating linebuf */
char	*globp;		/* (Untyped) input string to command mode */
char	linebuf[LBSIZE];/* The primary line buffer */
char	oldbuf[LBSIZE]; /* Saves prior version of linebuf */
char	obuf[BUFSIZ];	/* Buffer for tty output */
short	peekc;		/* Peek ahead character (cmd mode input) */
bool	vcatch;		/* Want to catch an error (open/visual) */
jmp_buf	vreslab;	/* For error throws to a visual catch */
jmp_buf	resetlab;	/* For error throws to top level (cmd mode) */
bool	done_edit;	/* Set true for NL or CR while inserting */
char	*markeol;	/* holds old end of linebuf for clreol w/out CE */
int	markline;	/* holds end of display for clreol of multiple lines */

extern	 char	exttytype[ONMSZ];		/* A long and pretty name */

/*
 * Capabilities from termcap
 *
 * Capabilities from termcap are of three kinds - string valued options,
 * numeric valued options, and boolean options.
 *
 * We only use a few termcap capibilities.  Since we are only doing
 * a one-line edit, we do only simple terminal operations.
 */
char	tspace[256];	/* Space for capability strings */
char	*aoftspace;	/* Address of tspace for relocation */

extern char *BC;	/*    Back cursor (declared in termcap) */
char	*CE;		/* P  Clear to end of line */
char	*KD;		/*    keypad down arrow */
char	*KH;		/*    keypad home */
char	*KE;		/*    Keypad don't xmit */
char	*KL;		/*    Keypad left arrow */
char	*KR;		/*    Keypad right arrow */
char	*KS;		/*    Keypad start xmitting */
char	*KU;		/*    keypad up arrow */
char	*ND;		/*    Non-destructive space (right cursor) */
extern char PC;		/*    Pad char (declared in termcap) */
char	*TA;		/* P  Tab (other than ^I or with padding) */
extern char *UP;	/*    Upline (declared in termcap) */
bool	AM;		/* Automatic margins */
bool	BS;		/* Backspace works */
bool	HC;		/* Hard copy terminal */
bool	HZ;		/* Hazeltine ~ braindamage */
bool	NC;		/* No Cr - \r snds \r\n then eats \n (dm2500) */
bool	XB;		/* Beehive (no escape key, simulate with f1) */

/*
 * From the tty modes...
 */
bool	NONL;		/* Terminal can't hack linefeeds doing a CR */
bool	UPPERCASE;	/* Ick! */

short   COLUMNS;	/* Set by termcap value, defaults to TUBECOLS */
short	outcol;		/* Where the cursor is */
short	destcol;	/* Where the cursor should be */
short	outline;	/* Line where the cursor is */
short	destline;	/* Line where the cursor should be */

typedef	int ttymode;		/* Mode to contain tty flags */
struct sgttyb hold_tty;		/* hold orig tty attributes */
struct tchars  hold_tchars;	/* INT, QUIT, XON, XOFF, EOF, BRK */

# define MAXNOMACS	128	/* max number of macros of each kind */
# define MAXCHARMACS	2048	/* max # of chars total in macros */
struct maps {
	char *cap;	/* pressing button that sends this.. */
	char *mapto;	/* .. maps to this string */
	char *descr;	/* legible description of key */
};
struct maps arrows[MAXNOMACS];	/* macro defs - 1st 5 are built in */
struct maps immacs[MAXNOMACS];	/* for while in insert mode */
int	maphopcnt;	/* check for infinite mapping loops */

/*
 * The current cursor position within the line is kept in cursor.  
 * During insertions we use the auxiliary array genbuf as scratch area.  
 * The cursor wcursor is used in operations within lines to mark 
 * the other end of the affected area, or the target for a motion.
 */
char	*cursor;
char	*wcursor;
char	*oldcursor;	/* old cursor position for undo */

char	DEL[VBSIZE];	/* Last deleted text */
char	INS[VBSIZE];	/* Last inserted text */
int	Xcnt;		/* External variable holding last cmd's count */
bool	Xhadcnt;	/* Last command had explicit count? */
short	dir;		/* Direction for search (+1 or -1) */
short	doomed;		/* Disply chars right of cursor to be killed */
bool	hadcnt;		/* (Almost) internal to vmain() */
char	lastcmd[5];	/* Chars in last command */
int	lastcnt;	/* Count for last command */
char	*lastcp;	/* Save current command here */
bool	lasthad;	/* True if last command had a count */
short	lastvgk;	/* Previous input key, if not from keyboard */
char	op;		/* Operation of current command */
short	Peekkey;	/* Peek ahead key */
char	vmacbuf[VBSIZE];   /* Text of visual macro, hence nonnestable */
char	*vmacp;		/* Like vglobp but for visual macros */
char	*vglobp;	/* Untyped input (e.g. repeat insert text) */
short	wdkind;		/* Liberal/conservative words? */
char	workcmd[5];	/* Temporary for lastcmd */
bool	insmode;	/* set true when in insert mode (for cursor position) */
char	*srchptr;	/* pointer into srchcmd */

/*
 * Function type definitions
 */
#define	NOSTR	(char *) 0

extern	int	(*Outchar)();
extern	int	(*Putchar)();
char	*getenv();
char	*longname();
char	*strcat();
char	*strcpy();
char	*strend();
char	*tgetstr();
char	*vgetline();
int	normchar();
int	normline();
int	putch();
int	vputch();
int	beep();
int	vchange();
int	vdelete();
int	vinschar();
int	vmove();
int	vputchar();
#endif
