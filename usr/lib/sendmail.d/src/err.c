#ifndef lint
static	char	*sccsid = "@(#)err.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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

# include "sendmail.h"
# include <errno.h>
# include <netdb.h>

/*
**  SYSERR -- Print error message.
**
**	Prints an error message via printf to the diagnostic
**	output.  If LOG is defined, it logs it also.
**
**	Parameters:
**		f -- the format string
**		a, b, c, d, e -- parameters
**
**	Returns:
**		none
**		Through TopFrame if QuickAbort is set.
**
**	Side Effects:
**		increments Errors.
**		sets ExitStat.
*/

# ifdef lint
int	sys_nerr;
char	*sys_errlist[];
# endif lint
char	MsgBuf[BUFSIZ*2];	/* text of most recent message */

/*VARARGS1*/
syserr(fmt, a, b, c, d, e)
	char *fmt;
{
	register char *p;
	int olderrno = errno;
	extern char Arpa_PSyserr[];
	extern char Arpa_TSyserr[];

	/* format and output the error message */
	if (olderrno == 0)
		p = Arpa_PSyserr;
	else
		p = Arpa_TSyserr;
	fmtmsg(MsgBuf, (char *) NULL, p, olderrno, fmt, a, b, c, d, e);
	puterrmsg(MsgBuf);

	/* determine exit status if not already set */
	if (ExitStat == EX_OK)
	{
		if (olderrno == 0)
			ExitStat = EX_SOFTWARE;
		else
			ExitStat = EX_OSERR;
	}

# ifdef LOG
	if (LogLevel > 0)
		syslog(LOG_CRIT, "%s: SYSERR: %s",
			CurEnv->e_id == NULL ? "NOQUEUE" : CurEnv->e_id,
			&MsgBuf[4]);
# endif LOG
	errno = 0;
	if (QuickAbort)
		longjmp(TopFrame, 2);
}
/*
**  USRERR -- Signal user error.
**
**	This is much like syserr except it is for user errors.
**
**	Parameters:
**		fmt, a, b, c, d -- printf strings
**
**	Returns:
**		none
**		Through TopFrame if QuickAbort is set.
**
**	Side Effects:
**		increments Errors.
*/

/*VARARGS1*/
usrerr(fmt, a, b, c, d, e)
	char *fmt;
{
	extern char SuprErrs;
	extern char Arpa_Usrerr[];
	extern int errno;

	if (SuprErrs)
		return;

	fmtmsg(MsgBuf, CurEnv->e_to, Arpa_Usrerr, errno, fmt, a, b, c, d, e);
	puterrmsg(MsgBuf);

	if (QuickAbort)
		longjmp(TopFrame, 1);
}
/*
**  MESSAGE -- print message (not necessarily an error)
**
**	Parameters:
**		num -- the default ARPANET error number (in ascii)
**		msg -- the message (printf fmt) -- if it begins
**			with a digit, this number overrides num.
**		a, b, c, d, e -- printf arguments
**
**	Returns:
**		none
**
**	Side Effects:
**		none.
*/

/*VARARGS2*/
message(num, msg, a, b, c, d, e)
	register char *num;
	register char *msg;
{
	errno = 0;
	fmtmsg(MsgBuf, CurEnv->e_to, num, 0, msg, a, b, c, d, e);
	putmsg(MsgBuf, FALSE);
}
/*
**  NMESSAGE -- print message (not necessarily an error)
**
**	Just like "message" except it never puts the to... tag on.
**
**	Parameters:
**		num -- the default ARPANET error number (in ascii)
**		msg -- the message (printf fmt) -- if it begins
**			with three digits, this number overrides num.
**		a, b, c, d, e -- printf arguments
**
**	Returns:
**		none
**
**	Side Effects:
**		none.
*/

/*VARARGS2*/
nmessage(num, msg, a, b, c, d, e)
	register char *num;
	register char *msg;
{
	errno = 0;
	fmtmsg(MsgBuf, (char *) NULL, num, 0, msg, a, b, c, d, e);
	putmsg(MsgBuf, FALSE);
}
/*
**  PUTMSG -- output error message to transcript and channel
**
**	Parameters:
**		msg -- message to output (in SMTP format).
**		holdmsg -- if TRUE, don't output a copy of the message to
**			our output channel.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Outputs msg to the transcript.
**		If appropriate, outputs it to the channel.
**		Deletes SMTP reply code number as appropriate.
*/

putmsg(msg, holdmsg)
	char *msg;
	bool holdmsg;
{
	/* output to transcript if serious */
	if (CurEnv->e_xfp != NULL && (msg[0] == '4' || msg[0] == '5'))
		fprintf(CurEnv->e_xfp, "%s\n", msg);

	/* output to channel if appropriate */
	if (!holdmsg && (Verbose || msg[0] != '0'))
	{
		(void) fflush(stdout);
		if (OpMode == MD_SMTP || OpMode == MD_ARPAFTP)
			fprintf(OutChannel, "%s\r\n", msg);
		else
			fprintf(OutChannel, "%s\n", &msg[4]);
		(void) fflush(OutChannel);
	}
}
/*
**  PUTERRMSG -- like putmsg, but does special processing for error messages
**
**	Parameters:
**		msg -- the message to output.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Sets the fatal error bit in the envelope as appropriate.
*/

puterrmsg(msg)
	char *msg;
{
	/* output the message as usual */
	putmsg(msg, HoldErrs);

	/* signal the error */
	Errors++;
	if (msg[0] == '5')
		CurEnv->e_flags |= EF_FATALERRS;
}
/*
**  FMTMSG -- format a message into buffer.
**
**	Parameters:
**		eb -- error buffer to get result.
**		to -- the recipient tag for this message.
**		num -- arpanet error number.
**		en -- the error number to display.
**		fmt -- format of string.
**		a, b, c, d, e -- arguments.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

/*VARARGS5*/
static
fmtmsg(eb, to, num, eno, fmt, a, b, c, d, e)
	register char *eb;
	char *to;
	char *num;
	int eno;
	char *fmt;
{
	char del;

	/* output the reply code */
	if (isdigit(fmt[0]) && isdigit(fmt[1]) && isdigit(fmt[2]))
	{
		num = fmt;
		fmt += 4;
	}
	if (num[3] == '-')
		del = '-';
	else
		del = ' ';
	(void) sprintf(eb, "%3.3s%c", num, del);
	eb += 4;

	/* output the file name and line number */
	if (FileName != NULL)
	{
		(void) sprintf(eb, "%s: line %d: ", FileName, LineNumber);
		eb += strlen(eb);
	}

	/* output the "to" person */
	if (to != NULL && to[0] != '\0')
	{
		(void) sprintf(eb, "%s... ", to);
		while (*eb != '\0')
			*eb++ &= 0177;
	}

	/* output the message */
	(void) sprintf(eb, fmt, a, b, c, d, e);
	while (*eb != '\0')
		*eb++ &= 0177;

	/* output the error code, if any */
	if (eno != 0)
	{
		extern char *errstring();

		(void) sprintf(eb, ": %s", errstring(eno));
		eb += strlen(eb);
	}
}
/*
**  ERRSTRING -- return string description of error code
**
**	Parameters:
**		errno -- the error number to translate
**
**	Returns:
**		A string description of errno.
**
**	Side Effects:
**		none.
*/

char *
errstring(errno)
	int errno;
{
	extern char *sys_errlist[];
	extern int sys_nerr;
	static char buf[100];
# ifdef SMTP
	extern char *SmtpPhase;
# endif SMTP

# ifdef DAEMON
# ifdef VMUNIX
	/*
	**  Handle special network error codes.
	**
	**	These are 4.2/4.3bsd specific; they should be in daemon.c.
	*/

	switch (errno)
	{
	  case ETIMEDOUT:
	  case ECONNRESET:
		(void) strcpy(buf, sys_errlist[errno]);
		if (SmtpPhase != NULL)
		{
			(void) strcat(buf, " during ");
			(void) strcat(buf, SmtpPhase);
		}
		if (CurHostName != NULL)
		{
			(void) strcat(buf, " with ");
			(void) strcat(buf, CurHostName);
		}
		return (buf);

	  case EHOSTDOWN:
		if (CurHostName == NULL)
			break;
		(void) sprintf(buf, "Host %s is down", CurHostName);
		return (buf);

	  case ECONNREFUSED:
		if (CurHostName == NULL)
			break;
		(void) sprintf(buf, "Connection refused by %s", CurHostName);
		return (buf);

	  case (TRY_AGAIN+MAX_ERRNO):
		(void) sprintf(buf, "Host Name Lookup Failure");
		return (buf);
	}
# endif VMUNIX
# endif DAEMON

	if (errno > 0 && errno < sys_nerr)
		return (sys_errlist[errno]);

	(void) sprintf(buf, "Error %d", errno);
	return (buf);
}
