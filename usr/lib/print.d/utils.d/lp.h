/*	@(#)lp.h	4.1      ULTRIX 7/2/90 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * lp.h -- Global definitions for the line printer system.
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  02/06/88 -- thoms
 * date and time created 88/06/02 17:46:22 by thoms
 * 
 * ***************************************************************
 * 
 * 1.2  02/06/88 -- thoms
 * Simplified code to allow any -D<datatype>
 * 
 * 
 * ***************************************************************
 *
 * 1.3  15/07/88 -- thoms
 * Added copyright notice and modification history
 *
 * ***************************************************************
 *
 * 1.4  18/07/88 -- thoms
 * Changed naming as agreed at code review
 *
 * ***************************************************************
 *
 * 1.5  28/07/88 -- thoms
 * Extended printer_type_e enumeration to cope with v3
 *
 * ****************************************************************
 *
 * 1.6  16/10/88 -- thoms
 * Removed printer type LPS_v2
 *
 * ****************************************************************
 *
 * 1.7   2/08/89 -- Giles Atkinson
 * Add new enum value for ULTRIX v4.0
 *
 * SCCS history end
 */


#include <stdio.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sgtty.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <varargs.h>
#include "assert.h"
#include "lp.local.h"
#include "filter.h"
#include "escapes.h"
#include "connection.h"
#include "dcl.h"
#include "argstrings.h"

extern int	DU;		/* daemon user-id */
extern int	MX;		/* maximum number of blocks to copy */
extern int	MC;		/* maximum number of copies allowed */
extern char	*LP;		/* line printer device name */
extern char	*RM;		/* remote machine name */
extern char	*RP;		/* remote printer name */
extern char	*LO;		/* lock file name */
extern char	*ST;		/* status file name */
extern char	*SD;		/* spool directory */
extern char	*AF;		/* accounting file */
extern char	*LF;		/* log file for error messages */
extern char	*OF;		/* name of output filter (created once) */
extern char	*IF;		/* name of input filter (created per job) */
extern char	*PP;		/* name of print filter replacement */
extern char	*RF;		/* name of fortran text filter (per job) */
extern char	*TF;		/* name of troff(1) filter (per job) */
extern char	*NF;		/* name of ditroff(1) filter (per job) */
extern char	*DF;		/* name of tex filter (per job) */
extern char	*GF;		/* name of graph(1G) filter (per job) */
extern char	*VF;		/* name of raster filter (per job) */
extern char	*CF;		/* name of cifplot filter (per job) */
extern char	*XF;		/* name of pass-thru (per job) */
extern char	*FF;		/* form feed string */
extern char	*TR;		/* trailer string to be output when Q empty */
extern char	*TS;		/* terminal server node name */
extern char	*OP;		/* object port on terminal server */
extern char	*OS;		/* object service on terminal server */
extern short	SC;		/* suppress multiple copies */
extern short	SF;		/* suppress FF on each print job */
extern short	SH;		/* suppress header page */
extern short	SB;		/* short banner instead of normal header */
extern short	RW;		/* open LP for reading and writing */
extern short	PW;		/* page width */
extern short	PX;		/* page width in pixels */
extern short	PY;		/* page length in pixels */
extern short	PL;		/* page length */
extern short	BR;		/* baud rate if lp is a tty */
extern int	FC;		/* flags to clear if lp is a tty */
extern int	FS;		/* flags to set if lp is a tty */
extern int	XC;		/* flags to clear for local mode */
extern int	XS;		/* flags to set for local mode */
extern short	RS;		/* restricted to those with local accounts */
extern short	FO;

/* Extra printcap entries for psv1.0 */
extern int	DB;		/* # Debug level for this printcap entry */
extern char	*UV;		/* = Ultrix version for extra features */
extern char	*PS;		/* : its a PostScript printer */
extern char	*DL;		/* PostScript DCL archive file */
extern char	*CT;		/* = Printer connection type */

/* Extra PostScript specific printcap entries for psv1.0 */
extern char	*pc_datatype;	/* Da */
extern char	*pc_input_tray;	/* It */
extern char	*pc_output_tray;/* Ot */
extern char	*pc_orientation;/* Or */
extern char	*pc_pagesize;	/* Ps */
extern char	*pc_sheetsize;	/* Ss */
extern char	*pc_upper_pglim;/* Ul */
extern char	*pc_number_up;	/* Nu */
extern char	*pc_message;	/* Ml */
extern char	*pc_layup_file;	/* Lu */
extern char	*pc_sides;	/* Si */

extern char	*pc_sheetstd;	/* Sd: fallback sheetsize if none specified */

extern char	*pc_layup;	/* Lf */
extern char	*pc_xlator;	/* Xf */


/* added for DQS */
extern char *DQ;  /* flag for DQS DQS notification */

/* possible string values for multi-choice string capabilities */

extern char *UV_choices[];
extern char *CT_choices[];
extern char *PS_choices[];

enum job_status_e {
	js_ok, js_retry, js_failed, js_too_many_retries, js_restricted,
};

/* The following definition should match the variable UV_choices
 * in pcap_choices.c
 */
enum ultrix_version_code {
	ULTRIX_base, ULTRIX_psv1_0, ULTRIX_v4_0, ULTRIX_upb,
};

/*
 * print_type_e -- enumeration to select job building functions
 */
enum printer_type_e {
	pt_non_PS, pt_LN03R, pt_LPS, pt_upb,
};

extern char	line[BUFSIZ];
extern char	pbuf[];		/* buffer for printcap entry */
extern char	*bp;		/* pointer into ebuf for pgetent() */
extern char	*bp_lim;	/* upper limit for bp */
extern char	*name;		/* program name */
extern char	*printer;	/* printer name */
extern char	host[32];	/* host machine name */
extern char	*from;		/* client's machine name */
extern int	errno;

/*
 * Structure used for building a sorted list of control files.
 */
struct queue {
	time_t	q_time;			/* modification time */
	char	q_name[MAXNAMLEN+1];	/* control file name */
};

char	*pgetstr();
char	*malloc();
char	*getenv();
char	*index();
char	*rindex();

/*
 * dofork(int action) - argument values and constants
 */
#define DORETURN	0	/* return after DOFORK_NRETRIES */
#define DOABORT		1	/* exit(1) after DOFORK_NRETRIES */
#define DOFORK_NRETRIES	20	/* retry fork this many times */

/*
 * Size of printcap string buffer
 */
#define PRINTCAP_BUFSIZ	(BUFSIZ*2)
