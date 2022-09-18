/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldaclose.c,v 2010.2.1.3 89/11/29 14:27:38 bettina Exp $ */
#include    <stdio.h>
#include    "filehdr.h"
#include    "syms.h"
#include    "ldfcn.h"

int
ldaclose(ldptr)

LDFILE    *ldptr; 

{
    extern 		fclose( );

    extern int		vldldptr( );
    extern	    	freeldptr( );

    if (vldldptr(ldptr) == FAILURE) {
	return(FAILURE);
    }

    fclose(IOPTR(ldptr));
    freeldptr(ldptr);

    return(SUCCESS);
}

static char ID[ ] = "@(#) ldaclose.c: 1.1 1/7/82";
