/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldtbindex.c,v 2010.2.1.3 89/11/29 14:28:12 bettina Exp $ */
#include	<stdio.h>
#include	"filehdr.h"
#include	"syms.h"
#include	"ldfcn.h"

long
ldtbindex(ldptr)

LDFILE	*ldptr;

{
    extern long		ftell( );

    extern int		vldldptr( );

    long		position;


    if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
	return (BADINDEX);
    if (ldptr->lastindex >= 0) {

	if (ldptr->lastindex < SYMHEADER(ldptr).isymMax) {
	    return(ldptr->lastindex);
	} else if (ldptr->lastindex < SYMHEADER(ldptr).isymMax +
	    SYMHEADER(ldptr).iextMax) {
	    return(ldptr->lastindex);
	} /* if */
    }

    return(BADINDEX);
}

static char	ID[ ] = "@(#) ldtbindex.c: 1.1 1/7/82";
