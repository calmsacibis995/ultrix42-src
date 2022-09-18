/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: stprint.c,v 2010.2.1.3 89/11/29 14:28:47 bettina Exp $ */
/*
 * Author	Mark I. Himelstein
 * Date started Wed May 22 10:16:36 PDT 1985
 * Module	stprint.c
 * Purpose	provide a set of routine to print out the symbol table in
 *		ascii.
 * Interfaces
 *	 void st_printsym (pfd, ext, psym)
 *
 * Naming conventions:
 *		- all function interfaces are prefixed with st_
 *		- functions returning a value from an arg have names of the form
 *				result_arg
 *		- all globals prefixed with a st_
 *		- all arrays start with an a
 *		- all pointers start with a p
 *		- an array mapping thing1 to thing to is named mthing1_thing2
 */

#include "syms.h"
#include "stext.h"
#include <stdio.h>


static long	st_cindent;
static long	fAux;

/* the following maps contain the ascii forms for constants in sym.h */


export char	*st_mlang_ascii [] = {
	"C",		/* langC		0 */
	"Pascal",	/* langPascal		1 */
	"Fortran",	/* langFortran		2 */
	"Assembler",	/* langAssembler	3 */
	"Machine",	/* langMachine		4 */
	"",		/* langNil		5 */
	"Ada",		/* langAda		6 */
	"Pl1",		/* langPl1		7 */
	"Cobol" 	/* langCobol		8 */
}; /* mlang_ascii */

export char	*st_mst_ascii [] = {

	"stNil",		/*  stNil	0 */
	"Global",		/*  stGlobal	1 */
	"Static",		/*  stStatic	2 */
	"Param",		/*  stParam	3 */
	"Local",		/*  stLocal	4 */
	"Label",		/*  stLabel	5 */
	"Proc",			/*  stProc 	6 */
	"Block",		/*  stBlock 	7 */
	"End",			/*  stEnd 	8 */
	"Member",		/*  stMember	9 */
	"Typdef",		/*  stTypedef	10 */
	"File",			/*  stFile 	11 */
	(char *)0,
	(char *)0,
	"StaticProc",		/*  stStaticProc 14 */
	"Constant",		/*  stConstant   15 */
	"StaParam"		/*  stStaParam   16 */
}; /* st_mst_ascii */

export char	*st_msc_ascii [] = {

	"scNil",		/* scNil	0 */
	"Text",			/* scText	1 */
	"Data",			/* scData	2 */
	"Bss",			/* scBss	3 */
	"Register",		/* scRegister	4 */
	"Abs",			/* scAbs	5 */
	"Undefined",		/* scUndefined	6 */
	"CdbLocal",		/* scCdbLocal	7 */
	"Bits",			/* scBits	8 */
	"CdbSystem",		/* scCdbSystem	9 */
	"RegImage",		/* scRegImage	10 */
	"Info",			/* scInfo	11 */
	"UserStruct",		/* scUserStruct	12 */
	"SData",		/* scSdata	13 */
	"SBss",			/* scSBss	14 */
	"RData",		/* scRdata	15 */
	"Var",			/* scVar	16 */
	"Common",		/* scCommon	17 */
	"SCommon",		/* scSCommon	18 */
	"VarRegister",		/* scVarRegister19 */
	"Variant",		/* scVariant	20 */
	"SUndefined",		/* scSUndefined	21 */
	"Init",			/* scInit	22 */
	"BasedVar",		/* scBasedVar	23 */
}; /* st_msc_ascii */

export char	*st_mbt_ascii [] = {
	"btNil",		/*  btNil	0 */
	"btAdr",		/*  btAdr	1 */
	"char",			/*  btChar	2 */
	"unsigned char",	/*  btUChar	3 */
	"short",		/*  btShort	4 */
	"unsigned short",	/*  btUShort	5 */
	"int",			/*  btInt	6 */
	"unsigned int",		/*  btUInt	7 */
	"long",			/*  btLong	8 */
	"unsigned long",	/*  btULong	9 */
	"float",		/*  btFloat	10 */
	"double",		/*  btDouble	11 */
	"struct",		/*  btStruct	12 */
	"union",		/*  btUnion	13 */
	"enum",			/*  btEnum	14 */
	"typedef",		/*  btTypedef	15 */
	"range",		/*  btRange	16 */
	"set of",		/*  btSet	17 */
	"complex",		/*  btComplex	18 */
	"double complex",	/*  btDComplex	19 */
	"indirect type",	/*  btIndirect	20 */
	"fixed decimal",	/*  btFixedDec	21 */
	"float decimal",	/*  btFloatDec	22 */
	"string",		/*  btString	23 */
	"bit string",		/*  btBit	24 */
	"picture"		/*  btPicture	25 */
}; /* st_mbt_ascii */
#define	MAX_ST_MBT_ASCII	25	/* update if the array is changed */
					/* must change ldgetrfd.c also    */

export char	*st_mtq_ascii [] = {
	"tqNil",		/* tqNil	0 */
	"Pointer to",		/* tqPtr	1 */
	"Function returning",	/* tqProc	2 */
	"Array",		/* tqArray 3 */
	"NON-mips",		/* tqFar	4 */
	"Volatile",		/* tqVol	5 */
}; /* st_mtq_ascii */


export void st_dump (fd, flags)

FILE	*fd;
long	flags;

{
    /* dump the symbol table in ascii */
    long	ifd;
    long	idn;
    pEXTR	pext;
    extern pCHDRR st_pchdr;

    if (!fd)
	fd = stdout;

    if (flags & (ST_PSYMS|ST_PLINES|ST_PRFDS|ST_POPTS|ST_PPDS|ST_PFDS|ST_PAUXS)) {
	fprintf (fd, "\n\nSYMBOLS TABLE:\n");
	st_cindent = 0;
	for (ifd = 0; ifd < st_ifdmax(); ifd++) {
	    st_setfd (ifd);
	    st_printfd (fd, ifd, flags);
	} /* for */
    } /* if */

    if (flags & ST_PEXTS)
	st_dumpext (fd, flags);

    if (flags & ST_PDNS) {
	fprintf (fd, "\n\nDense number table:\n");
	for (idn = ST_IDNINIT; idn < st_pchdr->cdn; idn++) {
	    fprintf (fd, "%d. ", idn);

	    if (st_pchdr->pdn[idn].rfd == ST_EXTIFD)
		fprintf (fd, "external ");
	    else
		fprintf (fd, "file %d, ", st_pchdr->pdn[idn].rfd);

	    if (st_pchdr->pdn[idn].index == ST_ANONINDEX)
		fprintf (fd, "anonymous symbol\n");
	    else
		fprintf (fd, "symbol %d\n", st_pchdr->pdn[idn].index);
	} /* for */
    } /* if */
    fflush (fd);
} /* st_dump */


#define NILSYMREF(pfd, x) {fprintf(pfd, \
	((x) == indexNil) ? "symref (indexNil) " : "symref %d", x); }

static st_printsym (pfd, fext, psym)
FILE	*pfd;
pSYMR	psym;

{
	/* this provides the interface to the print symbols and
	 *	externals. first print the basic info, then the
	 *	index or aux depending on the st.
	 */

	fprintf (pfd, "(%4d) %-10s %-10s %-10s ", psym->value,
		fext ? st_str_extiss (psym->iss) : st_str_iss (psym->iss),
		st_mst_ascii[psym->st], st_msc_ascii[psym->sc]);

	switch (psym->st) {

	case stEnd:
		 NILSYMREF (pfd, psym->index);
		break;

	case stBlock:
	case stFile:
		st_cindent++;
		/* fall through */
	case stLabel:
		NILSYMREF (pfd, psym->index);
		break;

	case stProc:
	case stStaticProc:
		if (fext && psym->sc != scUndefined) {
		    NILSYMREF (pfd, psym->index);
		    break;
		} /* if */
		st_cindent++;
		/* fall through */

	default:
		st_printaux (pfd, psym, fext);

	} /* switch */

} /* st_printsym */


func_print_tq(pfd, ppaux, ppaux2, piaux)
FILE  *pfd;
pAUXU *ppaux, *ppaux2;
long  *piaux;
{
			*ppaux = st_paux_iaux (*piaux);
			*piaux = *piaux + 1;
			if ((*ppaux)->rndx.rfd == ST_RFDESCAPE) {
			    *ppaux2 = st_paux_iaux (*piaux);
			    *piaux = *piaux + 1;
			    fprintf (pfd, "[(extended file %d, aux %d)", 
				(*ppaux2)->isym, (*ppaux)->rndx.index); 
			} else {
			    fprintf (pfd, "[(file %d, aux %d)", 
				(*ppaux)->rndx.rfd, (*ppaux)->rndx.index); 
			}
			*ppaux = st_paux_iaux (*piaux);
			*piaux = *piaux + 1;
			*ppaux2 = st_paux_iaux (*piaux);
			*piaux = *piaux + 1;		
			fprintf (pfd, "%d-%d:", (*ppaux)->dnLow, (*ppaux2)->dnHigh);
			*ppaux2 = st_paux_iaux (*piaux);
			*piaux = *piaux + 1;			
			fprintf (pfd, "%d] of ", (*ppaux2)->width);
}

static st_printaux (pfd, psym, fext)

FILE	*pfd;
pSYMR	psym;
long	fext;

{
	/* this routine will print auxiliary entries, it needs the
	 *	sym as an argument because the aux depends on it
	 *	for interpretation.
	 */
	pAUXU	paux;
	pAUXU	paux1;
	pAUXU	paux2;
	long	iaux;
	TIR	ti;
	long	fcomplex = 0;
	long	width;

	if (psym->index == 0xfffff) {
	    fprintf (pfd, "indexNil ");
	    return;
	}
	iaux = psym->index;

	fprintf (pfd, "[%2d] ", iaux);
	if (!fAux)
	    return;
	if (psym->st == stProc || psym->st == stStaticProc) {
		if (fext)
		    iaux++;
		else
		    fprintf (pfd, "endref %d, ", st_paux_iaux (iaux++)->isym);
	} /* if */

	paux = st_paux_iaux (iaux++);
	ti = paux->ti;

	if (ti.fBitfield == 1) {
	    paux = st_paux_iaux (iaux++);
	    width = paux->width;
	} /* if */


	if (ti.bt == btStruct || ti.bt == btEnum || ti.bt == btSet ||
		ti.bt == btIndirect ||
		ti.bt == btTypedef || ti.bt == btUnion) {
	    /* tuck away the rsym to the type this is */
	    paux1 = st_paux_iaux (iaux++);
	    if (paux1->rndx.rfd == ST_RFDESCAPE)
		iaux++;
	    fcomplex = 1;
	} /* if */

	 if (ti.bt == btRange) {
	    paux1 = st_paux_iaux (iaux);
	    if (paux1->rndx.rfd == ST_RFDESCAPE)
		iaux++;
	    iaux+=3;
	} /* if */


#define PRINTTQ(tq) \
	if (ti.tq != tqNil && ti.tq < (sizeof(st_mtq_ascii)/sizeof(st_mtq_ascii[0]))) {\
		fprintf (pfd, "%s ", st_mtq_ascii [ti.tq]);\
		if (ti.tq == tqArray) { \
			func_print_tq(pfd, &paux, &paux2, &iaux); \
		}\
	}

	while(1) {
		PRINTTQ(tq5);
		PRINTTQ(tq4);
		PRINTTQ(tq3);
		PRINTTQ(tq2);
		PRINTTQ(tq1);
		PRINTTQ(tq0);
		if (ti.continued == 1) {
			paux = st_paux_iaux(iaux++); 
			ti = paux->ti;}
		else break;
	}

	if (ti.bt < sizeof(st_mbt_ascii)/sizeof(st_mbt_ascii[0]))
	    fprintf (pfd, "%s", st_mbt_ascii [ti.bt]);
	else
	    fprintf (pfd, "BOGUS type (value:%d)", ti.bt);

	if (ti.bt == btRange) {
	    if (paux1->rndx.rfd == ST_RFDESCAPE)
		paux1++;
	    fprintf (pfd, "%d..%d of", paux1[1].dnLow, paux1[2].dnHigh);
	    if (paux1->rndx.rfd == ST_RFDESCAPE)
		paux1--;
	} /* if */

	if (fcomplex || ti.bt == btRange) {
	    if (paux1->rndx.rfd == ST_RFDESCAPE)
		fprintf (pfd, "(extended file %d, index %d)",paux1[1].isym, 
			paux1->rndx.index);
	    else
		fprintf (pfd, "(file %d, index %d)",paux1->rndx.rfd, 
			paux1->rndx.index);
	} /* if */

	if (ti.fBitfield == 1) {
		fprintf (pfd, ": %d", width);
	} /* if */

} /* st_print_aux */


static st_dumpext (fd, flags)

FILE	*fd;

{
	long	cext;
	long	iext;
	pEXTR	pext;
	pCFDR	pcfd;

	fprintf (fd, "\n\nExternals table:\n");
	cext = st_iextmax ();
	for (iext = 0; iext < cext; iext++) {

	    pext = st_pext_iext (iext);
	    st_setfd (pext->ifd>0 ? pext->ifd : 0);
	    pcfd = st_pcfd_ifd (st_currentifd());
	    fAux = (pcfd->pfd->caux && (flags & ST_PAUXS));
	    fprintf (fd, "%d. ", iext);
	    fprintf (fd, "(file %2d) ", pext->ifd);
	    st_printsym (fd, 1, &pext->asym);
	    if (pext->jmptbl)
		fprintf (fd, " (jmptbl)");
	    if (pext->cobol_main)
		fprintf (fd, " (cobol_main)");
	    fprintf (fd, "\n");
	} /* for */
} /* st_dumpext */


export void st_printfd (fd, ifd, flags)

FILE	*fd;
long	ifd;

{
	/* print the symbols associated with a file descriptor */
	pEXTR		pext;
	long		iss;
	long		isym;
	pCFDR		pcfd;
	pFDR		pfd;
	long		iline;
	pLINER		pline;
	long		iopt;
	pOPTR		popt;
	long		irfd;
	pRFDT		prfd;
	long		ipd;
	pPDR		ppd;
	long		iaux;
	pAUXU		paux;


	st_setfd (ifd);
	pcfd = st_pcfd_ifd (ifd);
	pfd = pcfd->pfd;
	fprintf (fd, "FILE %d. %s, %s, %s:\n", ifd,
		(pfd->fMerge) ? "mergable" : "unmergable",
		(pfd->fReadin) ? "preexisting" : "new",
		st_mlang_ascii [pfd->lang]);

	if (flags & ST_PAUXS) {
	    fprintf (fd, "\nBinary of auxes:");
	    for ((iaux = 0), (paux = pcfd->paux); iaux < pcfd->pfd->caux;
		iaux++, paux++) {
		if (iaux % 5 == 0)
		    fprintf(fd, "\n%3d. ", iaux);
		fprintf(fd, "0x%08x   ", paux->isym);
	    }
	    fprintf(fd, "\n");
	}

	if (flags & ST_PSYMS) {
	    char	*sbfile;

	    fprintf (fd, "\n\nLocal Symbols:\n");
	    sbfile = st_str_ifd_iss(ifd,pcfd->pfd->rss);
	    fAux = (pcfd->pfd->caux && (flags & ST_PAUXS));
	    fprintf(fd, "from file %s   %s",
		(sbfile) ? sbfile : "<stripped>",
		(fAux) ? "Printf aux if present" : "Do not print aux");
	    fprintf(fd, "\n");
	    for (isym = 0; isym < pcfd->pfd->csym; isym++) {
		    if (pcfd->psym[isym].st == stEnd)
			    st_cindent--;
		    fprintf (fd, "%3d. (%2d)", isym, st_cindent);
		    st_printsym (fd, 0, &pcfd->psym[isym]);
		    fprintf (fd, "\n");
	    } /* for */
	} /* if */

	if (flags & ST_PRFDS) {
	    fprintf (fd, "\nFile indirect table:\n");
	    for ((irfd = 0) , (prfd = pcfd->prfd) ; irfd < pcfd->pfd->crfd;
		irfd++, prfd++) {
		fprintf (fd, "%3d. %d\n", irfd, *prfd);
	    } /* for */
	    fprintf (fd, "\n");
	} /* if */

	if (flags & ST_POPTS) {
	    fprintf (fd, "\nOpts:\n");
	    for ((iopt = 0) , (popt = pcfd->popt) ; iopt < pcfd->pfd->copt;
		iopt++, popt++) {
		fprintf (fd, "not done yet \n");
	    } /* for */
	    fprintf (fd, "\n");
	} /* if */

	if (flags & ST_PPDS) {
	    fprintf (fd, "\nProcs(%d entries, only those with a symbol print):\n", pcfd->pfd->cpd);
	    for ((ipd = 0) , (ppd = pcfd->ppd) ; ipd < pcfd->pfd->cpd;
		ipd++, ppd++) {
		if (ppd->isym == isymNil) {
		    continue;
		} else if (pcfd->pfd->csym) {
		    iss = (pcfd->psym + ppd->isym)->iss;
		} else {
		    pext = st_pext_iext(ppd->isym);
		    iss = pext->asym.iss;
		} /* if */
	fprintf (fd, "%3d. (%3d) %s, iline= 0x%x, regmask=0x%08x, save_reg_offset 0x%x \n\tfregmask 0x%08x, save_freg_offset 0x%x, low_line 0x%x, high_line 0x%x\n\tframe_size 0x%x, proc_addr 0x%08x\n", 
		    ipd, ppd->isym, pcfd->pfd->csym ?
		    st_str_ifd_iss (ifd, iss): st_str_extiss (iss),
		    ppd->iline, ppd->regmask, ppd->regoffset, ppd->fregmask,
		    ppd->fregoffset, ppd->lnLow, ppd->lnHigh,
		    ppd->frameoffset, ppd->adr);
	    } /* for */
	    fprintf (fd, "\n");
	} /* if */

	if (flags & ST_PLINES) {
	    fprintf (fd, "\nLines:\n");
	    for ((iline = 0) , (pline = pcfd->pline) ; iline < pcfd->pfd->cline;
		iline++, pline++) {
		if (iline && !(iline %4))
		    fprintf (fd, "\n");
		else
		    fprintf (fd, "    ");
		fprintf (fd, "%3d. %3d", iline, *pline); 
	    } /* for */
	    fprintf (fd, "\n");
	} /* if */

} /* st_printfd */
