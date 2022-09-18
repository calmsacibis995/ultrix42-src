/************************************************************************
 *									*
 *			Copyright (c) 1987,1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *		            Bull, France				*
 *			   Siemens AG, FR Germany			*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

#ifndef lint
static char Sccsid[] = "@(#)io.c	4.1	(ULTRIX)	7/17/90";
#endif

#include "ic.h"

/*
 * template for temp file names
 */
static char template[] = "/tmp/icXXXXXX";

/*
 * tmp_make -- create a temporary file
 *	ATTENTION name is replaced with the name of the file created.
 *		assumed usage: fp = tmp_make(sym->sym_???->???_nam)
 */
FILE *
tmp_make(name)
char *name;
{
	char *mktemp();			/* function in libc.a		*/

	strcpy(name, template);
	mktemp(name);

	return fopen(name, "w+");
}

/*
 * tmp_del -- delete all temporary files restoring the name to the header
 */
void
tmp_del()
{
	register sym *sp;

	for (sp = prp_anc; sp; sp = sp->sym_nxt)
		if (strcmp(sp->sym_nam, sp->sym_prp->pr_name) != 0)
		{
			fil_del(sp->sym_prp->pr_name);
			strcpy(sp->sym_prp->pr_name, sp->sym_nam);
		}
	for (sp = col_anc; sp; sp = sp->sym_nxt)
		if (strcmp(sp->sym_nam, sp->sym_col->cl_name) != 0)
		{
			fil_del(sp->sym_col->cl_name);
			strcpy(sp->sym_col->cl_name, sp->sym_nam);
		}
	for (sp = frm_anc; sp; sp = sp->sym_nxt)
		if (strcmp(sp->sym_nam, sp->sym_frm->st_name) != 0)
		{
			fil_del(sp->sym_frm->st_name);
			strcpy(sp->sym_frm->st_name, sp->sym_nam);
		}
	for (sp = cnv_anc; sp; sp = sp->sym_nxt)
		if (strcmp(sp->sym_nam, sp->sym_cnv->cv_name) != 0)
		{
			fil_del(sp->sym_cnv->cv_name);
			strcpy(sp->sym_cnv->cv_name, sp->sym_nam);
		}
}

/*
 * fil_del -- unlink a file
 */
void
fil_del(name)
char *name;
{
	if (unlink(name) != 0)
		bug("fil_del1");
}

/*
 * append -- append a file to the database
 */
void
append(name, to)
char *name;
FILE *to;
{
	FILE *fp;
	register int c;

	if ((fp = fopen(name, "r")) == (FILE *)0)
		bug("append1");

	while ((c = getc(fp)) != EOF)
		putc(c, to);

	fclose(fp);
}

void
put_code(code, fp)
i_char code;
FILE *fp;
{
	if (code & ~0377)
		putc((code >> 8) & 0377, fp);
	putc(code & 0377, fp);
}
