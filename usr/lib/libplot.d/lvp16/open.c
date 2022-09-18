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
static char SccsId[]="@(#)open.c	4.1	ULTRIX	7/2/90";
#endif not lint

#include "lvp16.h"

#include <stdio.h>

int currentx = 0;
int currenty = 0;
double lowx = 0.0;
double lowy = 0.0;
double scalex = 1.0;
double scaley = 1.0;
int xmax = XMAX;
int ymax = YMAX;

openvt ()
{
	printf("\033.(:");		/* PLOTTER ON instruction */
	printf("\033.I128;;17:");	/* Set Handshake for O/P */
	printf("\033.N100;19:");	/* handshake: ^Q=17, ^S=19 */
	printf("IN;");			/* Initialize */
	printf("DF;");			/* Default */
	printf("SP 1;\n");		/* Select Pen */
}


openpl(){
	printf("\033.(:");		/* PLOTTER ON instruction */
	printf("\033.I128;;17:");	/* Set Handshake for O/P */
	printf("\033.N100;19:");	/* handshake: ^Q=17, ^S=19 */
	printf("IN;");			/* Initialize */
	printf("DF;");			/* Default */
	printf("SP 1;\n");		/* Select Pen */
}
