/*	checksum.c
 *		checksum routines
 *
 *			Copyright (c) 1985, 1989 by
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.
 *								
 *	This software is furnished under a license and may be used and
 *	copied  only  in accordance with the terms of such license and
 *	with the  inclusion  of  the  above  copyright  notice.   This
 *	software  or  any  other copies thereof may not be provided or
 *	otherwise made available to any other person.  No title to and
 *	ownership of the software is hereby transferred.		
 *								
 *	This software is  derived  from  software  received  from  the
 *	University    of   California,   Berkeley,   and   from   Bell
 *	Laboratories.  Use, duplication, or disclosure is  subject  to
 *	restrictions  under  license  agreements  with  University  of
 *	California and with AT&T.					
 *								
 *	The information in this software is subject to change  without
 *	notice  and should not be construed as a commitment by Digital
 *	Equipment Corporation.					
 *								
 *	Digital assumes no responsibility for the use  or  reliability
 *	of its software on equipment which is not supplied by Digital.
 *
 *	000	08-mar-1989	ccb
 *		This comment added. Originally derived from sum(1) in BSD 4.2
 *		sources in the summer of 1985.
 *	001	16-jun-1989	ccb
 *		lint. cleaned up return types.
*/

#ifndef lint
static	char *sccsid = "@(#)getsum.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/types.h>
#include	<sys/file.h>
#include	<stdio.h>
#include	<errno.h>

#define	GSBUFSIZ	(1024*8)

/*LINTLIBRARY*/

extern int	errno;			/* errno(2) */
extern char	*sys_errlist[];		/* errno(2) */

extern char	*prog;

u_short getsum();

u_short CheckSum(path)
char *path;
{
	return(getsum(path));
}

u_short getsum(path)
char *path;
{
	char			buf[GSBUFSIZ];
	int			fd, nread;
	register unsigned	i, sum;
	register char		*t;

	if( (fd = open(path,O_RDONLY,0)) < 0 )
	{
		(void) fprintf(stderr,"%s: can't open %s (%s)\n", prog, path,
			sys_errlist[errno]);
		errno = -1;
		return(0);
	}

	/* checksum done here */
	for( sum = 0; (nread = read(fd,buf,GSBUFSIZ)) > 0; )
	{
		for( i=0, t = buf; i<nread; ++t, ++i )
		{
			if( sum & 01 )
				sum = (sum>>1) + 0x8000;
			else
				sum >>= 1;
			sum += (unsigned char) (*t);
			sum &= 0xFFFF;
		}
	}
	(void) close(fd);
	return( (u_short) sum);
}

