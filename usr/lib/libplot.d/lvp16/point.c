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
static char SccsId[]="@(#)point.c	4.1	ULTRIX	7/2/90";
#endif not lint


#include "lvp16.h"

point(xi,yi)
int xi,yi;
{
	if(xsc(xi)!=currentx || ysc(yi)!=currenty)
		move(xi,yi);
	printf("PD;");
}
