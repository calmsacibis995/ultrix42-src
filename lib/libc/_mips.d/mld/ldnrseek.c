/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldnrseek.c,v 2010.2.1.3 89/11/29 14:27:57 bettina Exp $ */
#include	<stdio.h>
#include	"filehdr.h"
#include	"scnhdr.h"
#include	"syms.h"
#include	"ldfcn.h"

int
ldnrseek(ldptr, sectname)

LDFILE	*ldptr;
char 	*sectname; 


{
    SCNHDR scnhdr;

    if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
	    return (FAILURE);
    if (ldnshread(ldptr, sectname, &scnhdr) != SUCCESS)
	return (FAILURE);
    if (scnhdr.s_relptr == 0)
	return (FAILURE);
    if (FSEEK(ldptr, scnhdr.s_relptr, BEGINNING) == OKFSEEK)
	return (SUCCESS);
    else
	return (FAILURE);
}

static char ID[ ] = "@(#) ldnrseek.c: 1.1 1/7/82";
