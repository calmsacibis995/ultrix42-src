/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldtbseek.c,v 2010.2.1.3 89/11/29 14:28:15 bettina Exp $ */
#include	<stdio.h>
#include	"filehdr.h"
#include	"syms.h"
#include	"ldfcn.h"

int
ldtbseek(ldptr)

LDFILE		*ldptr;

{
    extern int		fseek( );

    extern int		vldldptr( );

    if (vldldptr(ldptr) == SUCCESS && PSYMTAB(ldptr) != NULL) {
	if (SYMHEADER(ldptr).cbSymOffset != 0L) {
	    if (FSEEK(ldptr, SYMHEADER(ldptr).cbSymOffset, BEGINNING) == OKFSEEK) {
		return(SUCCESS);
	    }
	}
    }

    return(FAILURE);
}

static char ID[ ] = "@(#) ldtbseek.c: 1.1 1/7/82";
