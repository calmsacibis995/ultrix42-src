#ifndef lint
static	char	*sccsid = "@(#)lib.c	4.1	(ULTRIX)	7/17/90";
#endif
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *	Modification History
 *      --------------------
 *
 *	31-May-1989		Tim N
 *		Added this header.  Made changes to up the field count
 *		when a cell is discovered to be a field which is not
 *		defined by the input record.
 ************************************************************************/

#include "stdio.h"
#include "awk.def"
#include "awk.h"
#include "ctype.h"

FILE	*infile	= NULL;
char	*file;
#define	RECSIZE	(5 * 512)
char	record[RECSIZE];
char	fields[RECSIZE];
char	EMPTY[] = "";

#define	MAXFLD	100
int	donefld;	/* 1 = implies rec broken into fields */
int	donerec;	/* 1 = record is valid (no flds have changed) */
int	mustfld;	/* 1 = NF seen, so always break*/

#define	FINIT	{EMPTY, EMPTY, 0.0, FLD|STR, 0}
cell fldtab[MAXFLD] = {	/*room for fields */
	{ "$record", record, 0.0, STR|FLD, 0},
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT
};
int	maxfld	= 0;	/* last used field */


getrec()
{
	register char *rr;
	extern int svargc;
	extern char **svargv;
	register c, sep;

	dprintf("**RS=%o, **FS=%o\n", **RS, **FS, NULL);
	donefld = 0;
	donerec = 1;
	record[0] = 0;
	while (svargc > 0) {
		dprintf("svargc=%d, *svargv=%s\n", svargc, *svargv, NULL);
		if (infile == NULL) {	/* have to open a new file */
			if (member('=', *svargv)) {	/* it's a var=value argument */
				setclvar(*svargv);
				svargv++;
				svargc--;
				continue;
			}
			*FILENAME = file = *svargv;
			dprintf("opening file %s\n", file, NULL, NULL);
			if (*file == '-')
				infile = stdin;
			else if ((infile = fopen(file, "r")) == NULL)
				error(FATAL, "can't open %s", file);
		}
		if ((sep = **RS) == 0)
			sep = '\n';
		for (rr = record; ; ) {
			for (; (c=getc(infile)) != sep && c != EOF; *rr++ = c)
				;
			if (**RS == sep || c == EOF)
				break;
			if ((c = getc(infile)) == '\n' || c == EOF)	/* 2 in a row */
				break;
			*rr++ = '\n';
			*rr++ = c;
		}
		if (rr > record+RECSIZE)
			error(FATAL, "record `%.20s...' too long", record);
		*rr = 0;
		if (mustfld)
			fldbld();
		if (c != EOF || rr > record) {	/* normal record */
			recloc->tval &= ~NUM;
			recloc->tval |= STR;
			++nrloc->fval;
			maxfld = 0;
			nrloc->tval &= ~STR;
			nrloc->tval |= NUM;
			return(1);
		}
		/* EOF arrived on this file; set up next */
		if (infile != stdin)
			fclose(infile);
		infile = NULL;
		svargc--;
		svargv++;
	}
	return(0);	/* true end of file */
}

setclvar(s)	/* set var=value from s */
char *s;
{
	char *p;
	cell *q;

	for (p=s; *p != '='; p++)
		;
	*p++ = 0;
	q = setsymtab(s, tostring(p), 0.0, STR, symtab);
	setsval(q, p);
	dprintf("command line set %s to |%s|\n", s, p, NULL);
}

fldbld()
{
	register char *r, *fr, sep;
	int i, j;

	r = record;
	fr = fields;
	i = 0;	/* number of fields accumulated here */
	if ((sep = **FS) == ' ')
		for (i = 0; ; ) {
			while (*r == ' ' || *r == '\t' || *r == '\n')
				r++;
			if (*r == 0)
				break;
			i++;
			if (i >= MAXFLD)
				error(FATAL, "record `%.20s...' has too many fields", record);
			if (!(fldtab[i].tval&FLD))
				strfree(fldtab[i].sval);
			fldtab[i].sval = fr;
			fldtab[i].tval = FLD | STR;
			do
				*fr++ = *r++;
			while (*r != ' ' && *r != '\t' && *r != '\n' && *r != '\0');
			*fr++ = 0;
		}
	else if (*r != 0)	/* if 0, it's a null field */
		for (;;) {
			i++;
			if (i >= MAXFLD)
				error(FATAL, "record `%.20s...' has too many fields", record);
			if (!(fldtab[i].tval&FLD))
				strfree(fldtab[i].sval);
			fldtab[i].sval = fr;
			fldtab[i].tval = FLD | STR;
			while (*r != sep && *r != '\n' && *r != '\0')	/* \n always a separator */
				*fr++ = *r++;
			*fr++ = 0;
			if (*r++ == 0)
				break;
		}
	*fr = 0;

	/* check to see if the field count has been upped by assigning
	 * a value to a field not defined in the input record.
	 */
	if ( i < maxfld )
		i = maxfld;

	for (j=MAXFLD-1; j>i; j--) {	/* clean out junk from previous record */
		if (!(fldtab[j].tval&FLD))
			strfree(fldtab[j].sval);
		fldtab[j].tval = STR | FLD;
		fldtab[j].sval = EMPTY;
	}
	maxfld = i;
	donefld = 1;
	for(i=1; i<=maxfld; i++)
		if(isnumber(fldtab[i].sval)) {
			fldtab[i].fval = atof(fldtab[i].sval);
			fldtab[i].tval |= NUM;
		}
	setfval(lookup("NF", symtab, 0), (awkfloat) maxfld);
	if (dbg)
		for (i = 0; i <= maxfld; i++)
			printf("field %d: |%s|\n", i, fldtab[i].sval);
}

void num_of_fields(nf)
	char	nf;
{
	if( nf > maxfld ){
		maxfld = nf;
		setfval(lookup("NF", symtab, 0), (awkfloat) maxfld);
	}
}

recbld()
{
	int i;
	register char *r, *p;

	if (donefld == 0 || donerec == 1)
		return;
	r = record;
	for (i = 1; i <= *NF; i++) {
		p = getsval(&fldtab[i]);
		while (*r++ = *p++)
			;
		*(r-1) = **OFS;
	}
	*(r-1) = '\0';
	dprintf("in recbld FS=%o, recloc=%o\n", **FS, recloc, NULL);
	recloc->tval = STR | FLD;
	dprintf("in recbld FS=%o, recloc=%o\n", **FS, recloc, NULL);
	if (r > record+RECSIZE)
		error(FATAL, "built giant record `%.20s...'", record);
	dprintf("recbld = |%s|\n", record, NULL, NULL);
}

cell *fieldadr(n)
{
	if (n >= MAXFLD)
		error(FATAL, "trying to access field %d", n);
	fldtab[n].field_num = n;
	return(&fldtab[n]);
}

int	errorflag	= 0;

yyerror(s) char *s; {
	fprintf(stderr, "awk: %s near line %d\n", s, lineno);
	errorflag = 2;
}

error(f, s, a1, a2, a3, a4, a5, a6, a7) {
	fprintf(stderr, "awk: ");
	fprintf(stderr, s, a1, a2, a3, a4, a5, a6, a7);
	fprintf(stderr, "\n");
	if (NR && *NR > 0)
		fprintf(stderr, " record number %g\n", *NR);
	if (f)
		exit(2);
}

PUTS(s) char *s; {
	dprintf("%s\n", s, NULL, NULL);
}

#define	MAXEXPON	38	/* maximum exponenet for fp number */

isnumber(s)
register char *s;
{
	register d1, d2;
	int point;
	char *es;

	if (s == NULL) 
		return (0);
	d1 = d2 = point = 0;
	while (*s == ' ' || *s == '\t' || *s == '\n')
		s++;
	if (*s == '\0')
		return(0);	/* empty stuff isn't number */
	if (*s == '+' || *s == '-')
		s++;
	if (!isdigit(*s) && *s != '.')
		return(0);
	if (isdigit(*s)) {
		do {
			d1++;
			s++;
		} while (isdigit(*s));
	}
	if(d1 >= MAXEXPON)
		return(0);	/* too many digits to convert */
	if (*s == '.') {
		point++;
		s++;
	}
	if (isdigit(*s)) {
		d2++;
		do {
			s++;
		} while (isdigit(*s));
	}
	if (!(d1 || point && d2))
		return(0);
	if (*s == 'e' || *s == 'E') {
		s++;
		if (*s == '+' || *s == '-')
			s++;
		if (!isdigit(*s))
			return(0);
		es = s;
		do {
			s++;
		} while (isdigit(*s));
		if (s - es > 2)
			return(0);
		else if (s - es == 2 && 10 * (*es-'0') + *(es+1)-'0' >= MAXEXPON)
			return(0);
	}
	while (*s == ' ' || *s == '\t' || *s == '\n')
		s++;
	if (*s == '\0')
		return(1);
	else
		return(0);
}
/*
isnumber(s) char *s; {return(0);}
*/
