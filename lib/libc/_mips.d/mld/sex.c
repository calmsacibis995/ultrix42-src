/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: sex.c,v 2010.2.1.3 89/11/29 14:29:47 bettina Exp $ */
/*LINTLIBRARY*/
/*
 * gethostsex() determines the byte sex of the host.
 */
#include "sex.h"

int
gethostsex()
{
    long x;
    char *p;

	x = 1;
	p = (char *)&x;
	if(*p == '\001')
		return(LITTLEENDIAN);
	else
		return(BIGENDIAN);
}
