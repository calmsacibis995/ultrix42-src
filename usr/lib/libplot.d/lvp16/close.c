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
static char SccsId[]="@(#)close.c	4.1	ULTRIX	7/2/90";
#endif not lint

#include <stdio.h>

closevt(){
	printf("SP 0;");
	printf("DF;");
	putchar('\033');
	printf(".):");
	fflush(stdout);
}
closepl(){
	printf("SP 0;");
	printf("DF;");
	putchar('\033');
	printf(".):");
	fflush(stdout);
}

