#ifndef lint
static CHTYPE *sccsid = "@(#)msg.c	4.1      7/17/90";
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
 * 002 - Gary A. Gaudet, Wed Nov  9 10:24:49 EST 1988
 *	 	MIPS portability and bug fixes
 *
 * 001 - Gary Gaudet for Andy Gadsby 09-mar-88
 *	i18n version of csh
 *
 *
 *
 */

/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */


#include	"defs.h"
#include	"sym.h"

/*
 * error messages
 */
CHTYPE	*badopt	= "bad option(s)";
CHTYPE	*mailmsg	= "you have mail\n";
CHTYPE	*nospace	= "no space";
CHTYPE	*nostack	= "no stack space";
CHTYPE	*synmsg	= "syntax error";

CHTYPE	*badnum	= "bad number";
CHTYPE	*badparam	= "parameter null or not set";
CHTYPE	*unset		= "parameter not set";
CHTYPE	*badsub	= "bad substitution";
CHTYPE	*badcreate	= "cannot create";
CHTYPE	*nofork	= "fork failed - too many processes";
CHTYPE	*noswap	= "cannot fork: no swap space";
CHTYPE	*restricted	= "restricted";
CHTYPE	*piperr	= "cannot make pipe";
CHTYPE	*badopen	= "cannot open";
CHTYPE	*coredump	= " - core dumped";
CHTYPE	*arglist	= "arg list too long";
CHTYPE	*txtbsy	= "text busy";
CHTYPE	*toobig	= "too big";
CHTYPE	*badexec	= "cannot execute";
CHTYPE	*notfound	= "not found";
CHTYPE	*badfile	= "bad file number";
CHTYPE	*badshift	= "cannot shift";
CHTYPE	*baddir	= "bad directory";
CHTYPE	*badtrap	= "bad trap";
CHTYPE	*wtfailed	= "is read only";
CHTYPE	*notid		= "is not an identifier";
CHTYPE 	*badulimit	= "bad ulimit";		/* DAG -- lower case */
CHTYPE	*badreturn = "cannot return when not in function";
CHTYPE	*badexport = "cannot export functions";
CHTYPE	*badunset 	= "cannot unset";
CHTYPE	*nohome	= "no home directory";
CHTYPE 	*badperm	= "execute permission denied";
CHTYPE	*longpwd	= "sh error: pwd too long";
/*
 * messages for 'builtin' functions
 */
CHTYPE	*btest		= "test";
CHTYPE	*badop		= "unknown operator ";
/*
 * built in names
 */
CHTYPE	*pathname	= "PATH";
CHTYPE	*cdpname	= "CDPATH";
CHTYPE	*homename	= "HOME";
CHTYPE	*mailname	= "MAIL";
CHTYPE	*ifsname	= "IFS";
CHTYPE	*ps1name	= "PS1";
CHTYPE	*ps2name	= "PS2";
CHTYPE	*mchkname	= "MAILCHECK";
CHTYPE	*acctname  	= "SHACCT";
CHTYPE	*mailpname	= "MAILPATH";

/*
 * string constants
 */
CHTYPE	*nullstr	= "";
CHTYPE	*sptbnl	= " \t\n";
CHTYPE	*defpath	= ":/bin:/usr/bin";
CHTYPE	*colon		= ": ";
CHTYPE	*minus		= "-";
CHTYPE	*endoffile	= "end of file";
CHTYPE	*unexpected 	= " unexpected";
CHTYPE	*atline	= " at line ";
CHTYPE	*devnull	= "/dev/null";
CHTYPE	*execpmsg	= "+ ";
CHTYPE	*readmsg	= "> ";
CHTYPE	*stdprompt	= "$ ";
CHTYPE	*supprompt	= "# ";
CHTYPE	*profile	= ".profile";
CHTYPE	*sysprofile	= "/etc/profile";

/*
 * tables
 */

struct sysnod reserved[] =
{
	{ "case",	CASYM	},
	{ "do",		DOSYM	},
	{ "done",	ODSYM	},
	{ "elif",	EFSYM	},
	{ "else",	ELSYM	},
	{ "esac",	ESSYM	},
	{ "fi",		FISYM	},
	{ "for",	FORSYM	},
	{ "if",		IFSYM	},
	{ "in",		INSYM	},
	{ "then",	THSYM	},
	{ "until",	UNSYM	},
	{ "while",	WHSYM	},
	{ "{",		BRSYM	},
	{ "}",		KTSYM	}
};

int no_reserved = 15;

CHTYPE	*sysmsg[] =
{
	0,
	"Hangup",
	0,	/* Interrupt */
	"Quit",
	"Illegal instruction",
	"Trace/BPT trap",
	"abort",
	"EMT trap",
	"Floating exception",
	"Killed",
	"Bus error",
	"Memory fault",
	"Bad system call",
	0,	/* Broken pipe */
	"Alarm call",
	"Terminated",
	"Signal 16",
	"Signal 17",
	"Child death",
	"Power Fail"
};

CHTYPE	*export = "export";
CHTYPE	*duperr = "cannot dup";
CHTYPE	*readonly = "readonly";


struct sysnod commands[] =
{
	{ ".",		SYSDOT	},
	{ ":",		SYSNULL	},

#ifndef RES
	{ "[",		SYSTST },
#endif

	{ "break",	SYSBREAK },
	{ "cd",		SYSCD	},
	{ "continue",	SYSCONT	},
	{ "echo",	SYSECHO },
	{ "eval",	SYSEVAL	},
	{ "exec",	SYSEXEC	},
	{ "exit",	SYSEXIT	},
	{ "export",	SYSXPORT },
	{ "hash",	SYSHASH	},

#ifdef RES
	{ "login",	SYSLOGIN },
	{ "newgrp",	SYSLOGIN },
#else
	{ "newgrp",	SYSNEWGRP },
#endif

	{ "pwd",	SYSPWD },
	{ "read",	SYSREAD	},
	{ "readonly",	SYSRDONLY },
	{ "return",	SYSRETURN },
	{ "set",	SYSSET	},
	{ "shift",	SYSSHFT	},
	{ "test",	SYSTST },
	{ "times",	SYSTIMES },
	{ "trap",	SYSTRAP	},
	{ "type",	SYSTYPE },


#ifndef RES		
	{ "ulimit",	SYSULIMIT },
	{ "umask",	SYSUMASK },
#endif

	{ "unset", 	SYSUNS },
	{ "wait",	SYSWAIT	}
};

int no_commands = sizeof commands / sizeof(struct sysnod);	/* DAG -- improved */
