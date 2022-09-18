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
static char SccsId[]="@(#)linemod.c	4.1	ULTRIX	7/2/90";
#endif not lint


#include "lvp16.h"

linemod(s)
char *s;
{
	int c;
	printf("LT ");
	switch(s[0]){
	case 'l':	
		printf("3;");
		break;
	case 'd':	
		/*if 4th character is "d" then "dotdashed" else "dotted"*/
		if(s[3] != 'd')printf("1;");
		else printf("4;");
		break;
	case 's':
		/*if 6th character is  there, then it is shortdashed not
solid */
		if(s[5] != '\0')printf("2;");
		else printf(";");
	}
}
