/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldnsseek.c,v 2010.2.1.3 89/11/29 14:28:01 bettina Exp $ */
#include	<stdio.h>
#include	"filehdr.h"
#include	"scnhdr.h"
#include	"syms.h"
#include	"ldfcn.h"

int
ldnsseek(ldptr, sectname)

LDFILE	*ldptr;
char 	*sectname; 

{
	extern	int	ldnshread( );
	extern	int	fseek( );

	SCNHDR	shdr;

	if (ldnshread(ldptr, sectname, &shdr) == SUCCESS) {
		if (shdr.s_scnptr != 0) {
		    if (FSEEK(ldptr, shdr.s_scnptr, BEGINNING) == OKFSEEK) {
			    return(SUCCESS);
		    }
		}
	}

	return(FAILURE);
}

static char ID[ ] = "@(#) ldnsseek.c: 1.1 1/7/82";
