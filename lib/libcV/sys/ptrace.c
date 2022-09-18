#ifndef lint
static	char	*sccsid = "@(#)ptrace.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *			Modification History				*
 *									*
 *	David L Ballenger, 3-Apr-1985					*
 * 0001	The result variable was not being initailized with the return	*
 *	value from the _ptrace() call.					*
 *									*
 ************************************************************************/


/*
	ptrace -- system call emulation for 4.2BSD

	last edit:	01-Jul-1983	D A Gwyn
*/

#include	<errno.h>
#include	<signal.h>

extern int	_ptrace();
extern int	errno;

int
ptrace( request, pid, addr, data )
	register int	request;	/* request code */
	int		pid;		/* child process ID */
	int		addr;		/* address of data */
	int		data;		/* data to be stored */
	{
	register int	result; 	/* _ptrace() value */
	register int	sdata;		/* save data for request 7 */

	switch ( request )
		{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 8:
		break;

	case 7:
	case 9:
		sdata = data;		/* save for return value */
		if ( sdata >= 0 && sdata <= NSIG )
			break;
		/* bad signal #; fall through into error return */

	default:			/* bad request # */
		errno = EIO;
		return -1;
		}

	if ( (result = _ptrace( request, pid, (int *)addr, data )) < 0 )
		switch ( errno )
			{
		case EINVAL:		/* PID doesn't exist */
			errno = ESRCH;
			return -1;

		case EFAULT:		/* address out of bounds */
			errno = EIO;
			/* fall through into error return */

		default:		/* errno already reasonable */
			return -1;
			}
	else if ( request == 7 || request == 9 )
		return sdata;
	else
		return result;
	}
