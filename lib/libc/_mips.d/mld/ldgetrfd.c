/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldgetrfd.c,v 2010.2.1.4 89/11/29 14:28:25 bettina Exp $ */
#include <stdio.h>
#include "filehdr.h"
#include "syms.h"
#include "ldfcn.h"

#define	MAX_ST_MBT_ASCII	25	/* array size of st_mbt_ascii */
					/* defined in stprint.c	      */
RFDT
ldgetrfd(ldptr, rfd)
	register LDFILE *ldptr;
	int	rfd;
{
	extern int vldldptr();
	extern char *malloc();
	extern void free();
	static int last_fnum_ = 0;
	static long last_offset = -1L;

	if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
		return (NULL);
	if (ldreadst(ldptr,ST_PRFDS) == FAILURE)
	    return(NULL);
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
	if (SYMHEADER(ldptr).crfd == 0)
	    return (rfd);

	if (SYMHEADER(ldptr).crfd <= rfd)
	    return (NULL);
	return (PSYMTAB(ldptr)->prfd[rfd]);
}

#define STRSIZE	256

static ldgettypestr (ldptr, iaux, straux)

LDFILE	*ldptr;
int	iaux;		/* auxiliary index */
char	*straux;

{

    int		rfdexists;		/* true if it does */
    int		rfd;			/* rfd returned from ldgetrndx */
    int		ifd;			/* file table index for aux */
    int		ifd2;			/* dito for indirect index of types */
    int		inc = 0;		/* current offset from base aux */
    SYMR	asym;			/* used to get the name of a type */
    pAUXU	paux;			/* aux we're interested in */
    int		isym;			/* index of type's sym */
    int		iaux2;			/* index of type's auxiliary */
    int		width;			/* width of basic type */
    AUXU	aux;			/* temp aux for swapping */
    AUXU	aux1;			/* temp aux for swapping */
    AUXU	aux2;			/* temp aux for swapping */
    AUXU	aux3;			/* temp aux for swapping */
    char	straux1[STRSIZE];
    char	straux2[STRSIZE];
    char	straux3[STRSIZE];
    char	straux4[STRSIZE];
    char	straux5[STRSIZE];
    extern char *st_mbt_ascii[];
    extern pAUXU ldgetaux();

    if (iaux == indexNil)
	return;

    if (ldreadst(ldptr,ST_PAUXS|ST_PFDS|ST_PSYMS) == FAILURE)
	return(NULL);
    /* warning, this is a tough routine it contains a lot of special casing
     *	based on types and recursion when necessary.
     */

    /* initializing things:
     *	 - get the ifd of the iaux param
     *   - set a flag if an indirect table exists
     *	 - get a pointer to the auxiliary
     */
    ifd = ld_ifd_iaux (ldptr, iaux);

    rfdexists = PFD(ldptr)[ifd].crfd;

    paux = ldgetaux (ldptr, iaux);
    if (paux == auxNil)
	return (FAILURE);
    aux = *paux;

    if (LDAUXSWAP(ldptr,ifd))
	swap_aux (&aux, ST_AUX_TIR, gethostsex());

    strcpy (straux, "");		/* make sure it's empty */

    /* stick the basic type into the string */
    if (aux.ti.bt > MAX_ST_MBT_ASCII)
    	strcat(straux, "BOGUS type");
    else
    	strcat (straux, st_mbt_ascii[aux.ti.bt]);
    strcat (straux, " ");

    if (aux.ti.fBitfield != 0) {
	aux1 = paux[1];
	if (LDAUXSWAP(ldptr,ifd))
	    swap_aux (&aux1, ST_AUX_WIDTH, gethostsex());
	width = aux1.width;
	paux++;
    } /* if */


    switch (aux.ti.bt) {

    case btTypedef:
    case btStruct:
    case btEnum:
    case btUnion:
	/*  paux[1] is a rndx which points indirectly through the rfd
	 *	table (if there is one, is direct otherwise) to a symbol
	 *	which we plan to concatenate the name of onto our string.
	 */
	inc += ldgetrndx(ldptr, ifd, paux + 1, &rfd, &aux1);
	if (aux1.rndx.index && aux1.rndx.index != ST_ANONINDEX) {	
	    /* if == 0, no type info for ref */
	    if (rfdexists) {
		/* get the index indirect through the rfd table */
		ifd2 = ldgetrfd(ldptr, PFD(ldptr)[ifd].rfdBase + rfd);
		isym = PFD(ldptr)[ifd2].isymBase + aux1.rndx.index;
	    } else {
		/* the rfd table is the identity mapping so rndx.rfd is really
		 *	an ifd (think about it)
		 */
		isym = PFD(ldptr)[rfd].isymBase + 
		    aux1.rndx.index;
	    } /* if */
	    /* get the symbol and then its name */
	    if (ldtbread (ldptr, isym, &asym) == FAILURE) {
		strcat (straux, "CANNOT deref basic type");
		return (FAILURE);
	    } /* if */
	    strcat (straux, ldgetname (ldptr, &asym));
	} /* if */
	inc +=1;
	break;
	
    case btIndirect:
    case btRange:
    case btSet:
	/* same as above except that now the rndx is a pointer to a
	 *	aux entry. So we just call ourselves recursively.
	 *	It should stop (maybe a level should be inserted in the
	 *	call.
	 */
	inc += ldgetrndx(ldptr, ifd, paux + 1, &rfd, &aux1);
	if (aux1.rndx.index == 0) {
	    /* no type info available */
	    inc += 1;
	    break;
	} /* if */
	if (rfdexists) {
	    ifd2 = ldgetrfd(ldptr, PFD(ldptr)[ifd].rfdBase + rfd);
	    iaux2 = PFD(ldptr)[ifd2].iauxBase + aux1.rndx.index;
	} else {
	    iaux2 = PFD(ldptr)[rfd].iauxBase + 
		aux1.rndx.index;
	} /* if */
	ldgettypestr (ldptr, iaux2, &straux[strlen(straux)]);
	inc +=1;
	if (aux.ti.bt == btRange) {
	    /* low and high dimensions of the range, if it's an enum
	     *	maybe we should print out the real names (do it later)
	     */
	    aux2 = paux[1+inc];
	    aux3 = paux[2+inc];
	    if (LDAUXSWAP(ldptr,ifd)) {
		swap_aux (&aux2, ST_AUX_DNLOW, gethostsex());
		swap_aux (&aux3, ST_AUX_DNMAC, gethostsex());
	    } /* if */
	    sprintf (&straux[strlen(straux)], " (%d..%d)", aux2.dnLow,
		aux3.dnHigh);
	    inc += 2;
	} /* if */
	break;
    } /* switch */

    if (aux.ti.fBitfield != 0)
	sprintf (&straux[strlen(straux)], " : %d ", width);

    /* add on the tq info */
    straux1[0] = straux2[0] = straux3[0] = straux4[0] = straux5[0] = '\0';
    ldtqstr (ldptr, aux.ti.tq5, straux5, &inc, ifd, rfdexists, paux);
    ldtqstr (ldptr, aux.ti.tq4, straux4, &inc, ifd, rfdexists, paux);
    ldtqstr (ldptr, aux.ti.tq3, straux3, &inc, ifd, rfdexists, paux);
    ldtqstr (ldptr, aux.ti.tq2, straux2, &inc, ifd, rfdexists, paux);
    ldtqstr (ldptr, aux.ti.tq1, straux1, &inc, ifd, rfdexists, paux);
    ldtqstr (ldptr, aux.ti.tq0, straux, &inc, ifd, rfdexists, paux);
    strcat (straux, straux1);
    strcat (straux, straux2);
    strcat (straux, straux3);
    strcat (straux, straux4);
    strcat (straux, straux5);

    return (SUCCESS);

} /* ldgettypestr */


static ldtqstr (ldptr, tq, straux, inc, ifd, rfdexists, pauxp)

int	tq;
LDFILE	*ldptr;
char	*straux;
int	*inc;
int	ifd;
int	rfdexists;
pAUXU	pauxp;

{
    int		rfd;			/* rfd returned from ldgetrndx */
    int	ifd2;
    int	iaux2;
    AUXU	aux1;
    AUXU	aux2;
    AUXU	aux3;
    AUXU	aux4;
    /* KLUDGE, ccom can't handle pauxp, too many params or something */
    pAUXU	paux = pauxp;

    if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
	    return (NULL);
    if (ldreadst(ldptr,ST_PAUXS|ST_PFDS|ST_PSYMS) == FAILURE)
	return(NULL);
    switch (tq) {

    case tqProc:
	strcat (straux, "()");
	break;

    case tqPtr:
	strcat (straux, "*");
	break;

    case tqVol:
	strcat (straux, "volatile");
	break;
	
    case tqArray:
	/* this is the only tuffy and it resembles ranges very much */
	*inc += ldgetrndx(ldptr, ifd, paux + *inc + 1, &rfd, &aux1);
	aux2 = paux[*inc + 2];
	aux3 = paux[*inc + 3];
	aux4 = paux[*inc + 4];

	if (LDAUXSWAP(ldptr,ifd)) {
	    swap_aux (&aux2, ST_AUX_DNLOW, gethostsex());
	    swap_aux (&aux3, ST_AUX_DNMAC, gethostsex());
	    swap_aux (&aux4, ST_AUX_WIDTH, gethostsex());
	} /* if */
	if (PFD(ldptr)[ifd].lang == langC) {
	    if (aux3.dnHigh == 0)
		strcat (straux, "[]");
	    else
		sprintf (&straux[strlen(straux)], "[%d]", aux3.dnHigh + 1);
	} else {
	    strcat (straux, "[");
	    if (rfdexists) {
		ifd2 = ldgetrfd(ldptr, PFD(ldptr)[ifd].rfdBase + 
		    rfd);
		iaux2 = PFD(ldptr)[ifd2].iauxBase + aux1.rndx.index;
	    } else {
		iaux2 = PFD(ldptr)[rfd].iauxBase + 
		    aux1.rndx.index;
	    } /* if */
	    ldgettypestr (ldptr, iaux2, &straux[strlen(straux)]);
	    sprintf (&straux[strlen(straux)], ":%d:%d(%d)]", aux2.dnLow, 
		aux3.dnHigh, aux4.width);
	} /* if */
	*inc += 4;
    } /* switch */
} /* ldtqstr */


ldgetsymstr (ldptr, psym, isym, straux)

LDFILE	*ldptr;
pSYMR	psym;
char	*straux;

{
    int		ifd;			/* file table index for aux */
    pAUXU	paux;
    AUXU	aux;


    if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
	    return (NULL);
    if (ldreadst(ldptr,ST_PAUXS|ST_PFDS|ST_PSYMS) == FAILURE)
	return(NULL);
    strcpy (straux, "");

    switch (psym->st) {

    case stLabel:
	return (SUCCESS);

    case stFile:
    case stBlock:
    case stEnd:
	sprintf (straux, "ref=%d", psym->index);
	return (SUCCESS);

    case stStaticProc:
    case stProc:
	if (isym >= SYMHEADER(ldptr).isymMax &&
	    (psym->sc == scText || psym->sc == scNil)) {
	    if (psym->sc == scText)
	    	sprintf (straux, "ref=%d ", psym->index);
	    return (SUCCESS);
	} else {
	    ifd = ld_ifd_iaux (ldptr, psym->index);
	    paux = ldgetaux (ldptr, psym->index);
	    if (paux == auxNil)
		return (FAILURE);
	    aux = paux[0];
	    if (LDAUXSWAP(ldptr,ifd))
		swap_aux (&aux, ST_AUX_ISYM, gethostsex());
	    sprintf (straux, "end=%d ", aux.isym);
	    psym->index++;
	    straux += strlen(straux);
	} /* if */
	/* fall through */
	    
    default:
	ldgettypestr(ldptr, psym->index, straux);
    } /* switch */
} /* ldgetsymstr */


ldgetrndx(ldptr, ifd, paux, prfd, praux)
LDFILE	*ldptr;
AUXU	*paux, *praux;
long	*prfd;
{
    int inc = 0;
    AUXU auxrfd;

    *praux = *paux;
    if (LDAUXSWAP(ldptr,ifd))
	swap_aux (praux, ST_AUX_RNDXR, gethostsex());
    if (praux->rndx.rfd == ST_RFDESCAPE) {
	auxrfd = paux[1];
	if (LDAUXSWAP(ldptr,ifd))
	    swap_aux (&auxrfd, ST_AUX_ISYM, gethostsex());
	*prfd = auxrfd.isym;
	inc = 1;
    } else {
	*prfd = praux->rndx.rfd;
    } /* if */
    return inc;
} /* ldgetrndx */

