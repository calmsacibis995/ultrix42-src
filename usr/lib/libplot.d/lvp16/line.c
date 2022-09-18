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
static char SccsId[]="@(#)line.c	4.1	ULTRIX	7/2/90";
#endif not lint


#include "lvp16.h"

line(x0,y0,x1,y1)
int x0,y0,x1,y1;
{
	if(xsc(x0)==currentx && ysc(y0)==currenty)
		cont(x1,y1);
	else if(xsc(x1)==currentx && ysc(y1)==currenty)
		cont(x0,y0);
	else{
		move(x0,y0);
		cont(x1,y1);
	}
}
