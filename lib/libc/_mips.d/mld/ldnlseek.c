/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldnlseek.c,v 2010.2.1.3 89/11/29 14:27:55 bettina Exp $ */
#include	<stdio.h>
#include	"filehdr.h"
#include	"scnhdr.h"
#include	"syms.h"
#include	"ldfcn.h"

int
ldnlseek(ldptr, sectname)

LDFILE	*ldptr;
char 	*sectname; 

{

	if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
		return (NULL);
	return (ldlseek (ldptr, 0));

}

static char ID[ ] = "@(#) ldnlseek.c: 1.1 1/7/82";
