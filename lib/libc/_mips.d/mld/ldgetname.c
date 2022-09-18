/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldgetname.c,v 2010.2.1.3 89/11/29 14:27:50 bettina Exp $ */
#include <stdio.h>
#include "filehdr.h"
#include "syms.h"
#include "ldfcn.h"

char *
ldgetname(ldptr, psym)
	register LDFILE *ldptr;
	register pSYMR	psym;
{
	extern int vldldptr();
	extern char *strncpy();
	extern char *strcpy();
	extern char *malloc();
	extern void free();
	static int last_fnum_ = 0;
	static long last_offset = -1L;

	if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
		return (NULL);
	/*
	* Different archive members are noted by the OFFSET change.
	* Otherwise, normal other ldptr's are distinguished by
	* the different _fnum_'s.
	*/
	if (last_offset != OFFSET(ldptr) ||
		last_fnum_ != ldptr->_fnum_)
	{
	    st_setchdr (PSYMTAB(ldptr));
	    last_fnum_ = ldptr->_fnum_;
	    last_offset = OFFSET(ldptr);
	}
	if (psym->iss >= 0 && psym->iss < SYMHEADER(ldptr).issMax) {
	    if (ldreadst(ldptr,ST_PSSS) == FAILURE)
		return(NULL);
	    return (PSYMTAB(ldptr)->pss + psym->iss);
	} /* if */
	if (psym->iss >= 0 && psym->iss >= SYMHEADER(ldptr).issMax &&
	    psym->iss < SYMHEADER(ldptr).issMax+SYMHEADER(ldptr).issExtMax) {
	    if (ldreadst(ldptr,ST_PSSEXTS) == FAILURE)
		return(NULL);
		return (PSYMTAB(ldptr)->pssext + 
		    (psym->iss - SYMHEADER(ldptr).issMax));
	} /* if */
	return (NULL);
}

static char ID[] = "@(#) ldgetname.c: 1.2 2/16/83";
