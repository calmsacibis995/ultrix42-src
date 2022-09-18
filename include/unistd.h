/*
 *		@(#)unistd.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1987, 1989 by		*
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
 *	Jon Reeves, 07-Dec-1989
 * 009	Namespace protection.
 *
 * 008	Jon Reeves, 15-Sep-1989
 *	Fix NULL for consistency
 *
 * 007	Jon Reeves, 18-Jul-1989
 *	Add a few defines and many function definitions for X/Open.
 *
 * 006	Mark A. Parenti, 25-Aug-1988
 *	Remove numerous defines that have been deleted from the
 *	final POSIX standard.
 *
 * 005	Mark A. Parenti, 05-Aug-1988
 *	Add definition of NULL as per POSIX
 *
 * 004	Mark A. Parenti, 11-July-1988
 *	Remove POSIX_PATHNAME_NULL. Add POSIX_VDISABLE.  Both POSIX 12.3 
 *	changes.
 *
 * 003	Mark A. Parenti, 05-Feb-1988
 *	Latch R_OK to prevent warning message if <sys/file.h> is
 *	also included.
 *
 * 002	Mark A. Parenti, 30-Nov-1987
 *	Add definitions for POSIX.
 *
 * 001	David L Ballenger, 17-Oct-1985
 *	Change defintion of IN_PATH so that it is not BRL specific.
 *
 *	Based on:  unistd.h	1.1 (BRL UNIX System V emulation version)
 *		   85/02/24	D A Gwyn
 *
 ************************************************************************/

#ifndef	_UNISTD_H_
#define	_UNISTD_H_			/* once-only latch */

/*
 * Symbolic constants for the "access" call. Latch this as these are also
 * defined in <sys/file.h> for compatibility reasons.
 */
#ifndef R_OK
#define	R_OK	4	/* Test for "Read Permission */
#define	W_OK	2	/* Test for "Write Permission */
#define	X_OK	1	/* Test for "Execute" (Search) Permission */
#define	F_OK	0	/* Test for existence of file */
#endif /* R_OK */

/* 1003.1a notwithstanding, this is required; see 1003.1 2.8.1. */
#ifndef NULL
#define	NULL	0
#endif /*	NULL */

#if !defined(_POSIX_SOURCE) /* Withdrawn in XPG3 */
/* Symbolic constants for the "lockf" function: */
#define	F_ULOCK	0	/* Unlock a previously locked region */
#define	F_LOCK	1	/* Lock a region for exclusive use */
#define	F_TLOCK	2	/* Test and lock a region for exclusive use */
#define	F_TEST	3	/* Test region for other processes' locks */

/* Path names: */
#define	GF_PATH	"/etc/group"	/* Path name of the "group" file */
#define	PF_PATH	"/etc/passwd"	/* Path name of the "passwd" file */
#define	IN_PATH	"/usr/include"	/* Path name for <...> directory */
#endif /* !defined(_POSIX_SOURCE) */

/* Symbolic constants for the "lseek" function: */
/* Set file pointer to offset */
#define	SEEK_SET 0
/* Set file pointer to its current value plus offset */
#define	SEEK_CUR 1
/* Set file pointer to the size of the file plus offset */
#define	SEEK_END 2

/* POSIX optionals   */
#define _POSIX_JOB_CONTROL	1 /* Job Control Present		*/
#define _POSIX_SAVED_IDS	1 /* Support saved-set-ids feature	*/
#define _POSIX_VERSION		198808L /* POSIX version		*/
#define _POSIX_CHOWN_RESTRICTED	1 /* chown() restricted to super user	*/
#define _POSIX_NO_TRUNC		1 /* Pathname longer than NAME_MAX err	*/
#define _POSIX_VDISABLE		0 /* termio(s) special character disable */

/* POSIX sysconf() defines */
#define	_SC_ARG_MAX	1
#define	_SC_CHILD_MAX	2
#define	_SC_CLK_TCK	3
#define	_SC_NGROUPS_MAX	4
#define	_SC_OPEN_MAX	5
#define	_SC_JOB_CONTROL	6
#define	_SC_SAVED_IDS	7
#define	_SC_VERSION	8
#define	_SC_XOPEN_VERSION	9
#define	_SC_PASS_MAX	10

/* POSIX pathconf() defines */
#define	_PC_LINK_MAX	1
#define	_PC_MAX_CANON	2
#define	_PC_MAX_INPUT	3
#define	_PC_NAME_MAX	4
#define	_PC_PATH_MAX	5
#define	_PC_PIPE_BUF	6
#define	_PC_CHOWN_RESTRICTED	7
#define	_PC_NO_TRUNC	8
#define	_PC_VDISABLE	9

/* X/Open specific */
#define	_XOPEN_VERSION	3	/* X/Open Portability Guide version	*/

/* Stream numbers (why here and not stdio?  ah well, that's X/Open for ya) */
#define	STDIN_FILENO	0	/* standard in	*/
#define	STDOUT_FILENO	1	/* standard out	*/
#define	STDERR_FILENO	2	/* standard error	*/

#ifndef KERNEL
/*
 *	Function declarations
 */

extern int
	access(),
	chdir(),
	chown(),
	close(),
	dup(),
	dup2(),
	execl(),
	execle(),
	execlp(),
	execv(),
	execve(),
	execvp(),
	getgroups(),
	isatty(),
	link(),
	pause(),
	pipe(),
	read(),
	rename(),
	rmdir(),
	setgid(),
	setpgid(),
	setuid(),
	tcsetpgrp(),
	unlink(),
	write();

extern unsigned int
	alarm(),
	sleep();

extern long
	fpathconf(),
	pathconf(),
	sysconf();

extern char
	*ctermid(),
	*cuserid(),
	*getcwd(),
	*getlogin(),
	*ttyname();

extern void
	_exit();

/* Kludge-around for POSIX typedef braindamage */

extern short /* gid_t */
	getegid(),
	getgid();

extern int /* off_t */
	lseek();

extern int /* pid_t */
	fork(),
	getpgrp(),
	getpid(),
	getppid(),
	setsid(),
	tcgetpgrp();

extern short /* uid_t */
	geteuid(),
	getuid();

#endif	/* KERNEL */
#endif /*	_UNISTD_H_ */
