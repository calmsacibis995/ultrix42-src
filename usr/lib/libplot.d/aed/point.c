
#ifndef lint
static char SccsId[] = " @(#)point.c	4.1	(ULTRIX)	7/2/90";
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
static char sccsid[] = "@(#)point.c	5.2 (Berkeley) 4/30/85";
#endif not lint


#include "aed.h"

/*---------------------------------------------------------
 *	This routine plots a single point.
 *
 *	Results:	None.
 *
 *	Side Effects:
 *	A single point is displayed on the screen.
 *	The point is made the current point.
 *---------------------------------------------------------
 */
point(x, y)
int x, y;
{
    setcolor("01");
    putc('Q', stdout);
    outxy20(x, y);
    fputs("O01", stdout);
    (void) fflush(stdout);
    curx = x;
    cury = y;
}
