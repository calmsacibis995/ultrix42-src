/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: vldldptr.c,v 2010.2.1.3 89/11/29 14:28:20 bettina Exp $ */
#include	<stdio.h>
#include	"syms.h"
#include	"filehdr.h"
#include	"ldfcn.h"
#include	"lddef.h"

int
vldldptr(ldptr)

LDFILE	*ldptr;

{
    extern LDLIST	*_ldhead;

    LDLIST		*ldindx;

    for (ldindx = _ldhead; ldindx != NULL; ldindx = ldindx->ld_next) {
	if (ldindx == (LDLIST *) ldptr) {
	    if (PSYMTAB(ldptr) != 0)
		st_setchdr(PSYMTAB(ldptr));
	    return(SUCCESS);
	}
    }

    return(FAILURE);
}

static char ID[ ] = "@(#) vldldptr.c: 1.1 1/8/82";
