#ifndef lint
static char *sccsid = "@(#)mtabinit.c	4.1      ULTRIX 	10/17/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * File:	mtabinit.c
 * Author:	Adrian Thoms
 * Description:
 *	This file prints out a C source file containing the pre-compiled
 *	version of the magic data from the magic file
 */

#include "magic_data.h"

init_printmtab()
{
	reg Entry *ep;

	printf("#include \"magic_data.h\"\n\n");
	printf("Entry_init magic_tab[] = {\n");
	for(ep = mtab; ep->e_off > -2L*(long)maxexecfn; ep++) {
		printf("{ %d,\t%DL,\t%DL,\t%d,\t%d,\t",
		       ep->e_level, ep->e_off, 
		       ep->e_retcode, ep->e_type, ep->e_opcode);
		if (ep->e_type == STR) {
			printf("(long)");
			init_printstr(ep->e_value.str, 50);
			printf(",\t");
		}
		else
		    printf("0%#lo,\t", ep->e_value.num);
		printf("\"%s\" },\n", ep->e_str);
	}
	printf("{ 0,\t%DL }\n", -2L*(long)maxexecfn);
	printf("};\n");
}

init_printstr(p, n)
unsigned char *p;
int n;
{

	register unsigned char *sp;
	register int c;

	putchar('"');
	for (sp = p, c = 0; *sp != '\0' && c++ < n; sp++)
		if (isprint(*sp)) printf("%c", *sp);
		else if (*sp == '\n') printf("\\n");
		else if (*sp == '\r') printf("\\r");
		else if (*sp == '\t') printf("\\t");
		else if (*sp == '\b') printf("\\b");
		else if (*sp == '\f') printf("\\f");
		else printf("\\%#o", *sp);
	putchar('"');
}
