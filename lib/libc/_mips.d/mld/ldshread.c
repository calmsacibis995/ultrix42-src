/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldshread.c,v 2010.2.1.3 89/11/29 14:28:08 bettina Exp $ */
#include	<stdio.h>
#include	"filehdr.h"
#include	"scnhdr.h"
#include	"syms.h"
#include	"ldfcn.h"

int
ldshread(ldptr, sectnum, secthdr)

LDFILE	*ldptr;
unsigned short	sectnum;
SCNHDR	*secthdr; 

{
    extern int		vldldptr( );

    if (vldldptr(ldptr) == SUCCESS) {
	if ((sectnum != 0) && (sectnum <= HEADER(ldptr).f_nscns)) {
	    if (FSEEK(ldptr,
		FILHSZ + HEADER(ldptr).f_opthdr + (sectnum - 1L) * SCNHSZ,
		BEGINNING) == OKFSEEK) {
		if (FREADM(secthdr, SCNHSZ, 1, ldptr) == 1) {
		    if (LDSWAP(ldptr))
			swap_scnhdr (secthdr, gethostsex());
		    return(SUCCESS);
		}
	    }
	}
    }

    return(FAILURE);
}

static char ID[ ] = "@(#) ldshread.c: 1.1 1/7/82";
