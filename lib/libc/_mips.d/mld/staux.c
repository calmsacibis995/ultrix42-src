/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: staux.c,v 2010.2.1.3 89/11/29 14:29:42 bettina Exp $ */
/*
 * Author	Mark I. Himelstein
 * Date started 5/10/85
 * Module	staux.c
 * Purpose	provide a set of routine to access and modify
 *		auxiliary entries
 *
 * Naming conventions:
 *		- all function interfaces are prefixed with st_
 *		- functions returning a value from an arg have names of the
 *			form result_arg
 *		- all globals prefixed with a st_
 *		- all arrays start with an a
 *		- all pointers start with a p
 *		- an array mapping thing1 to thing to is named mthing1_thing2
 */

#include "syms.h"
#include "stext.h"
#include "sex.h"

#include <stdio.h>

#ifdef SYSV
#define bzero(p,n) memset(p,0,n)
#endif

/* AUX routines */

st_tqhigh_iaux (iaux)

long	iaux;

{
    pAUXU	paux;

    paux = st_paux_iaux (iaux);
    /* return the highest numbered tq set in the aux */
    if (paux->ti.tq5 != tqNil)
	    return (paux->ti.tq5);
    if (paux->ti.tq4 != tqNil)
	    return (paux->ti.tq4);
    if (paux->ti.tq3 != tqNil)
	    return (paux->ti.tq3);
    if (paux->ti.tq2 != tqNil)
	    return (paux->ti.tq2);
    if (paux->ti.tq1 != tqNil)
	    return (paux->ti.tq1);
    if (paux->ti.tq0 != tqNil)
	    return (paux->ti.tq0);
    return (tqNil);
} /* st_tqhigh_paux */


export void st_shifttq (iaux, tq)

long	iaux;
long	tq;

{
    /* shift in the tq to the iaux auxiliary entry in the current file */
    pAUXU	paux;

    paux = st_paux_iaux (iaux);

    paux->ti.tq5 = paux->ti.tq4;
    paux->ti.tq4 = paux->ti.tq3;
    paux->ti.tq3 = paux->ti.tq2;
    paux->ti.tq2 = paux->ti.tq1;
    paux->ti.tq1 = paux->ti.tq0;

    paux->ti.tq0 = tq;
} /* st_shifttq */


export long st_iaux_copyty (ifd, psym)

long	ifd;
pSYMR	psym;

{
    /* copy an aux from file ifd and symbol psym to the current file
     *	and return the index to it.
     */
    long	caux = 0;
    long	iaux;
    long	iauxret;
    pAUXU	paux;
    pTIR	pti;

    /* caux should index paux to next auxiliary */
    paux = st_paux_ifd_iaux (ifd, psym->index);

    if (psym->st == stProc)
	caux++;

    pti = (TIR *)(paux + caux);
    caux++;

    if (pti->bt == btSet || pti->bt == btIndirect || ST_FCOMPLEXBT(pti->bt)) {
	/* has a rndx to reference definition */
	if (paux[caux].rndx.rfd == ST_RFDESCAPE)
	    caux++;
	caux++;
    } else if (pti->bt == btRange) {
	/* has a rndx to reference definition + low and high values */
	if (paux[caux].rndx.rfd == ST_RFDESCAPE)
	    caux++;
	caux+=3;
    } /* if */

	/* has rndx for index type, low, high and stride */
#define IFRFDESCAPE() if (paux[caux].rndx.rfd == ST_RFDESCAPE) caux++;
#define IFARRAY(tq) IFRFDESCAPE(); if (pti->tq == tqArray) caux+=4;

    IFARRAY(tq0);
    IFARRAY(tq1);
    IFARRAY(tq2);
    IFARRAY(tq3);
    IFARRAY(tq4);
    IFARRAY(tq5);

    if (pti->fBitfield != 0) caux++;
    

    for (iaux= 0; iaux < caux; iaux++)
	    if (iaux == 0)
		iauxret = st_auxadd (*st_paux_ifd_iaux (ifd, psym->index+iaux));
	    else
		st_auxadd (*st_paux_ifd_iaux (ifd, psym->index+iaux));

    return (iauxret);
} /* st_iaux_copyty */


export void st_changeaux (iaux, aux)

long	iaux;
AUXU	aux;

{
    pAUXU paux;

    /* change the iaux auxiliary entry in the current file to aux */
    paux = st_paux_iaux (iaux);
    *paux = aux;
} /* st_changeaux */


export void st_addtq (iaux, tq)

long	iaux;
long	tq;

{
    pAUXU	paux;

    paux = st_paux_iaux (iaux);
    /* set the lowest available tq */
    if (paux->ti.tq0 == tqNil)
	    paux->ti.tq0 = tq;
    else if (paux->ti.tq1 == tqNil)
	    paux->ti.tq1 = tq;
    else if (paux->ti.tq2 == tqNil)
	    paux->ti.tq2 = tq;
    else if (paux->ti.tq3 == tqNil)
	    paux->ti.tq3 = tq;
    else if (paux->ti.tq4 == tqNil)
	    paux->ti.tq4 = tq;
    else if (paux->ti.tq5 == tqNil)
	    paux->ti.tq5 = tq;
} /* st_addtq */



export void st_changeauxrndx (iaux, ifd, index)

long iaux;	/* index to be replaced */
long ifd;
long index;

{
    AUXU	aux;
    RNDXR	rndx;
    pAUXU paux;

    /* change the iaux auxiliary entry in the current file to aux */
    paux = st_paux_iaux (iaux);

    if(paux->rndx.rfd < ST_RFDESCAPE && ifd >= ST_RFDESCAPE)
	st_internal("tried to replace rndx aux (%d) that fits into one word (%d, %d) with one that can't (%d,%d)\n", iaux, paux->rndx.rfd, paux->rndx.index, ifd, index);

    if (paux->rndx.rfd < ST_RFDESCAPE) {
	paux->rndx.rfd = ifd;
    } else {
	paux[1].isym = ifd;
    } /* if */
    paux->rndx.index = index;
} /* st_changeauxrndx */


export long st_auxbtadd (bt)

long	bt;

{
    AUXU	aux;

    /* scalar interface to add a TIR aux with just the bt specified to
     * the auxiliary table
     */

    bzero (&aux, cbAUXU);
    aux.ti.bt = bt;
    return (st_auxadd (aux));
} /* st_auxbtadd */


export long st_auxisymadd (isym)

long isym;

{
    /* scalar interface to add a isym aux to the auxiliary table
     */

    AUXU	aux;

    aux.isym = isym;
    return (st_auxadd (aux));
} /* st_auxisymadd */



export long st_auxrndxadd (ifd, index)

long ifd;
long index;

{
    /* scalar interface to add a RNDX aux to the auxiliary table
     */

    RNDXR	rndx;
    AUXU	aux;
    int		iaux;

    rndx.rfd = ST_RFDESCAPE;
    rndx.index = index;
    aux.rndx = rndx;
    iaux = st_auxadd (aux);
    st_auxisymadd(ifd);
    return iaux;
} /* st_auxrndxadd */


export long st_auxbtsize (iaux, width)

{
    /* this routines sets the bit that says the basic type has a non-default
     *	width and then the auxiliary.
     */
    st_paux_iaux (iaux)->ti.fBitfield = 1;
    return (st_auxisymadd(width));
} /* st_auxbtsize */


export long st_auxrndxadd_idn (idn)

int idn;

{
    /* add the the number table entry pointed to by idn to the auxiliary
     *	table.
     */

    pDNR	pdn;

    pdn = st_pdn_idn(idn);
    return st_auxrndxadd(pdn->rfd, pdn->index);
} /* st_auxrndxadd_idn */

export void st_addcontinued(iaux)
long iaux;
{
    pAUXU	paux, st_paux_iaux();

    paux = st_paux_iaux(iaux);
    paux->ti.continued = 1;
} /* st_addcontinued */
