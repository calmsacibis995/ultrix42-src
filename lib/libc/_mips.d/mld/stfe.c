/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: stfe.c,v 2010.2.1.3 89/11/29 14:28:44 bettina Exp $ */
/*
 * Author	Mark I. Himelstein
 * Date started 5/10/85
 * Module	stfe.c
 * Purpose	provide a set of routine to interface to front-ends, which
 *		will be used to build up the symbol table. These routines
 *		are really second level and combine the basic routines
 *		to provide higher level functions.
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
#include "cmplrs/usys.h"
#include "cmplrs/ucode.h"

#define ST_NESTMAX	20

static DNR *adn_stack;			/* stack of begin blocks */
static long idn_stack;			/* counter for the stack */
static long dn_stackmax;		/* counter for the stack */
static DNR *filestack;			/* stack of files */
static long ifilestack;			/* counter for stack of files */
static long filestackmax;		/* counter for max stack of files */


export long st_filebegin (filename, lang, merge, glevel)

char	*filename;

{
    /*
     * This routine takes afilename and return a dense number for the
     *	file. 
     * If it didn't exist before, it is created and made the
     *	current file and pushed on the file stack. 
     * If it did exist,
     *	and it is closed off with the appropriate stEnd local symbol
     *	a new one is created as above.
     * If it did exist and is not closed off,
     *	we need to find it on the filestack, we pop off and end all files
     *	above the file we are looking for on the stack.
     *
     * Regardless we always return the dense number for the stFile symbol.
     */
    long ifd;


    /* just search through the files. If we already have an entry
     *	return it, otherwise return a new one.
     */
    for (ifd = 0; ifd < st_ifdmax(); ifd++) {

	if (!strcmp(filename, st_str_ifd_iss (ifd, ST_FDISS))) {

	    int		i;

	    /* if it's not in the current filestack, it's not it */
	    for (i = ifilestack - 1; i >= 0; i--) {
		if (ifd == filestack[i].rfd)
		    break;
	    } /* for */
	    if (i < 0)
		continue;

	    while (ifd != filestack[ifilestack-1].rfd) {
		if (--ifilestack < 0)
		    st_internal ("st_filebegin: tried to end too many files (%s)\n",
			filename);
	    } /* while */
	    st_setfd (ifd);
	    return (filestack[ifilestack-1].index);

	} /* if */
    } /* for */

    st_fdadd (filename, lang, merge, glevel);
    st_feinit();
    /* add to the filestack which keeps track of CPP style line/file
     *	announcements and figures out when to remove a file.
     */
    if (ifilestack + 1 > filestackmax)
	filestack = (pDNR) st_malloc (filestack, &filestackmax, cbDNR,
		ST_FILESINIT);
    filestack[ifilestack].rfd = st_currentifd();
    filestack[ifilestack].index = st_idn_index_fext (st_symadd(ST_FDISS, 
	valueNil, stFile, scText, indexNil), 0);
    return (filestack[ifilestack++].index);
}

export void st_endallfiles()

{
    pSYMR	psym;
    pCFDR	pcfd;
    long	ifd;
    long	old_ifd;
    long	temp;

    old_ifd = st_currentifd();

    /* ends all files from the filestack */
    for (ifd = 0; ifd < st_ifdmax(); ifd++) {

	pcfd = st_pcfd_ifd (ifd);
	if (pcfd->psym != symNil && pcfd->psym != (pSYMR)-1 &&
	    pcfd->pfd->csym > 0 &&
	    pcfd->psym[pcfd->pfd->csym-1].st == stEnd &&
	    pcfd->psym[pcfd->pfd->csym-1].index == 0)
	    /* this file is closed */
	    continue;

	st_setfd (ifd);
	psym = st_psym_ifd_isym (ifd, 0);
	temp = st_symadd(psym->iss, valueNil, stEnd, scText, 0);
	psym = st_psym_ifd_isym (ifd, 0);
	psym->index = ++temp;
    } /* for */
    st_setfd (old_ifd);
} /* st_endallfiles */


export long st_fileend (idn)

long idn;

{
    /* we usually figure out we need to end a file after we've already
     * started another so this routine first sets up the file we're
     * ending as the current file, get the symbol for the beginning
     * of file and add's the end of file symbol. The indexes are updated
     * appropriately.
     */
    DNR		dn;
    pSYMR	psym;
    pCFDR	pcfd;
    long		ifd;

    dn = *st_pdn_idn (idn);
    pcfd = st_pcfd_ifd (dn.rfd);
    if (pcfd->psym != symNil && pcfd->psym != (pSYMR)-1 &&
	pcfd->pfd->csym > 0 &&
	pcfd->psym[pcfd->pfd->csym-1].st == stEnd &&
	pcfd->psym[pcfd->pfd->csym-1].index == 0)
	/* this file is closed */
	return -1;
    ifd = st_currentifd();
    st_setfd (dn.rfd);
    psym = st_psym_ifd_isym (dn.rfd, dn.index);
    dn.index = psym->index = st_symadd(psym->iss, valueNil, stEnd, scText, 0);
    psym->index++;
    st_setfd (ifd);
    return (st_idn_dn (dn));
} /* st_fileend */


static st_fblockpending = 0;	/* set if we just got a nested text block */

/* Algorithm for adding textblocks:
 *
 *	when the begin block or left brace is encountered:
 *
 *	idn = st_blockbegin (issNull, 0, scText);
 *	if (idn != 0)
 *		generate u-code begin block;
 *	endif
 *
 *	if the block contains local declarations then before the first one:
 *	idn = st_textblock();
 *	if (idn != 0)
 *		generate u-code begin block;
 *	endif
 *
 *	when an end of block is encountered:
 *	idn = st_endblock();
 *	if (idn != 0)
 *		generate u-code end block;
 *	endif
 */

export long st_textblock()


{
    /* this routine is used to begins nested language blocks.
     * if a block is oending we add the symbol and update the block stack.
     */
    long	index;
    DNR		dn;

    if (st_fblockpending && idn_stack > 1) {
	st_fblockpending = 0;
	index = st_symadd (0, 0, stBlock, scText, 0);
	dn.rfd = st_currentifd();
	dn.index = index;
	adn_stack [idn_stack - 1] = dn;
	return (st_idn_dn (dn));
    } else if (st_fblockpending) {
	st_internal ("st_textblock: block pending set in illegal case\n");
    } /* if */
    return (0);
} /* st_blockbegin */


export long st_blockbegin(iss, value, sc)

long	iss;
long	value;
long	sc;

{
    /* this routine is used to begin structures/etc and language blocks.
     *	we add the symbol and store an dn for it in the block stack.
     *	we return a dense number referencing the begin block symbol.
     * If it's a nested textblock we only note the begin on the block
     *	stack and a subsequent st_textblock will make the actual symbol.
     */
    long index;
    DNR	dn;

    if (sc == scText && idn_stack != 0) {
	st_fblockpending = 1;
	index =  ST_ANONINDEX;
    } else {
	index = st_symadd (iss, value, stBlock, sc,  0);
    } /* if */
    dn.rfd = st_currentifd();
    dn.index = index;
    if (idn_stack + 1 > dn_stackmax)
	adn_stack = (pDNR) st_malloc (adn_stack, &dn_stackmax, cbDNR,
		ST_FILESINIT);
    adn_stack [idn_stack++] = dn;
    if (index == ST_ANONINDEX)
	return (0);
    return (st_idn_dn (dn));
} /* st_blockbegin */


export long st_blockend(size)

long	size;

{
    /* this routine is used to etc structures/etc and language blocks.
     * we get the corresponding begin block symbol, add the end block
     *	symbol and fix the indexes. If it's scText we must emit the
     *	end block ucode otherwise we must update the size in the
     *	begin block for structures/etc cause we only know the size at
     *	this point.
     * If it's a text block and a st_block symbol was never emitted then
     *	a zero is returned.
     */
    pSYMR	psym;
    DNR	dn;
    long	index;
    long	tempifd;

    dn = adn_stack [--idn_stack];
    if (dn.index == ST_ANONINDEX) {
	st_fblockpending = 0;
	return (0);
    } /* if */

    psym = st_psym_ifd_isym (dn.rfd, dn.index);
    tempifd = st_currentifd();
    st_setfd(dn.rfd);
    index = st_symadd (issNull, valueNil, stEnd, psym->sc, 
	adn_stack [idn_stack].index) + 1;
    /* need to refresh pointer, that may have been moved by symadd */
    psym = st_psym_ifd_isym (dn.rfd, dn.index);
    psym->index = index;
    if (psym->sc != scText)
	/* it's a struct/etc. and we can only fill this in now */
	psym->value = size;
    index = st_idn_index_fext (psym->index - 1, 0);
    st_setfd(tempifd);
    return (index);
} /* st_blockend */


export void st_blockpop ()

{
    /* we decided to pop the block instead of making an end for it */
    --idn_stack;
} /* st_blockpop */


export long st_procend(idn)

long	idn;

{
	/* the proc's block no. is the idn (our arg), which points to the
	 *	external symbol whose index points to the local symbol or
	 *	it's a static proic and the idn points at the local symbol.
	 *	So we get the local symbol and update the end symbol reference
	 *	residing in the auxiliary entry to point at the stEnd local
	 *	symbol we emit. The stEnd will point back the local symbol
	 *	that begins the proc and a dense number to the stEnd is
	 *	returned.
	 */
	pEXTR	pext;
	pSYMR	psym;
	pAUXU	paux;
	DNR	dn;
	long	index;
	long	isextern;
	long	tempifd;

	dn = *st_pdn_idn (idn);
	/* get the local symbol even if we have to go through the external */
	if (dn.rfd == ST_EXTIFD) {
	    isextern = 1;
	    pext = st_pext_iext (dn.index);
	    psym = st_psym_ifd_isym (pext->ifd, pext->asym.index);
	    dn.rfd = pext->ifd;
	    dn.index = index = pext->asym.index;
	} else {
	    isextern = 0;
	    psym = st_psym_ifd_isym (dn.rfd, dn.index);
	    index = dn.index;
	} /* if */

	/* add the stEnd and change the auxiliary */
	tempifd = st_currentifd();
	st_setfd(dn.rfd);
	index = st_symadd (psym->iss, valueNil, stEnd, scText, index);
	/* refresh pointer after symadd */
	psym = st_psym_ifd_isym (dn.rfd, dn.index);
	if (st_pcfd_ifd(st_currentifd())->pfd->caux >= psym->index &&
	    psym->index != indexNil) {
	    paux = st_paux_iaux (psym->index);
	    paux->isym = index + 1;
	} /* if */

	if (isextern == 1 && pext->asym.st == stStaticProc) {

	    /* make sure that static procs dense numbers point to the
	     * local symbols-- I guess this is a good time to do this!!
	     */
	    long	idnnew;
	    idnnew = st_idn_index_fext (pext->asym.index, 0);
	    st_setidn (idn, idnnew);
	    pext->asym.sc = scNil;
	} /* if */

	idn = st_idn_index_fext (index, 0);
	st_setfd(tempifd);
	return(idn);
} /* st_procend */


export long st_procbegin (idn)

{
	/* this routine assumes that an external symbol has been added for
	 *	for the external referenced by our argument.
	 *	we make sure that the name is in this files string space,
	 *	copy the type if necessary and
	 *	then we create the local symbol for it in the symbol table
	 *	and patch some symbol fields in the external.
	 */
	long	iss;
	pEXTR	pext;
	pCFDR	pcfd;
	DNR	dn;

	dn = *st_pdn_idn (idn);
	if (dn.rfd != ST_EXTIFD)
	    return (idn);
	pext = st_pext_iext (dn.index);
	if (pext->asym.sc == scText)
		return (idn);

	pext->asym.sc = scText;
	if (pext->ifd != st_currentifd()) {
		/* we have to make a copy of the name of the string in this
		 */
		if (st_pcfd_ifd(st_currentifd())->pfd->caux > 0 &&
		    pext->asym.index != indexNil)
		    pext->asym.index = st_iaux_copyty (pext->ifd, &pext->asym);
		pext->ifd = st_currentifd ();
	} /* if */
	iss = st_stradd (st_str_extiss (pext->asym.iss));
	pext->asym.index = st_symadd (iss, valueNil, pext->asym.st, 
		scText, pext->asym.index);
	st_pcfd_ifd (pext->ifd)->pfd->fMerge = 0; /* we got real code here */
	return (idn);
} /* st_procbegin */


export char *st_sym_idn (idn,sc,st,value,index)

long idn;
long *sc;
long *st;
long *value;
long *index;

{
	DNR dn;
	pSYMR psym;

	dn = *st_pdn_idn (idn);
	/* this is to help support anonymous names, i.e. an index
	 *	of ST_ANONINDEX in a dense number table entry says that the
	 *	this is a dummy dense number for anonymous name.
	 */
	if (dn.index == ST_ANONINDEX)
	    return ((char *)-1);
	psym = st_psym_ifd_isym (dn.rfd, dn.index);
	*sc = psym->sc;
	*st = psym->st;
	*value = psym->value;
	*index = psym->index;
	return (st_str_ifd_iss (dn.rfd, psym->iss));

} /* st_sym_idn */




export char *st_str_idn (idn)

long idn;

{
	DNR dn;
	pSYMR psym;

	dn = *st_pdn_idn (idn);
	/* this is to help support anonymous names, i.e. an index
	 *	of ST_ANONINDEX in a dense number table entry says that the
	 *	this is a dummy dense number for anonymous name.
	 */
	if (dn.index == ST_ANONINDEX)
	    return ((char *)-1);
	psym = st_psym_ifd_isym (dn.rfd, dn.index);
	return (st_str_ifd_iss (dn.rfd, psym->iss));

} /* st_str_idn */


export long st_fglobal_idn (idn)

long idn;

{
	DNR dn;
	pSYMR psym;

	dn = *st_pdn_idn (idn);
	/* this is to help support anonymous names, i.e. an index
	 *	of ST_ANONINDEX in a dense number table entry says that the
	 *	this is a dummy dense number for anonymous name.
	 */
	if (dn.index == ST_ANONINDEX)
	    return (0);
	psym = st_psym_ifd_isym (dn.rfd, dn.index);
	/* jkh 3/21/86 added check for stStaticProc */
	return ((psym->st != stStatic) && (psym->st != stStaticProc));

} /* st_fglobal_idn */


export pSYMR st_psym_idn_offset (idn, offset)

long	idn;
long	offset;

{
    DNR	dn;
    pSYMR	psym;
    long	iend;
    pAUXU	paux;
    pEXTR	pext;

    /* this routine returns a pointer to a stParam or stLocal local symbol
     *	encase by the procedure specified by idn and being offset from
     *	the virtual frame pointer (the value field)
     */
    dn = *st_pdn_idn (idn);
    if (dn.rfd == ST_EXTIFD) {
	/* check if it's external */
	pext = st_pext_iext (dn.index);
	dn.rfd = pext->ifd;
	dn.index = pext->asym.index;
    } /* if */

    /* get the index of the last symbol to check */
    psym = st_psym_ifd_isym (dn.rfd, dn.index);
    if (psym->index == indexNil)
	return ((pSYMR) 0);
    paux = st_paux_ifd_iaux(dn.rfd, psym->index);
    if (paux == auxNil)
	return ((pSYMR) 0);
    iend = paux->isym;

    while (++dn.index < iend) {
	psym = st_psym_ifd_isym (dn.rfd, dn.index);
	if ((psym->st == stLocal || psym->st == stParam) && 
	    (psym->sc == scAbs || psym->sc == scVar) &&
	    psym->value == offset)
	    return (psym);
	else if (psym->st == stProc || psym->st == stStaticProc)
	    break;
	if (psym->sc == scInfo && psym->st == stBlock)
	    dn.index = psym->index - 1;
    } /* while */
    return ((pSYMR) 0);
} /* st_psym_idn_offset */


export void st_fixextindex(idn, index)

long idn;	/* from dense number, find ext table index */
long index;

{
    pEXTR	pext;
    DNR	dn;

    dn = *st_pdn_idn(idn);
    pext = st_pext_iext (dn.index);
    pext->ifd = st_currentifd ();
    pext->asym.index = index;
    pext->asym.sc = scText;
} /* st_fixext */




export void st_fixextsc(idn, sc)

long idn;	/* from dense number, find ext table index */
long sc;

{
    pEXTR	pext;
    DNR	dn;

    dn = *st_pdn_idn(idn);
    pext = st_pext_iext(dn.index);
    pext->asym.sc = sc;
} /* st_fixext */




export long st_pdadd_idn  (idn)

long idn;

{
    /* this routine will add a pd and get the isym from the dense number */

    pDNR	pdn;
    pEXTR	pext;
    long	isym;

    pdn = st_pdn_idn (idn);
    if (pdn->rfd == ST_EXTIFD) {
	pext = st_pext_iext (pdn->index);
	isym = pext->asym.index;
    } else {
	isym = pdn->index;
    } /* if */

    return (st_pdadd(isym));
} /* st_pdadd_idn */


export st_fixiss (idn, iss)
long iss;
long idn;

{
    pSYMR	psym;
    pDNR	pdn;

    pdn = st_pdn_idn (idn);
    if (pdn->rfd == ST_EXTIFD) {
	psym = &(st_pext_iext (pdn->index)->asym);
    } else {
	psym = st_psym_ifd_isym (pdn->rfd, pdn->index);
    } /* if */

    psym->iss = iss;
} /* st_fixiss */


export st_changedn (idn, rfd, index)

{
    pDNR	pdn;

    pdn = st_pdn_idn (idn);
    pdn->rfd = rfd;
    pdn->index = index;
} /* st_changedn */
