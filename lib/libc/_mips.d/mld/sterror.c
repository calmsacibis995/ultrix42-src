/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: sterror.c,v 2010.2.1.3 89/11/29 14:28:50 bettina Exp $ */
/* this is a fill in case people don't define it */

#include <stdio.h>

extern char *st_errname;

st_error (s, a, b, c, d) 

char *s; 

{
	fprintf( stderr, "%s: Error: ", st_errname );
	fprintf( stderr, s, a, b, c, d );
	fprintf( stderr, "\n" );
	exit (1);
}
