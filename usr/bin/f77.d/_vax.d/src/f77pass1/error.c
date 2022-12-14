#ifndef lint
static	char	*sccsid = "@(#)error.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 * error.c
 *
 * Error handling routines for f77 compiler pass 1, 4.2 BSD.
 *
 * University of Utah CS Dept modification history:
 *
 * $Log:	error.c,v $
 * Revision 1.2  84/08/20  17:57:20  donn
 * Added strategic colons to the format strings in execerr() and dclerr().
 * 
 */

#include "defs.h"


warn1(s,t)
char *s, *t;
{
char buff[100];
sprintf(buff, s, t);
warn(buff);
}


warn(s)
char *s;
{
if(nowarnflag)
	return;
fprintf(diagfile, "Warning on line %d of %s: %s\n", lineno, infname, s);
++nwarn;
}


errstr(s, t)
char *s, *t;
{
char buff[100];
sprintf(buff, s, t);
err(buff);
}


errnm(fmt, l, s)
char *fmt;
int l;
register char *s;
{
  char buff[VL+1];
  register int i;

  i = 0;
  while (i < l)
    {
      buff[i] = s[i];
      i++;
    }
  buff[i] = '\0';

  errstr(fmt, buff);
}

warnnm(fmt, l, s)
char *fmt;
int l;
register char *s;
{
  char buff[VL+1];
  register int i;

  i = 0;
  while (i < l)
    {
      buff[i] = s[i];
      i++;
    }
  buff[i] = '\0';

  warn1(fmt, buff);
}

erri(s,t)
char *s;
int t;
{
char buff[100];
sprintf(buff, s, t);
err(buff);
}


err(s)
char *s;
{
fprintf(diagfile, "Error on line %d of %s: %s\n", lineno, infname, s);
++nerr;
optoff();
}


yyerror(s)
char *s;
{ err(s); }



dclerr(s, v)
char *s;
Namep v;
{
char buff[100];

if(v)
	{
	sprintf(buff, "Declaration error for %s: %s", varstr(VL, v->varname), s);
	err(buff);
	}
else
	errstr("Declaration error: %s", s);
}



execerr(s, n)
char *s, *n;
{
char buf1[100], buf2[100];

sprintf(buf1, "Execution error: %s", s);
sprintf(buf2, buf1, n);
err(buf2);
}


fatal(t)
char *t;
{
fprintf(diagfile, "Compiler error line %d of %s: %s\n", lineno, infname, t);
if (debugflag[8])
	showbuffer();
if (debugflag[0])
	abort();
done(3);
exit(3);
}




fatalstr(t,s)
char *t, *s;
{
char buff[100];
sprintf(buff, t, s);
fatal(buff);
}



fatali(t,d)
char *t;
int d;
{
char buff[100];
sprintf(buff, t, d);
fatal(buff);
}



badthing(thing, r, t)
char *thing, *r;
int t;
{
char buff[50];
sprintf(buff, "Impossible %s %d in routine %s", thing, t, r);
fatal(buff);
}



badop(r, t)
char *r;
int t;
{
badthing("opcode", r, t);
}



badtag(r, t)
char *r;
int t;
{
badthing("tag", r, t);
}





badstg(r, t)
char *r;
int t;
{
badthing("storage class", r, t);
}




badtype(r, t)
char *r;
int t;
{
badthing("type", r, t);
}


many(s, c)
char *s, c;
{
char buff[25];

sprintf(buff, "Too many %s.  Try the -N%c option", s, c);
fatal(buff);
}


err66(s)
char *s;
{
errstr("Fortran 77 feature used: %s", s);
}



errext(s)
char *s;
{
errstr("F77 compiler extension used: %s", s);
}
