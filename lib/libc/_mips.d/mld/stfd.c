/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: stfd.c,v 2010.2.1.3 89/11/29 14:28:38 bettina Exp $ */
/*
 * Author	Mark I. Himelstein
 * Date started 5/10/85
 * Module	stfd.c
 * Purpose	provide a set of routine to interface front-ends to
 *		file descriptors and their constituent objects, which
 *		help to build up the symbol table.
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

extern pCHDRR	st_pchdr;		/* current chdr */
static pCFDR	pcfdcur;		/* current cfd entry in use */
static char messg_checkinit[] =
	"routine: you didn't initialize with st_cuinit or st_readst\n";
static char messg_checkinit2[] =
	"routine: no current routine, see fdadd or setfd\n";
static char messg_checkadd[] =
	"routine: cannot add to this entry it was readin from disk\n";
	
#define CHECKINIT(routine) \
    if (st_pchdr->pcfd == cfdNil)  \
	st_internal (messg_checkinit);

#define CHECKINIT2(routine) \
    CHECKINIT(routine); \
    if (pcfdcur == cfdNil)  \
	st_internal (messg_checkinit2);

#define CHECKADD(routine,mask) \
    CHECKINIT2(routine); \
    if ((pcfdcur->freadin&mask) != 0)  \
	st_internal (messg_checkadd);

/* FILE routines */

export long st_currentifd ()
{
    /* return the current ifd, although here it's the same as cfd */
    if (pcfdcur == cfdNil)
	return (-1);
    return (st_ifd_pcfd (pcfdcur));
} /* st_currentifd */


export long st_ifdmax ()
{
    /* return the number of files we are keeping track of */
    return (st_pchdr->cfd);
} /* st_ifdmax */


export void st_setfd (index)

long index;

{
    CHECKINIT(st_setfd);
    /* set the current file */
    pcfdcur = &st_pchdr->pcfd[index];
} /* st_setfd */


/* ADD routines */

export void st_fdadd (filename, lang, merge, glevel)

char	*filename;

{
    long		max;
    register long	oldmax;
    static FDR		fdZero;
    static CFDR		cfdZero;

    /* add a file descriptor allocating a set amount for syms aux's and
     * pointers to string tables. Also make it the current file.
     */
    if (st_pchdr->cfd  >= ST_EXTIFD)
	st_error ("st_fdadd: number of files (%d) exceeds max (%d)\n",
	    st_pchdr->cfd, ST_EXTIFD);

    if (st_pchdr->cfd  >= st_pchdr->cfdMax) {
	/* more than current max so we reallocate & relink */
	oldmax = max = st_pchdr->cfdMax;
	st_pchdr->pcfd = (pCFDR) st_malloc (st_pchdr->pcfd, &st_pchdr->cfdMax, 
	    cbCFDR, ST_FILESINIT);
	st_pchdr->pfd = (pFDR) st_malloc (st_pchdr->pfd, &max, 
	    cbFDR, ST_FILESINIT);
	if (max != st_pchdr->cfdMax)
	    st_internal ("st_fdadd: allocation botch (%d fds and %d cfds) in %s\n",
		max, st_pchdr->cfdMax, filename);
	while (oldmax--) {
	    st_pchdr->pcfd[oldmax].pfd = st_pchdr->pfd + oldmax;
	} /* while */
    } /* if */

    pcfdcur = &st_pchdr->pcfd[st_pchdr->cfd];
    *pcfdcur = cfdZero;
    pcfdcur->pfd = &st_pchdr->pfd [st_pchdr->cfd++];
    *pcfdcur->pfd = fdZero;
    pcfdcur->pfd->lang = lang;
    pcfdcur->pfd->fMerge = merge;
    pcfdcur->pfd->glevel = glevel;
    pcfdcur->pfd->fBigendian = (gethostsex() == BIGENDIAN);

    /* set up the null string and name for this file (always at one) */
    st_stradd ("");
    pcfdcur->pfd->rss = st_stradd (filename);

} /* st_fdadd */


export long st_symadd (iss, value, st, sc, index)

long	iss;
long	value;
long	st;
long	sc;
long	index;

{
    /* add a symbol to the current file's symbol table filling in the
     *	fields with the arguments and returning the index to this symbol.
     */
    pSYMR	psym;

    CHECKADD(st_symadd,ST_PSYMS);
    if (pcfdcur->pfd->csym >= pcfdcur->csymMax)
	pcfdcur->psym = (pSYMR) st_malloc((char *)pcfdcur->psym, 
	    &pcfdcur->csymMax, cbSYMR, ST_SYMINIT);

    psym = &pcfdcur->psym[pcfdcur->pfd->csym];
    psym->iss = iss;
    psym->value = value;
    psym->st = st;
    psym->sc = sc;
    psym->reserved = 0;
    psym->index = index;

    /* if a static has been added note this in the file descriptor so that
     *	it isn't merged.
     */
    if (st == stStaticProc || st == stStatic || st == stLabel || st == stProc)
	pcfdcur->pfd->fMerge = 0;
    
    return (pcfdcur->pfd->csym++);

} /* st_symadd */


export long st_auxadd (aux)

AUXU	aux;

{
    /* add an auxiliary entry and return its index */

    CHECKADD(st_auxadd,ST_PAUXS);
    if (pcfdcur->pfd->caux >= pcfdcur->cauxMax)
	pcfdcur->paux = (pAUXU) st_malloc((char *)pcfdcur->paux, 
	    &pcfdcur->cauxMax, cbAUXU, ST_AUXINIT);
	
    pcfdcur->paux[pcfdcur->pfd->caux] = aux;

    return (pcfdcur->pfd->caux++);

} /* st_auxadd */


export long st_pdadd (isym)

long	isym;

{
    pPDR	ppd;
    static PDR	pdzero;
    /* add an auxiliary entry and return its index */

    CHECKADD(st_pdadd,ST_PPDS);
    if (pcfdcur->pfd->cpd >= pcfdcur->cpdMax)
	pcfdcur->ppd = (pPDR) st_malloc((char *)pcfdcur->ppd, 
	    &pcfdcur->cpdMax, cbPDR, ST_PDINIT);
	
    pcfdcur->ppd[pcfdcur->pfd->cpd] = pdzero;
    pcfdcur->ppd[pcfdcur->pfd->cpd].isym = isym;

    return (pcfdcur->pfd->cpd++);

} /* st_pdadd */


export long st_lineadd (lineno)

long	lineno;

{
    /* add an auxiliary entry and return its index */

    CHECKADD(st_lineadd,ST_PLINES);
    if (pcfdcur->pfd->cline >= pcfdcur->clineMax)
	pcfdcur->pline = (pLINER) st_malloc((char *)pcfdcur->pline, 
	    &pcfdcur->clineMax, cbLINER, ST_LINEINIT);
	
    pcfdcur->pline[pcfdcur->pfd->cline] = lineno;

    return (pcfdcur->pfd->cline++);

} /* st_lineadd */


export long st_stradd( cp )			

register char *cp;

{
    /* place string into permanent string storage. each file has its
     *	own set of string tables. If there's no room left in the
     *	current one, allocate another. Return an index from the
     *	beginning of the current file string tables for the string.
     */
    register long	len;
    long			iss;

    if (cp == 0)
	st_error("st_stradd: argument is nil\n");

    len = strlen (cp) + 1;

    CHECKADD(st_stradd,ST_PSSS);
    if (len + pcfdcur->pfd->cbSs > pcfdcur->cbssMax)
	pcfdcur->pss = (char *) st_malloc((char *)pcfdcur->pss, 
	    &pcfdcur->cbssMax, sizeof(char), ST_STRTABINIT);
    
    strcpy (&pcfdcur->pss[pcfdcur->pfd->cbSs], cp);
    iss = pcfdcur->pfd->cbSs;
    pcfdcur->pfd->cbSs += len;
    return (iss);
} /* st_stradd */



/* ACCESS routines */


export pSYMR st_psym_ifd_isym (ifd, isym)

long	ifd;
long	isym;
{
    CHECKINIT(ST_psym_ifd_isym);
    /* given an index into the file descriptor table and a symbol index
     * return a pointer to the symbol it refers to
     */
    if (ifd == ST_EXTIFD)
	return (&st_pext_iext(isym)->asym);
    if (ifd < 0 || isym < 0 || ifd >= st_pchdr->cfd || isym >= st_pchdr->pcfd[ifd].pfd->csym)
	st_internal ("st_psym_ifd_isym: ifd (%d) or isym (%d) out of range\n",
	    ifd, isym);
    return (&st_pchdr->pcfd[ifd].psym[isym]);
} /* st_psym_ifd_isym */


export pAUXU st_paux_ifd_iaux (ifd, iaux)

long	ifd;
long	iaux;

{
    CHECKINIT(st_paux_ifd_iaux);
    /* return an aux entry for file ifd, index iaux */
    if (ifd < 0 || iaux < 0 || ifd >= st_pchdr->cfd || iaux >= st_pchdr->pcfd[ifd].pfd->caux)
	st_internal ("st_paux_ifd_iaux: ifd (%d) or iaux (%d) out of range\n",
	    ifd, iaux);
    return (&st_pchdr->pcfd[ifd].paux[iaux]);
} /* st_aux_iaux */


export pLINER st_pline_ifd_iline (ifd, iline)

long	ifd;
long	iline;

{
    CHECKINIT(st_pline_ifd_iline);
    /* return an aux entry for file ifd, index iaux */
    if (ifd < 0 || iline < 0 || ifd >= st_pchdr->cfd || iline >= st_pchdr->pcfd[ifd].pfd->cline)
	st_internal ("st_paux_ifd_iaux: ifd (%d) or iline (%d) out of range\n",
	    ifd, iline);
    return (&st_pchdr->pcfd[ifd].pline[iline]);
} /* st_pline_ifd_iline */


export pAUXU st_paux_iaux (iaux)

long	iaux;

{
    CHECKINIT2(st_paux_iaux);
    /* return the iaux entry in the current files auxiliary table */
    if (iaux < 0 || iaux >= pcfdcur->pfd->caux)
	st_internal ("st_paux_iaux: iaux (%d) out of range\n", iaux);
    return (&pcfdcur->paux[iaux]);
} /* st_paux_ifd_iaux */


export pPDR st_ppd_ifd_isym (ifd, isym)

long	ifd;
long	isym;

{
    pCFDR	pcfd1;
    long	ipd;

    /* return a pointer to the procedure descriptor entry whose isym is
     *	equal to the isym argument.
     */
    CHECKINIT(st_ppd_ifd_isym);
    pcfd1 = st_pcfd_ifd (ifd);
    for (ipd = 0; ipd < pcfd1->pfd->cpd; ipd++)
	if (pcfd1->ppd[ipd].isym == isym)
	    return (pcfd1->ppd + ipd);

    return (pdNil);
} /* st_ppd_ifd_isym */


export long st_ifd_pcfd (pcfd1)

pCFDR	pcfd1;

{
    CHECKINIT2(st_ifd_pcfd);
    /* return an ifd given a pcfdcur */
    return (pcfd1 - st_pchdr->pcfd);
} /* st_ifd_pcfd */


export pCFDR st_pcfd_ifd (ifd)

long	ifd;

{
    CHECKINIT(st_pcfd_ifd);
    /* return a pcfd gven an ifd */
    if (ifd < 0 || ifd >= st_pchdr->cfd)
	st_internal ("st_pcfd_ifd: ifd (%d) out of range\n", ifd);
    return (&st_pchdr->pcfd [ifd]);
} /* st_pcfd_ifd */


export char *st_str_iss (iss)

{
    /* return a string pointer given an index into string space for the current
     *  file. return 0 if the iss is invalid.
     */
    CHECKINIT2(st_str_iss);
    if (pcfdcur->pfd->cbSs && iss < pcfdcur->pfd->cbSs)
	return (&pcfdcur->pss[iss]);
    return ((char *) 0);
} /* st_str_iss */


export char *st_str_ifd_iss (ifd, iss)

{
    register pCFDR lpcfd;
  
    /* return a string pointer given a file index and 
     * an index into string space. If it's an external file, call the extstr
     *	routine. If the iss is out of bounds return 0.
     */


    CHECKINIT(st_str_ifd_iss);
    if (ifd == ST_EXTIFD)
	return (st_str_extiss (iss));
	
    lpcfd = st_pcfd_ifd(ifd);

    if (lpcfd->pfd->cbSs && iss < lpcfd->pfd->cbSs)
	return (&lpcfd->pss[iss]);
    return ((char *) 0);
} /* st_str_ifd_iss */


/* MISCELLANEOUS routines */

export char * st_malloc (ptr,size,itemsize,basesize)

char	*ptr;		/* old pointer */
long	*size;		/* current size, will be modified to new size */
long	itemsize;	/* size of the item being allocated */
long	basesize;	/* initial number to be allocated */

{
    /* generic allocation routine */
    if (*size == 0 || !ptr || ptr == (char *) -1) {
	*size = basesize;
	ptr = (char *) malloc (*size * itemsize);
	if (ptr == (char *)0)
	    st_error ("st_malloc: cannot allocate item\n");
    } else {
	*size *= 2;
	ptr = (char *) realloc (ptr, *size * itemsize);
	if (ptr == (char *)0)
	    st_error ("st_malloc: cannot grow item\n");
    } /* if */

    return (ptr);
} /* st_malloc */

