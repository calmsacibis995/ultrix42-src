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

#ifndef lint
static char SccsId[]="@(#)color.c	4.1	ULTRIX	7/2/90";
#endif not lint

#include "lvp16.h"

int color(ci)
	int ci;
{
	int c2;

	if(ci==0) {
		printf("SP 0;\n");
		return(0);
	}
	/* A negative value gets a default pen of #1 */
	else if (ci<=0) {
		printf("SP 1\n");
		return(1);
	}
	else {
	/*
 	 * The lvp16 only has six pens, therefore do modulo arithmetic for pen
 	 * selection.  Return the number actually printed out to tell the program
 	 * that another pen was selected rather than the "right" one.
	 */
	c2=((ci-1)%6)+1;
	printf("SP %d ;\n",c2);
	return(c2);
	}
}
