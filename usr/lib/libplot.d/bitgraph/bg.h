
/*
 * @(#)bg.h	4.1	(ULTRIX)	7/2/90
 *
 * Modification History
 *
 * 	April-11-1989, Pradeep Chetal
 *	Added changes from 4.3Tahoe BSD for lots of new drivers
 */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)bg.h	5.1 (Berkeley) 4/30/85
 */

/*
 * Displays plot files on a bbn bitgraph terminal.
 */

#include <stdio.h>
#include <math.h>

#define ESC	033
#define PI	3.141592659

/* The graphics address range is 0..XMAX, 0..YMAX. */
#define XMAX	768
#define YMAX	1024
#define scaleX(xi)	((int) ((xi - lowx)*scale +0.5))
#define scaleY(yi)	((int) ((yi - lowy)*scale +0.5))

extern int currentx;
extern int currenty;
extern double lowx;
extern double lowy;
extern double scale;
