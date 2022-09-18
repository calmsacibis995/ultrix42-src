/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: allocldptr.c,v 2010.2.1.3 89/11/29 14:27:33 bettina Exp $ */
#include <stdio.h>
#include "filehdr.h"
#include "sex.h"
#include "syms.h"
#include "ldfcn.h"
#include "lddef.h"

LDLIST	*_ldhead = NULL;
int ldlast_fnum_ = 0;


LDFILE *
allocldptr()
{
	extern char *calloc();
	extern LDLIST *_ldhead;
	LDLIST *ldptr, *ldindx;

	if ((ldptr = (LDLIST *)calloc(1, LDLSZ)) == NULL)
		return (NULL);
	ldptr->ld_next = NULL;
	if (_ldhead == NULL)
		_ldhead = ldptr;
	else
	{
		for (ldindx = _ldhead;
			ldindx->ld_next != NULL; ldindx = ldindx->ld_next)
		{
		}
		ldindx->ld_next = ldptr;
	}
	ldptr->ld_item._fnum_ = ++ldlast_fnum_;
	ldptr->ld_item.fBigendian = (gethostsex() == BIGENDIAN);
	return (&ldptr->ld_item);
}

static char ID[] = "@(#) allocldptr.c: 1.2 2/16/83";
