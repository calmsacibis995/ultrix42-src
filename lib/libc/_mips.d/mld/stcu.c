/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: stcu.c,v 2010.3.1.3 89/11/29 14:28:42 bettina Exp $ */
/*
 * Author	Mark I. Himelstein
 * Date started 5/10/85
 * Module	stcu.c
 * Purpose	provide a set of routine to interface to front-ends, which
 *		will be used to build up the per compilation unit part
 *		of symbol table.
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

#include <sys/types.h>

#ifdef BSD
#include <sys/file.h>
#endif

#ifdef SYSV
#include <fcntl.h>
#define bzero(p,n) memset(p,0,n)
#endif

#include <stdio.h>
#include <errno.h>
#include "stamp.h"
#include "syms.h"
#include "stext.h"

extern int errno;

pCHDRR	st_pchdr;			/* current object we're dealing with */

#define pssExt pssext
#define cssExt cbssext

export pCHDRR st_cuinit ()

{
    /* create and intialize current chdr */
    st_pchdr = (pCHDRR) calloc (cbCHDRR, 1);
    if (!st_pchdr)
	st_error("st_cuinit: cannot allocate current chdr\n");
    st_pchdr->cdn = ST_IDNINIT;
    return (st_pchdr);
} /* st_cuinit */


export void st_setchdr(pchdr)

pCHDRR	pchdr;
{
    /* set the current chdr */
    st_pchdr = pchdr;
};


export pCHDRR st_currentpchdr()
{
    /* return pointer to current chdr */
    return st_pchdr;
}

export void st_free ()

{
#define FREE(x) if (x && ((int) x) != -1) free (x);

    FREE(st_pchdr->pdn);
    FREE(st_pchdr->pext);
    FREE(st_pchdr->pssext);
    FREE(st_pchdr->pfd);
    FREE(st_pchdr->psym);
    FREE(st_pchdr->paux);
    FREE(st_pchdr->popt);
    FREE(st_pchdr->ppd);
    FREE(st_pchdr->pline);
    FREE(st_pchdr->pss);
    FREE(st_pchdr->prfd);

    bzero (st_pchdr, cbHDRR);
} /* st_free */


export long st_extadd (iss, value, st, sc, index)

long	iss;
long	value;
long	st;
long	sc;
long	index;

{
    /* add an external to the externals table with the information
     *	passed in. Also allocate or grow the table as needed.
     */
    pEXTR	pext;

    if (!st_pchdr)
	st_internal("st_extadd: you didn't initialize with cuinit or readst\n");
    if (st_pchdr->cext >= st_pchdr->cextMax) 
	st_pchdr->pext = (pEXTR) st_malloc ((char *)st_pchdr->pext, 
	    &st_pchdr->cextMax, cbEXTR, ST_EXTINIT);
	
    pext = &st_pchdr->pext[st_pchdr->cext];
    pext->ifd = st_currentifd();
    pext->jmptbl = 0;
    pext->cobol_main = 0;
    pext->reserved = 0;
    pext->asym.iss =  iss;
    pext->asym.value = value;
    pext->asym.st = st;
    pext->asym.sc = sc;
    pext->asym.reserved = 0;
    pext->asym.index = index;

    /* if this allocates space in the current file then we cannot 
     *	merge this file come load time.
     */
    if (sc != scSCommon && sc != scCommon && sc != scNil && sc != scUndefined
	&& sc != scSUndefined)
	st_pcfd_ifd (pext->ifd)->pfd->fMerge = 0;
    return (st_pchdr->cext++);

} /* st_extadd */


export long st_extstradd(cp)			

register char *cp;

{
    /* place string into permanent external string storage. 
     *	Return an index into the external string space for that string.
     * This routine also takes care of incremental allocation for the
     *	external string space.
     */
    register long	len;
    long			iss;

    if (!st_pchdr)
	st_internal("st_extstradd: you didn't initialize with cuinit or readst\n");
    if (cp == 0)
	st_error("st_extstradd: argument is nil\n");
    len = strlen (cp) + 1;
    while ((st_pchdr->cbssext + len) > st_pchdr->cbssextMax)
	st_pchdr->pssext = st_malloc (st_pchdr->pssext, &st_pchdr->cbssextMax,
	    1, ST_STRTABINIT);

    strcpy (&st_pchdr->pssext[st_pchdr->cbssext], cp);
    iss = st_pchdr->cbssext;
    st_pchdr->cbssext += len;
    return (iss);
} /* st_extstradd */


export char *st_str_extiss (iss)

long iss;

{
    /* return a string pointer given an index into external string space 
     *  Only thing we must do is check to see if it's in bounds
     */
    if (iss >= 0 && iss < st_pchdr->cbssext)
	return (&st_pchdr->pssext[iss]);

    return ((char *) 0);
} /* st_str_extiss */


export long st_idn_index_fext (index, fext)

long fext;
long index;

{
    /* return a new dense number for the specified index and assume the current 
     * file. Allocate or grow  the table ass necessary.
     */
    DNR		dn;

    if (!st_pchdr)
	st_internal("st_idn_index_fext: you didn't initialize with cuinit or readst\n");
    if (st_pchdr->cdn >= st_pchdr->cdnMax) 
	st_pchdr->pdn = (pDNR) st_malloc ((char *)st_pchdr->pdn, 
	    &st_pchdr->cdnMax, cbDNR, ST_DNINIT);

    dn.index = index;
    if (fext)
	dn.rfd = ST_EXTIFD;
    else
	dn.rfd = st_currentifd();

    st_pchdr->pdn[st_pchdr->cdn] = dn;
    return (st_pchdr->cdn++);

} /* st_idn_index_fext */


export long st_idn_dn (dn)

DNR	dn;
{
    if (!st_pchdr)
	st_internal("st_idn_dn: you didn't initialize with cuinit or readst\n");
    /* return a new dense number for the specified index dn
     */
    if (st_pchdr->cdn >= st_pchdr->cdnMax) 
	st_pchdr->pdn = (pDNR) st_malloc ((char *)st_pchdr->pdn, 
	    &st_pchdr->cdnMax, cbDNR, ST_DNINIT);

    st_pchdr->pdn[st_pchdr->cdn].rfd = dn.rfd;
    st_pchdr->pdn[st_pchdr->cdn].index = dn.index;
    return (st_pchdr->cdn++);
} /* st_idn_dn */


export long st_idn_rndx (rndx)

RNDXR	rndx;
{
    if (!st_pchdr)
	st_internal("st_idn_rndx: you didn't initialize with cuinit or readst\n");
    /* return a new dense number for the specified index rndx
     */
    if (st_pchdr->cdn >= st_pchdr->cdnMax) 
	st_pchdr->pdn = (pDNR) st_malloc ((char *)st_pchdr->pdn, 
	    &st_pchdr->cdnMax, cbDNR, ST_DNINIT);

    st_pchdr->pdn[st_pchdr->cdn].rfd = rndx.rfd;
    st_pchdr->pdn[st_pchdr->cdn].index = rndx.index;
    return (st_pchdr->cdn++);
} /* st_idn_rndx */


export RNDXR st_rndx_idn (idn)

long	idn;
{
    RNDXR rndx;
    /* return an rndx given a dense number */
    if (idn >= st_pchdr->cdn)
	st_internal ("st_rndx_idn: idn (%d) greater than max (%d)\n", idn, 
	    st_pchdr->cdn);
    if (st_pchdr->pdn[idn].rfd >= ST_RFDESCAPE)
	st_internal ("st_rndx_idn: old interface can't put rfd(%d) into rndx, use st_pdn_idn instead\n",
	    st_pchdr->pdn[idn].rfd);
    rndx.rfd = st_pchdr->pdn[idn].rfd;
    rndx.index = st_pchdr->pdn[idn].index;
    return (rndx);
} /* st_idn_rndx */


export pDNR st_pdn_idn (idn)

long idn;

{
    if (idn < 0 || idn > st_pchdr->cdn)
	st_internal ("st_pdn_idn: idn (%d) less than 0 or greater than max (%d)\n",
	    idn, st_pchdr->cdn);
    return (&st_pchdr->pdn[idn]);
} /* st_pdn_idn */


export void st_setidn (idndest, idnsrc)

long idndest;
long idnsrc;

{
    /* should remove */
    if (idndest < 0 || idnsrc < 0 || idndest >= st_pchdr->cdn || 
	idnsrc >= st_pchdr->cdn)
	st_internal ("st_setidn: idnsrc (%d) or idndest (%d) out of range\n", 
	    idnsrc, idndest);
    st_pchdr->pdn[idndest] = st_pchdr->pdn[idnsrc];
} /* st_setidn */




export pEXTR st_pext_dn (dn)

DNR	dn;

{
    /* return a pointer to a symbol, given a dn */
    if (dn.rfd != ST_EXTIFD)
	st_internal ("st_pext_dn: rfd field (%d) isn't equal to ST_EXTIFD(%d)\n",
	    dn.rfd, ST_EXTIFD);
    if (dn.index < 0 || dn.index > st_pchdr->cext)
	st_internal ("st_pext_dn: index out of range (%d)\n", dn.index);
    return (&st_pchdr->pext[dn.index]);
} /* st_pext_rndx */


export pEXTR st_pext_iext (index)

long	index;

{
    /* return a pointer to a symbol, given a index */
    if (index < 0 || index > st_pchdr->cext)
	st_internal ("st_pext_iext: index out of range (%d)\n", index);
    return (&st_pchdr->pext[index]);
} /* st_pext_iext */


export long st_iextmax ()

{
    return (st_pchdr->cext);
} /* st_iextmax */


char *st_errname = "libmld";

st_setmsgname(name)
char *name;
{
    st_errname = (char *)malloc(strlen(name)+1);
    if (st_errname == 0) {
	fprintf(stderr, "libmld: Internal: cannot allocate to initialize component name for libmld errors\n");
	exit(1);
    } /* if */
    strcpy(st_errname, name);
}
