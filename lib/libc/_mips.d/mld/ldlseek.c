/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldlseek.c,v 2010.2.1.3 89/11/29 14:27:54 bettina Exp $ */
#include	<stdio.h>
#include	"filehdr.h"
#include	"scnhdr.h"
#include	"syms.h"
#include	"ldfcn.h"

int
ldlseek(ldptr, sectnum)

LDFILE		*ldptr;
unsigned short	sectnum; 

{
	extern	int	fseek( );

	if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
		return (NULL);
	if (SYMHEADER(ldptr).ilineMax != 0) {
	    if (FSEEK(ldptr, SYMHEADER(ldptr).cbLineOffset, BEGINNING)
		== OKFSEEK) {
		    return(SUCCESS);
	    }
	}

	return(FAILURE);
}

static char ID[ ] = "@(#) ldlseek.c: 1.1 1/7/82";
