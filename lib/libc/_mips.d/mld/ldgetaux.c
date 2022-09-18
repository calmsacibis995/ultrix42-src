/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldgetaux.c,v 2010.2.1.3 89/11/29 14:28:22 bettina Exp $ */
#include <stdio.h>
#include "filehdr.h"
#include "syms.h"
#include "ldfcn.h"

AUXU *
ldgetaux(ldptr, iaux)
	register LDFILE *ldptr;
	register int	iaux;
{
	extern int vldldptr();
	extern char *malloc();
	extern void free();
	static pAUXU auxtab = NULL;
	static int last_fnum_ = 0;
	static long last_offset = -1L;
	static long length;

	if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
		return (NULL);
	if (ldreadst(ldptr, ST_PAUXS) == FAILURE)
	    return(NULL);
	/*
	* Different archive members are noted by the OFFSET change.
	* Otherwise, normal other ldptr's are distinguished by
	* the different _fnum_'s.
	*/
	if (last_offset != OFFSET(ldptr) ||
		last_fnum_ != ldptr->_fnum_)
	{
	    st_setchdr(PSYMTAB(ldptr));
	    last_fnum_ = ldptr->_fnum_;
	    last_offset = OFFSET(ldptr);
	}
	if (iaux < 0 || SYMHEADER(ldptr).iauxMax <= iaux)
		return (NULL);
	return (PSYMTAB(ldptr)->paux + iaux);
}

static char ID[] = "@(#) ldgetname.c: 1.2 2/16/83";
