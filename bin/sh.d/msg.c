#ifndef lint
static char sccsid[] = "@(#)msg.c	4.2 (ULTRIX) 8/13/90";
/* Original ID:  "@(#)msg.c	4.2 8/11/83" */
#endif

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
 *	UNIX shell
 *
 *	S. R. Bourne
 *	Bell Telephone Laboratories
 *
 */

/*
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 * 001	David Lindner Wed Apr 11 10:27:26 EDT 1990
 *	- Added declarations of nofork and noswap error messages for 
 *	  fork bug.
 *	- Added comment header.
 *
 */


#include	"defs.h"
#include	"sym.h"

MSG		version = "\nVERSION sys137	DATE 1978 Nov 6 14:29:22\n";

/* error messages */
MSG	badopt		= "bad option(s)";
MSG	mailmsg		= "you have mail\n";
MSG	nospace		= "no space";
MSG	synmsg		= "syntax error";

MSG	badnum		= "bad number";
MSG	badparam	= "parameter not set";
MSG	badsub		= "bad substitution";
MSG	badcreate	= "cannot create";
MSG	illegal		= "illegal io";
MSG	restricted	= "restricted";
MSG	piperr		= "cannot make pipe";
MSG	badopen		= "cannot open";
MSG	coredump	= " - core dumped";
MSG	arglist		= "arg list too long";
MSG	txtbsy		= "text busy";
MSG	toobig		= "too big";
MSG	badexec		= "cannot execute";
MSG	notfound	= "not found";
MSG	badfile		= "bad file number";
MSG	badshift	= "cannot shift";
MSG	baddir		= "bad directory";
MSG	badtrap		= "bad trap";
MSG	wtfailed	= "is read only";
MSG	notid		= "is not an identifier";
MSG	nofork		= "fork failed - too many processes";	/* DJL 001 */
MSG	noswap		= "cannot fork: no swap space";

/* built in names */
MSG	pathname	= "PATH";
MSG	homename	= "HOME";
MSG	mailname	= "MAIL";
MSG	fngname		= "FILEMATCH";
MSG	ifsname		= "IFS";
MSG	ps1name		= "PS1";
MSG	ps2name		= "PS2";

/* string constants */
MSG	nullstr		= "";
MSG	sptbnl		= " \t\n";
MSG	defpath		= ":/bin:/usr/bin";
MSG	colon		= ": ";
MSG	minus		= "-";
MSG	endoffile	= "end of file";
MSG	unexpected 	= " unexpected";
MSG	atline		= " at line ";
MSG	devnull		= "/dev/null";
MSG	execpmsg	= "+ ";
MSG	readmsg		= "> ";
MSG	stdprompt	= "$ ";
MSG	supprompt	= "# ";
MSG	profile		= ".profile";


/* tables */
SYSTAB reserved = {
		{"in",		INSYM},
		{"esac",	ESSYM},
		{"case",	CASYM},
		{"for",		FORSYM},
		{"done",	ODSYM},
		{"if",		IFSYM},
		{"while",	WHSYM},
		{"do",		DOSYM},
		{"then",	THSYM},
		{"else",	ELSYM},
		{"elif",	EFSYM},
		{"fi",		FISYM},
		{"until",	UNSYM},
		{ "{",		BRSYM},
		{ "}",		KTSYM},
		{0,	0},
};

STRING	sysmsg[] = {
		0,
		"Hangup",
		0,	/* Interrupt */
		"Quit",
		"Illegal instruction",
		"Trace/BPT trap",
		"IOT trap",
		"EMT trap",
		"Floating exception",
		"Killed",
		"Bus error",
		"Memory fault",
		"Bad system call",
		0,	/* Broken pipe */
		"Alarm call",
		"Terminated",
		"Urgent condition",
		"Stopped",
		"Stopped from terminal",
		"Continued",
		"Child terminated",
		"Stopped on terminal input",
		"Stopped on terminal output",
		"Asynchronous I/O",
		"Exceeded cpu time limit",
		"Exceeded file size limit",
		"Virtual time alarm",
		"Profiling time alarm",
		"Window changed",
		"Signal 29",
		"User defined signal 1",
		"User defined signal 2",
		"Signal 32",
};
INT		num_sysmsg = (sizeof sysmsg / sizeof sysmsg[0]);

MSG		export = "export";
MSG		readonly = "readonly";
SYSTAB	commands = {
		{"cd",		SYSCD},
		{"read",	SYSREAD},
/*
		{"[",		SYSTST},
*/
		{"set",		SYSSET},
		{":",		SYSNULL},
		{"trap",	SYSTRAP},
		{"login",	SYSLOGIN},
		{"wait",	SYSWAIT},
		{"eval",	SYSEVAL},
		{".",		SYSDOT},
		{readonly,	SYSRDONLY},
		{export,	SYSXPORT},
		{"chdir",	SYSCD},
		{"break",	SYSBREAK},
		{"continue",	SYSCONT},
		{"shift",	SYSSHFT},
		{"exit",	SYSEXIT},
		{"exec",	SYSEXEC},
		{"times",	SYSTIMES},
		{"umask",	SYSUMASK},
		{0,	0},
};
