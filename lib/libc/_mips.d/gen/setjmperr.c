#ifndef lint
static char *sccsid = "@(#)setjmperr.c	4.1	(ULTRIX)	7/3/90";
#endif not lint
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: setjmperr.c,v 1.1 87/02/16 11:17:37 dce Exp $ */

#define ERRMSG	"longjmp botch\n"

/*
 * This routine is called from longjmp() when an error occurs.
 * Programs that wish to exit gracefully from this error may
 * write their own versions.
 * If this routine returns, the program is aborted.
 */
longjmperror()
{

	write(2, ERRMSG, sizeof(ERRMSG));
}
