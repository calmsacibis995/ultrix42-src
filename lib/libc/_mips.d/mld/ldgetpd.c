/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldgetpd.c,v 2010.2.1.3 89/11/29 14:28:28 bettina Exp $ */
#include <stdio.h>
#include "filehdr.h"
#include "syms.h"
#include "ldfcn.h"

long
ldgetpd(ldptr, ipd, ppd)
	register LDFILE *ldptr;
	register int	ipd;
	pPDR ppd;
{
	extern int vldldptr();
	extern char *malloc();
	extern void free();
	static pPDR pdtab = NULL;
	static int last_fnum_ = 0;
	static long last_offset = -1L;
	static long length;
	int ifd;

	if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
		return (FAILURE);
	if (ldreadst(ldptr,ST_PPDS|ST_PSYMS) == FAILURE)
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
	if (ipd < 0 && SYMHEADER(ldptr).ipdMax >= ipd)
		return (FAILURE);
	*ppd = PSYMTAB(ldptr)->ppd[ipd];
	for (ifd = 0; ifd < SYMHEADER(ldptr).ifdMax; ifd++)
	    if (PSYMTAB(ldptr)->pfd[ifd].cpd > 0 &&
	       ipd >= PSYMTAB(ldptr)->pfd[ifd].ipdFirst &&
	       ipd < PSYMTAB(ldptr)->pfd[ifd].ipdFirst +
	       PSYMTAB(ldptr)->pfd[ifd].cpd)
	       break;
	if (ifd >= SYMHEADER(ldptr).ifdMax)
	    return (FAILURE);
	if (ppd->isym != isymNil)
	    ppd->isym += PSYMTAB(ldptr)->pfd[ifd].isymBase; 
	if (ppd->iline != ilineNil)
	    ppd->iline += PSYMTAB(ldptr)->pfd[ifd].ilineBase; 
	if (ppd->iopt != ioptNil)
	    ppd->iopt += PSYMTAB(ldptr)->pfd[ifd].ioptBase; 
	ppd->adr = PSYMTAB(ldptr)->psym[ppd->isym].value;
	return (SUCCESS);
}

static char ID[] = "@(#) ldgetname.c: 1.2 2/16/83";
