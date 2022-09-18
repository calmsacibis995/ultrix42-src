
#ifndef lint
static char SccsId[] = " @(#)space.c	4.1	(ULTRIX)	7/2/90";
#endif not(lint)

/*
 * Modification History
 *
 * 	April-11-1989, Pradeep Chetal
 *	Added changes from 4.3Tahoe BSD for lots of new drivers
 */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)space.c	5.1 (Berkeley) 6/7/85";
#endif not lint

space_(x0,y0,x1,y1)
int *x0, *y0, *x1, *y1;
{
	space(*x0,*y0,*x1,*y1);
}
