/*#@(#)mkdate.c	1.6	Ultrix	5/2/86*/

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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

/************************************************************************
 *									*
 *			Modification History				*
 *									*
 *	003 - Update copyright.						*
 *	      (vjh, August 23, 1985)					*
 *									*
 *	002 - Commented out DoVersionNumber.  No longer used -		*
 *	      see version.h.						*
 *	      (vjh, May 10, 1985)					*
 *									*
 *	001 - Removed call to gethostname().				*
 *	      (Victoria Holt, April 15, 1985)				*
 *									*
 ************************************************************************/

/* Copyright (c) 1982 Regents of the University of California */

static char rcsid[] = "$Header: mkdate.c,v 1.3 84/03/27 10:21:59 linton Exp $";

#include <stdio.h>
#include <sys/time.h>

main()
{
    struct tm *t;
    long clock;
    char name[100];
    int namelen;

    printf("char *date = \"");
    clock = time(0);
    t = localtime(&clock);
    printf("%d/%d/%d ", t->tm_mon + 1, t->tm_mday, t->tm_year % 100);
    printf("%d:%02d", t->tm_hour, t->tm_min);
    printf("\";\n");
/*  DoVersionNumber(); */
}

/* This routine no longer used - see version.h
 *
DoVersionNumber()
{
    FILE *f;
    int n;

    f = fopen("version", "r");
    if (f == NULL) {
	n = 1;
    } else {
	fscanf(f, "%d", &n);
	n = n + 1;
	fclose(f);
    }
    f = fopen("version", "w");
    if (f != NULL) {
	fprintf(f, "%d\n", n);
	fclose(f);
    }
    printf("int versionNumber = %d;\n", n);
}
*/
