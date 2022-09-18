/*
 *		Copyright (C) 1986 by
 *		DIGITAL EQUIPMENT CORPORATION, Maynard, MA.
 *
 *  This software is furnished under a license and may be used and copied
 * only in accordance with the terms of such license and with the inclusion
 * of the above copyright notice.
 *
 * Creation date: February 1, 1986
 * Modifications: Added lvp16 dependent code.
 */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char SccsId[]="@(#)space.c	4.1	ULTRIX	7/2/90";
#endif not lint


#include "lvp16.h"

space(x0,y0,x1,y1)
int x0,y0,x1,y1;
{
	int flag = 1;
	/*
	 * If xmax OR ymax is set to 0 by program, do machine scaling.
	 */
	if( (xmax==0) || (ymax==0) ) {
		xmax = XMAX;
		ymax = YMAX;
		flag = 0;
	}
  
	if( x0 == x1 ) {	/* Avoid Div by 0, Do full screen scaling */
		x0 = 0;
		x1 = XMAX;
	}
  
	if( y0 == y1 ) {	 /* Avoid Div by 0, Do full screen scaling */
		y0 = 0;
		y1 = YMAX;
	}
	lowx =(double)x0;
	lowy = (double)y0;
	scalex = (double)xmax/(double)((double)x1-lowx);
	scaley = (double)ymax/(double)((double)y1-lowy);

	/*
	 * If machine scaling is requested, output scale command using current
	 * values of lowx, lowy, xmax and ymax
	 */
	if ( flag == 0 ) 
		printf("SC %d %d %d %d ;\n", (int)lowx, xmax, (int)lowy, ymax);
}
