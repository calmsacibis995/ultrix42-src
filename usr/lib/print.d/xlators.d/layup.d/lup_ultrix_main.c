#ifdef ultrix
#ifdef lint
static char *sccsid = "@(#)lup_ultrix_main.c	4.1	(ULTRIX)	7/2/90";
#endif lint
#endif ultrix

/***	
 ***	lup_ultrix_main.c -- Ultrix calling harness for layup translator
 ***/

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/* SCCS history beginning
 ***	
 ***	D. Maxwell 22/7/88
 ***	modified from lup_xtest.c 
 ***
 ***	N.Batchelder, 2/9/87.
 ***	heavily modified from xltest.c of the ANSI translator.
 ***	
 ***/

# include "lup_def.h"
# include <sys/file.h>

FILE	*fnptr;
char	*infile, *outfile;

int	fin = 0, fout = 1;
int	i, status, z;

# define IBUFSIZE	1024
# define OBUFSIZE	1024

/*----------*/
get_xlbuf (length, buf, user_arg_g)
short int	*length;
int		*buf;
int		user_arg_g;
{
	static unsigned char	ibuf [IBUFSIZE];
	int		status;

	status = read (user_arg_g, ibuf, IBUFSIZE);
	if (status <= 0) {
	    *length = 0;
	} else {
	    *length = status;
	}

	*buf = (int)ibuf;
	return SS$_NORMAL;
}

/*----------*/
put_xlbuf (length, buf, user_arg_p)
short int	*length;
int		*buf;
int		user_arg_p;
{
	static unsigned char	putbuf [OBUFSIZE];

	if (*length) {
	    write (user_arg_p, *buf, *length);
	}

	*buf = (int)putbuf;
	*length = OBUFSIZE;
	return SS$_NORMAL;
}

/*----------*/
main (argc, argv)
int     argc;
char    *argv [];
{
	/* Do the actual translation */
	if (trn$layupdef_ps (get_xlbuf, fin, put_xlbuf, fout, NULL) !=
	    SS$_NORMAL)
	    exit(2);
	exit(0);
}
