/*	udetect.c -
 *		write records in inventory differing from system state.
 *
 *	udetect < invfile		; report sum, size, type, date diffs
 *	udetect -s < invfile	; (strict) report all diffs
 *	udetect -m < invfile	; (missing) report missing files
 *
 *			Copyright (c) 1989 by
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
 *	The information in this software is subject to change  without
 *	notice  and should not be construed as a commitment by Digital
 *	Equipment Corporation.					
 *								
 *	Digital assumes no responsibility for the use  or  reliability
 *	of its software on equipment which is not supplied by Digital.
 *
 *	HISTORY:
 *	000	2-feb-1989	Chas. Bennett
 *		New.
 *
 *	001	29-apr-1989	ccb
 *		lint.
 *	002	24-jul-1989	ccb
 *		More Lint.
 *		Include <sys/dir.h> for setld.h
*/
#ifndef lint
static	char *sccsid = "@(#)udetect.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<stdio.h>
#include	"setld.h"

extern void	exit();		/* exit(3) */

/*	program option switch flag values
*/
#define	STRICT	0x0001		/* print record on any discrepancy */
#define	MISSING	0x0002		/* print record for missing files */
#define	DEBUG	0x0004		/* enable debugging output */

char		*prog;		/* program name pointer */
unsigned	flags;		/* command flags */

main(argc,argv)
int argc;
char *argv[];
{
	int		opt;	/* option pointer for use with getopt */
	unsigned	mask;	/* mask of file differnces */
	InvRecT		*i;	/* working inventory record */
	InvT		*inp,	/* input inventory pointer */
			*outp;	/* output inventory pointer */

	prog = *argv;

	while( (opt = getopt( argc, argv, "ms" )) != EOF )
	{
		switch( opt )
		{
		case 'd':
			flags |= DEBUG;
			break;
		case 'm':
			flags |= MISSING;
			break;
		case 's':
			flags |= STRICT;
			break;
		case '?':
			(void) fprintf(stderr, "usage: %s -[ms]\n", prog);
			exit(1);
		}
	}

	inp = InvInit(stdin);
	outp = InvInit(stdout);

	/* read lines and check file attributes */
	while( (i = InvRead( inp )) )
	{
		/* get the mask of file differences */
		(void) FVerify(i);
		mask = i->i_vflags;

		if( flags & DEBUG )
			(void) fprintf( stderr, "mask, file: %04x, %s\n", mask,
				i->i_path );

		/* is the file there? */
		if( mask & I_PATH )
		{
			if( flags & MISSING )
				(void) InvWrite( outp, i );
			continue;
		}
		if( flags & STRICT && mask & (I_VALL) )
		{
			(void) InvWrite( outp, i );
			continue;
		}
		if( mask & I_VDATA )
			(void) InvWrite( outp, i );
	}
	exit(0);
}


