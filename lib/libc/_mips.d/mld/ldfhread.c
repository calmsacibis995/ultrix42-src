/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldfhread.c,v 2010.2.1.3 89/11/29 14:27:48 bettina Exp $ */
#include    <stdio.h>
#include    "filehdr.h"
#include    "syms.h"
#include    "ldfcn.h"

int
ldfhread(ldptr, filehead)

LDFILE    *ldptr;
FILHDR    *filehead; 

{
    extern int		vldldptr( );

    if (vldldptr(ldptr) == SUCCESS) {
	if (FSEEK(ldptr, 0L, BEGINNING) == OKFSEEK) {
	    if (FREADM(filehead, FILHSZ, 1, ldptr) == 1) {
		if (LDSWAP(ldptr))
		    swap_filehdr(filehead, gethostsex());
	    	return(SUCCESS);
	    }
	}
    }

    return(FAILURE);
}

static char ID[ ] = "@(#) ldfhread.c: 1.1 1/7/82";
