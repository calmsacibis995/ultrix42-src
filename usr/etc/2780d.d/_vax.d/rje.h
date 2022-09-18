/*  static char sccsid[]="@(#)rje.h	1.2		(ULTRIX)	4/2/86"; 	*/

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

#include <sys/dir.h>
#include <sys/stat.h>
#include <pwd.h>
#include "rje.local.h"

#define RJELOGIN "rje"		/* rje login name */
#define RJEUID	68            /* User-Id for "rje" */

#define XMTRD	1	/* xmit read file descriptor */
#define ERRFD	2	/* errors file descriptor */
#define XMTWR	3	/* xmit write file descriptor */
#define DSPRD	4	/* disp read file descriptor */
#define DSPWR	5	/* disp write file descriptor */
#define JBLOG	6	/* joblog file descriptor */

#define NAMESZ	8

extern int	DU;		/* daeomon user-id */
extern int	MX;		/* maximum number of blocks to copy */
extern int	MC;		/* maximum number of copies allowed */
extern char	*RM;		/* remote machine name */
extern char	*LO;		/* lock file name */
extern char	*ST;		/* status file name */
extern char	*SD;		/* spool directory */
extern char	*AF;		/* accounting file */
extern char	*LF;		/* log file for error messages */
extern char	*TR;		/* trailer string to be output when Q empties */
extern short	SC;		/* suppress multiple copies */
extern short	BR;		/* baud rate if lp is a tty */

extern char	line[BUFSIZ];
extern char	*name;		/* program name */
extern char	host[32];	/* host machine name */
extern char	*from;		/* client's machine name */
extern int	errno;

struct joblog {
	char j_file[NAMESZ];	/* Name of file to be sent */
	unsigned j_uid;		/* User ID of owner */
	int j_lvl;		/* Message level */
	long j_cnt;		/* Number of "cards" */
};

	/* joblog header info */

struct loghdr {
	int h_pgrp;		/* Process group Id */
};

struct dsplog {
	int d_type;		/* Type of record */
	union {
		struct {	/* record from xmit */
			char d_file[NAMESZ];	/* file sent */
			long d_cnt;		/* no. of cards sent */
			unsigned d_uid;		/* who sent the file */
			int d_lvl;		/* message level */
			int d_rdr;		/* reader sent from (0-6) */
		} x;
		struct {	/* record from recv */
			char d_file[NAMESZ];	/* file received */
			long d_cnt;		/* no. of records */
			int d_trunc;		/* file truncation flag */
		} r;
	} d_un;
};

struct sque {
	char sq_exfil[140];	/* Executable file */
	char sq_infil[48];	/* Input file */
	char sq_jobnm[9];	/* Remote job name */
	char sq_pgrmr[25];	/* Programmer name */
	char sq_jobno[9];	/* Remote job number */
	char sq_login[9];	/* Login name from usr= */
	char sq_homed[48];	/* Login directory */
	long sq_min;		/* Minimum file system space */
};
/*
 * Structure used for building a sorted list of control files.
 */
struct queue {
	time_t	q_time;			/* modification time */
	char	q_name[MAXNAMLEN+1];	/* control file name */
};

char	*pgetstr();
char	*malloc();
char	*index();
char	*rindex();
