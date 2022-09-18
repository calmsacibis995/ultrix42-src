/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldtbread.c,v 2010.2.1.3 89/11/29 14:28:13 bettina Exp $ */
#include	<stdio.h>
#include	"filehdr.h"
#include	"syms.h"
#include	"ldfcn.h"
#include	"sex.h"

extern pEXTR st_pext_iext();
extern pSYMR st_psym_ifd_isym();

int
ldtbread(ldptr, symnum, psym)

LDFILE	*ldptr;
long	symnum;
pSYMR	psym;

{
    extern int		vldldptr( );
    int			home;
    int			size;
    EXTR		aext;

    if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
	return(FAILURE);
    if ((symnum >= 0) && (symnum <= SYMHEADER(ldptr).isymMax + 
	    SYMHEADER(ldptr).iextMax)) {
	ldptr->lastindex = symnum;
	if ((symnum >= (SYMHEADER(ldptr)).isymMax)) {
	    if (ldreadst(ldptr, ST_PFDS|ST_PEXTS) == FAILURE)
		return (FAILURE);
	    symnum -= SYMHEADER(ldptr).isymMax;
	    home = SYMHEADER(ldptr).cbExtOffset;
	    size = cbEXTR;
	    aext = *st_pext_iext(symnum);
	    *psym = aext.asym;
	    /* make iss and index absolute */
	    psym->iss += SYMHEADER(ldptr).issMax;
	    if (psym->index != indexNil) {
		if (psym->sc != scUndefined && psym->st == stProc)
		    psym->index += PFD(ldptr)[aext.ifd].isymBase;
		else
		    psym->index += PFD(ldptr)[aext.ifd].iauxBase;
	    }
	    return(SUCCESS);
	} else {
	    int ifd = ld_ifd_symnum (ldptr, symnum);

	    if (ldreadst(ldptr, ST_PFDS|ST_PSYMS) == FAILURE)
		return (FAILURE);
	    if (ifd == ifdNil)
		return (FAILURE);
	    *psym = *st_psym_ifd_isym (ifd, symnum - 
		PFD(ldptr)[ifd].isymBase);
	    psym->iss += PFD(ldptr)[ifd].issBase;
	    if (psym->index != indexNil) switch (psym->st) {
		case stFile:
		case stBlock:
		case stEnd:
		    psym->index += PFD(ldptr)[ifd].isymBase;
		case stLabel:
		    break;

		default:
		    psym->index += PFD(ldptr)[ifd].iauxBase;
	    } /* switch */
	    return(SUCCESS);
	} /* if */
    }

    return(FAILURE);
}

ld_ifd_symnum (ldptr, symnum)

LDFILE	*ldptr;
int	symnum;

{
    int ifd;
    static int lastifd = ifdNil;

    if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
	return(FAILURE);
    if (ldreadst(ldptr, ST_PFDS) == FAILURE)
	return (FAILURE);
    if (lastifd != ifdNil &&
	lastifd < SYMHEADER(ldptr).ifdMax &&
	symnum >= PFD(ldptr)[lastifd].isymBase &&
	symnum < PFD(ldptr)[lastifd].isymBase + PFD(ldptr)[lastifd].csym)
	return(lastifd);
    for (ifd = 0; ifd < SYMHEADER(ldptr).ifdMax; ifd++)
	if (symnum >= PFD(ldptr)[ifd].isymBase &&
	    symnum < PFD(ldptr)[ifd].isymBase + PFD(ldptr)[ifd].csym)
	    return (lastifd = ifd);

    return (lastifd = ifdNil);
}

ld_ifd_iaux (ldptr, iaux)

LDFILE	*ldptr;
int	iaux;

{
    int ifd;

    if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
	return(FAILURE);
    if (ldreadst(ldptr, ST_PFDS) == FAILURE)
	return (FAILURE);
    for (ifd = 0; ifd < SYMHEADER(ldptr).ifdMax; ifd++)
	if (iaux >= PFD(ldptr)[ifd].iauxBase &&
	    iaux < PFD(ldptr)[ifd].iauxBase + PFD(ldptr)[ifd].caux)
	    return (ifd);

    return (ifdNil);
}
static char ID[ ] = "@(#) ldtbread.c: 1.1 1/7/82";
