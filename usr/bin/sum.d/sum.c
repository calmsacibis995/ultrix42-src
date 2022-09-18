/*	sum.c --
 *		Sum bytes in file mod 2^16
 *									
 *			Copyright (c) 1985 by				
 *		Digital Equipment Corporation, Maynard, MA		
 *			All rights reserved.				
 *									
 *   This software is furnished under a license and may be used and	
 *   copied  only  in accordance with the terms of such license and	
 *   with the  inclusion  of  the  above  copyright  notice.   This	
 *   software  or  any  other copies thereof may not be provided or	
 *   otherwise made available to any other person.  No title to and	
 *   ownership of the software is hereby transferred.			
 *									
 *   This software is  derived  from  software  received  from  the	
 *   University    of   California,   Berkeley,   and   from   Bell	
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	
 *   restrictions  under  license  agreements  with  University  of	
 *   California and with AT&T.						
 *									
 *   The information in this software is subject to change  without	
 *   notice  and should not be construed as a commitment by Digital	
 *   Equipment Corporation.						
 *									
 *   Digital assumes no responsibility for the use  or  reliability	
 *   of its software on equipment which is not supplied by Digital.	
 *
 *	000	Ancient History
 *		Berkeley SCCS 4.1 October 1, 1980.
 *
 *	001	CCB
 *		Performance improvements.
*/
#ifndef lint
static	char	*sccsid = "@(#)sum.c	4.1 (ULTRIX) 7/17/90";
#endif lint

#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <errno.h>

extern int errno;
extern char *sys_errlist[];

char *prog;

main(argc,argv)
char **argv;
{
	register unsigned	sum;
	register long		size;
	register char		*t;
	int			fd;
	int			errflg = 0;
	register int		nread;
	int			printname;
	char			buf[BUFSIZ*8];

	prog = *argv;
	fd = -1;
	if(argc < 2)
	{
		fd = 0;
		printname = 0;
	}
	else
		printname = (--argc > 1);

	while(++argv,argc--)
	{
		if( fd )
		{
			if ((fd = open(*argv, O_RDONLY)) < 0)
			{
				fprintf(stderr, "%s: Can't open %s (%s)\n",
					prog, *argv, sys_errlist[errno]);
				errno = 0;
				errflg += 10;
				continue;
			}
		} 
		for( sum = 0, size = 0;
			(nread = read(fd,buf,sizeof(buf))) > 0; )
		{
			size += (unsigned) nread;
			for( t = buf; nread--; ++t )
			{
				if( sum&01 )
					sum = (sum>>1) + 0x8000;
				else
					sum >>= 1;
				sum += (u_char) *t;
				sum &= 0xFFFF;
			}
		}
		if( nread == -1 )	/* read error */
		{
			fprintf(stderr,"%s: read error on %s (%s)\n",
				prog, fd ? *argv : "-", sys_errlist[errno]);
			errno = 0;
			errflg++;
		}

		printf("%05u%6ld%s%s\n", sum, (size+BUFSIZ-1)/BUFSIZ,
			printname ? " " : "", printname ? *argv : "");

		close(fd);
	} 
	exit(errflg);
}

