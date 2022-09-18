/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: freeldptr.c,v 2010.2.1.3 89/11/29 14:27:35 bettina Exp $ */
#include	<stdio.h>
#include	"filehdr.h"
#include "syms.h"
#include	"ldfcn.h"
#include	"lddef.h"

int
freeldptr(ldptr)

LDFILE	*ldptr;

{
    extern		free( );

    extern LDLIST	*_ldhead;

    LDLIST		*ldindx;

    if (ldptr != NULL) {
	if (_ldhead == (LDLIST *) ldptr) {
	    _ldhead = _ldhead->ld_next;
	    free(ldptr);
	    return(SUCCESS);
	} else {
	    for (ldindx = _ldhead; ldindx != NULL; ldindx = ldindx->ld_next) {
		if (ldindx->ld_next == (LDLIST *) ldptr) {
		    ldindx->ld_next = ((LDLIST *) ldptr)->ld_next;
		    free(ldptr);
		    return(SUCCESS);
		}
	    }
	}
    }

    return(FAILURE);
}

static char ID[ ] = "@(#) freeldptr.c: 1.1 1/7/82";
