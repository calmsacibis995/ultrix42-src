/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: swap.c,v 2010.3.1.3 89/11/29 14:29:49 bettina Exp $ */
/*LINTLIBRARY*/
/*
 * This file contains the routines to change the byte sex of structures.
 *
 * These routines are VERY DEPENDENT on the definitions in the header files for
 * the structures they operate on.  If any of these structures change the code
 * will have to change in the routines that deal with them.  Bit fields are
 * allocated in opposite orders for the two sexes so there are local definitions
 * of structures which have the bit fields declared in the opposite order so
 * they can be sex changed.  Again if any of these structures change the local
 * definition in here also have to be changed.
 *
 * The way the byte sex of a structure is changed is by first swapping the bytes
 * in the words and half words in the structure.  And then for bit fields after
 * the word or half that contains the fields has been swapped assignments of the
 * fields from a structure which has the fields declared in the opposite order
 * are done.
 */

#include <stdio.h>
#include "filehdr.h"
#include "aouthdr.h"
#include "syms.h"
#include "scnhdr.h"
#include "reloc.h"
#include "ar.h"

#include "sex.h"

/*
 * Convert a file header pointed to by pfilehdr to the other byte sex.
 */
void
swap_filehdr(pfilehdr, destsex)
register FILHDR *pfilehdr;
long destsex;
{
	pfilehdr->f_magic  = swap_half(pfilehdr->f_magic);
	pfilehdr->f_nscns  = swap_half(pfilehdr->f_nscns);
	pfilehdr->f_timdat = swap_word(pfilehdr->f_timdat);
	pfilehdr->f_symptr = swap_word(pfilehdr->f_symptr);
	pfilehdr->f_nsyms  = swap_word(pfilehdr->f_nsyms);
	pfilehdr->f_opthdr = swap_half(pfilehdr->f_opthdr);
	pfilehdr->f_flags  = swap_half(pfilehdr->f_flags);
}

/*
 * Convert a optional header pointed to by paouthdr to the other byte sex.
 */
void
swap_aouthdr(paouthdr, destsex)
register AOUTHDR *paouthdr;
long destsex;
{
	paouthdr->magic      = swap_half(paouthdr->magic);
	paouthdr->vstamp     = swap_half(paouthdr->vstamp);
	paouthdr->tsize      = swap_word(paouthdr->tsize);
	paouthdr->dsize      = swap_word(paouthdr->dsize);
	paouthdr->bsize      = swap_word(paouthdr->bsize);
	paouthdr->entry      = swap_word(paouthdr->entry);
	paouthdr->text_start = swap_word(paouthdr->text_start);
	paouthdr->data_start = swap_word(paouthdr->data_start);
	paouthdr->bss_start  = swap_word(paouthdr->bss_start);
	paouthdr->gprmask    = swap_word(paouthdr->gprmask);
	paouthdr->cprmask[0] = swap_word(paouthdr->cprmask[0]);
	paouthdr->cprmask[1] = swap_word(paouthdr->cprmask[1]);
	paouthdr->cprmask[2] = swap_word(paouthdr->cprmask[2]);
	paouthdr->cprmask[3] = swap_word(paouthdr->cprmask[3]);
	paouthdr->gp_value   = swap_word(paouthdr->gp_value);
}

/*
 * Convert a section header pointed to by pscnhdr to the other byte sex.
 */
void
swap_scnhdr(pscnhdr, destsex)
register SCNHDR *pscnhdr;
long destsex;
{
	pscnhdr->s_paddr   = swap_word(pscnhdr->s_paddr);
	pscnhdr->s_vaddr   = swap_word(pscnhdr->s_vaddr);
	pscnhdr->s_size    = swap_word(pscnhdr->s_size);
	pscnhdr->s_scnptr  = swap_word(pscnhdr->s_scnptr);
	pscnhdr->s_relptr  = swap_word(pscnhdr->s_relptr);
	pscnhdr->s_lnnoptr = swap_word(pscnhdr->s_lnnoptr);
	pscnhdr->s_nreloc  = swap_half(pscnhdr->s_nreloc);
	pscnhdr->s_nlnno   = swap_half(pscnhdr->s_nlnno);
	pscnhdr->s_flags   = swap_word(pscnhdr->s_flags);
}

/*
 * Convert lib section pointed to by plibscn to the other byte sex.
 */
void
swap_libscn(plibscn, destsex)
register LIBSCN *plibscn;
long destsex;
{
	plibscn->size       = swap_word(plibscn->size );
	plibscn->offset     = swap_word(plibscn->offset);
	plibscn->tsize      = swap_word(plibscn->tsize);
	plibscn->dsize      = swap_word(plibscn->dsize);
	plibscn->bsize      = swap_word(plibscn->bsize);
	plibscn->text_start = swap_word(plibscn->text_start);
	plibscn->data_start = swap_word(plibscn->data_start);
	plibscn->bss_start  = swap_word(plibscn->bss_start);
}
/*
 * Convert a symbolic header pointed to by phdr to the other byte sex.
 */
void
swap_hdr(phdr, destsex)
register pHDRR phdr;
long destsex;
{
	phdr->magic         = swap_half(phdr->magic);
	phdr->vstamp        = swap_half(phdr->vstamp);
	phdr->ilineMax      = swap_word(phdr->ilineMax);
	phdr->cbLine        = swap_word(phdr->cbLine);
	phdr->cbLineOffset  = swap_word(phdr->cbLineOffset);
	phdr->idnMax        = swap_word(phdr->idnMax);
	phdr->cbDnOffset    = swap_word(phdr->cbDnOffset);
	phdr->ipdMax        = swap_word(phdr->ipdMax);
	phdr->cbPdOffset    = swap_word(phdr->cbPdOffset);
	phdr->isymMax       = swap_word(phdr->isymMax);
	phdr->cbSymOffset   = swap_word(phdr->cbSymOffset);
	phdr->ioptMax       = swap_word(phdr->ioptMax);
	phdr->cbOptOffset   = swap_word(phdr->cbOptOffset);
	phdr->iauxMax       = swap_word(phdr->iauxMax);
	phdr->cbAuxOffset   = swap_word(phdr->cbAuxOffset);
	phdr->issMax        = swap_word(phdr->issMax);
	phdr->cbSsOffset    = swap_word(phdr->cbSsOffset);
	phdr->issExtMax     = swap_word(phdr->issExtMax);
	phdr->cbSsExtOffset = swap_word(phdr->cbSsExtOffset);
	phdr->ifdMax        = swap_word(phdr->ifdMax);
	phdr->cbFdOffset    = swap_word(phdr->cbFdOffset);
	phdr->crfd          = swap_word(phdr->crfd);
	phdr->cbRfdOffset   = swap_word(phdr->cbRfdOffset);
	phdr->iextMax       = swap_word(phdr->iextMax);
	phdr->cbExtOffset   = swap_word(phdr->cbExtOffset);
}

/*
 * Convert count file table entries pointed to by pfd to the other sex.
 */
void
swap_fd(pfd, count, destsex)
register pFDR pfd;
register long count;
long destsex;
{
    register long i;
    register long hostsex = gethostsex();

    typedef struct {
	unsigned long	adr;
	long	rss;
	long	issBase;
	long	cbSs;
	long	isymBase;
	long	csym;
	long	ilineBase;
	long	cline;
	long	ioptBase;
	long	copt;
	short	ipdFirst;
	short	cpd;
	long	iauxBase;
	long	caux;
	long	rfdBase;
	long	crfd;
	union {
	    struct {
		unsigned reserved : 22;
		unsigned glevel : 2;
		unsigned fBigendian : 1;
		unsigned fReadin : 1;
		unsigned fMerge : 1;
		unsigned lang: 5;
	    } fields;
	    unsigned long whole;
	} u;
	long	cbLineOffset;
	long	cbLine;
    } sFDR, *psFDR;

    sFDR sfdr;

	for(i = 0 ; i < count ; i++){
	    sfdr = ((psFDR)pfd)[i];

	    sfdr.adr         = swap_word(sfdr.adr);
	    sfdr.rss         = swap_word(sfdr.rss);
	    sfdr.issBase     = swap_word(sfdr.issBase);
	    sfdr.cbSs        = swap_word(sfdr.cbSs);
	    sfdr.isymBase    = swap_word(sfdr.isymBase);
	    sfdr.csym        = swap_word(sfdr.csym);
	    sfdr.ilineBase   = swap_word(sfdr.ilineBase);
	    sfdr.cline       = swap_word(sfdr.cline);
	    sfdr.ioptBase    = swap_word(sfdr.ioptBase);
	    sfdr.copt        = swap_word(sfdr.copt);
	    sfdr.ipdFirst    = swap_half(sfdr.ipdFirst);
	    sfdr.cpd         = swap_half(sfdr.cpd);
	    sfdr.iauxBase    = swap_word(sfdr.iauxBase);
	    sfdr.caux        = swap_word(sfdr.caux);
	    sfdr.rfdBase     = swap_word(sfdr.rfdBase);
	    sfdr.crfd        = swap_word(sfdr.crfd);
	    sfdr.cbLineOffset = swap_word(sfdr.cbLineOffset);
	    sfdr.cbLine	     = swap_word(sfdr.cbLine);

	    if (destsex == hostsex) {
		pfd[i] = *(pFDR)&sfdr;
		sfdr.u.whole = swap_word(sfdr.u.whole);

		pfd[i].lang       = sfdr.u.fields.lang;
		pfd[i].fMerge     = sfdr.u.fields.fMerge;
		pfd[i].fReadin    = sfdr.u.fields.fReadin;
		pfd[i].fBigendian = sfdr.u.fields.fBigendian;
		pfd[i].glevel     = sfdr.u.fields.glevel;
		pfd[i].reserved   = sfdr.u.fields.reserved;
	    } else {

		sfdr.u.fields.lang = pfd[i].lang;
		sfdr.u.fields.fMerge = pfd[i].fMerge;
		sfdr.u.fields.fReadin = pfd[i].fReadin;
		sfdr.u.fields.fBigendian = pfd[i].fBigendian;
		sfdr.u.fields.glevel = pfd[i].glevel;
		sfdr.u.fields.reserved = pfd[i].reserved;

		sfdr.u.whole = swap_word(sfdr.u.whole);
		pfd[i] = *(pFDR)&sfdr;
	    } /* if */
	}
}

/*
 * Convert count file indirect table entries pointed to by pfi to the other sex.
 */
void
swap_fi(pfi, count, destsex)
register pFIT pfi;
register long count;
long destsex;
{
    register long i;

	for(i = 0 ; i < count ; i++)
	    pfi[i] = swap_word(pfi[i]);
}

typedef struct {
    long	iss;
    long	value;
    union {
	struct {
	    unsigned index : 20;
	    unsigned reserved : 1;
	    unsigned sc  : 5;
	    unsigned st : 6;
	} fields;
	unsigned long word;
    } u;
} sSYMR, *psSYMR;

/*
 * Convert count symbol table entries pointed to by psym to the other byte sex.
 */
void
swap_sym(psym, count, destsex)
register pSYMR psym;
register long count;
long destsex;
{
    register long i;
    register long hostsex = gethostsex();

    sSYMR ssymr;

	for(i = 0 ; i < count ; i++){
	    ssymr = ((psSYMR)psym)[i];
	    ssymr.iss   = swap_word(ssymr.iss);
	    ssymr.value = swap_word(ssymr.value);

	    if (destsex == hostsex) {
		psym[i] = *(pSYMR)&ssymr;
		ssymr.u.word  = swap_word(ssymr.u.word);
		psym[i].st       = ssymr.u.fields.st;
		psym[i].sc       = ssymr.u.fields.sc;
		psym[i].reserved   = ssymr.u.fields.reserved;
		psym[i].index    = ssymr.u.fields.index;
	    } else {
		ssymr.u.fields.st = psym[i].st;
		ssymr.u.fields.sc = psym[i].sc;
		ssymr.u.fields.reserved = psym[i].reserved;
		ssymr.u.fields.index = psym[i].index;
		ssymr.u.word  = swap_word(ssymr.u.word);
		psym[i] = *(pSYMR)&ssymr;
	    } /* if */
	}
}

/*
 * Convert count external symbol table entries pointed to by pext to the other
 * sex.
 */
void
swap_ext(pext, count, destsex)
register pEXTR pext;
register long count;
long destsex;
{
    register long i;
    register long hostsex = gethostsex();

    typedef struct {
	union {
	    struct {
		short	ifd;
		unsigned reserved : 14;
		unsigned cobol_main: 1;
		unsigned jmptbl : 1;
	    } fields;
	    unsigned long word;
	} u;
	sSYMR	asym;
    } sEXTR, *psEXTR;

    sEXTR sextr;

	for(i = 0 ; i < count ; i++){
	    sextr = ((psEXTR)pext)[i];
	    sextr.asym.iss   = swap_word(sextr.asym.iss);
	    sextr.asym.value = swap_word(sextr.asym.value);

	    if (destsex == hostsex) {
		pext[i] = *(pEXTR)&sextr;
		sextr.asym.u.word  = swap_word(sextr.asym.u.word);

		pext[i].asym.st       = sextr.asym.u.fields.st;
		pext[i].asym.sc       = sextr.asym.u.fields.sc;
		pext[i].asym.reserved   = sextr.asym.u.fields.reserved;
		pext[i].asym.index    = sextr.asym.u.fields.index;

		sextr.u.word  = swap_word(sextr.u.word);
		pext[i].ifd   = sextr.u.fields.ifd;
		pext[i].reserved   = sextr.u.fields.reserved;
		pext[i].jmptbl   = sextr.u.fields.jmptbl;
		pext[i].cobol_main = sextr.u.fields.cobol_main;
	    } else {

		sextr.asym.u.fields.st = pext[i].asym.st;
		sextr.asym.u.fields.sc = pext[i].asym.sc;
		sextr.asym.u.fields.reserved = pext[i].asym.reserved;
		sextr.asym.u.fields.index = pext[i].asym.index;

		sextr.asym.u.word  = swap_word(sextr.asym.u.word);

		sextr.u.fields.ifd = pext[i].ifd;
		sextr.u.fields.reserved = pext[i].reserved;
		sextr.u.fields.cobol_main = pext[i].cobol_main;
		sextr.u.fields.jmptbl = pext[i].jmptbl;

		sextr.u.word  = swap_word(sextr.u.word);
		pext[i] = *(pEXTR)&sextr;
	    } /* if */
	}
}

/*
 * Convert count procedure table entries pointed to by ppd to the other byte
 * sex.
 */
void
swap_pd(ppd, count, destsex)
register pPDR ppd;
register long count;
long destsex;
{
    register long i;

	for(i = 0 ; i < count ; i++){
	    ppd[i].adr         = swap_word(ppd[i].adr);
	    ppd[i].isym        = swap_word(ppd[i].isym);
	    ppd[i].iline       = swap_word(ppd[i].iline);
	    ppd[i].regmask     = swap_word(ppd[i].regmask);
	    ppd[i].regoffset   = swap_word(ppd[i].regoffset);
	    ppd[i].iopt        = swap_word(ppd[i].iopt);
	    ppd[i].fregmask    = swap_word(ppd[i].fregmask);
	    ppd[i].fregoffset  = swap_word(ppd[i].fregoffset);
	    ppd[i].frameoffset = swap_word(ppd[i].frameoffset);
	    ppd[i].framereg    = swap_half(ppd[i].framereg);
	    ppd[i].pcreg       = swap_half(ppd[i].pcreg);
	    ppd[i].lnLow       = swap_word(ppd[i].lnLow);
	    ppd[i].lnHigh      = swap_word(ppd[i].lnHigh);
	    ppd[i].cbLineOffset = swap_word(ppd[i].cbLineOffset);
	}
}

/*
 * Convert count dense number entries entries pointed to by pdn to the
 * other byte sex.
 */
void
swap_dn(pdn, count, destsex)
register pDNR pdn;
register long count;
long destsex;
{
    register long i;

	for(i = 0 ; i < count ; i++) {
	    pdn[i].rfd            = swap_word(pdn[i].rfd);
	    pdn[i].index            = swap_word(pdn[i].index);
	}
}

/*
 * Convert count runtime procedure table entries pointed to by prpd to the
 * other byte sex.
 */
void
swap_rpd(prpd, count, destsex)
register pRPDR prpd;
register long count;
long destsex;
{
    register long i;

	for(i = 0 ; i < count ; i++){
	    prpd[i].adr            = swap_word(prpd[i].adr);
	    prpd[i].regmask        = swap_word(prpd[i].regmask);
	    prpd[i].regoffset      = swap_word(prpd[i].regoffset);
	    prpd[i].fregmask       = swap_word(prpd[i].fregmask);
	    prpd[i].fregoffset     = swap_word(prpd[i].fregoffset);
	    prpd[i].frameoffset    = swap_word(prpd[i].frameoffset);
	    prpd[i].framereg       = swap_word(prpd[i].framereg);
		/* pointers must be castable to long and back */
	    prpd[i].exception_info = (struct exception_info *)swap_word((long)prpd[i].exception_info);
	}
}

typedef struct {
    union {
	struct {
	    unsigned index : 20;
	    unsigned rfd : 12;
	} fields;
	unsigned long word;
    } u;
} sRNDXR, *psRNDXR;

/*
 * Convert count optimization table entries pointed to by popt to the other byte
 * sex.
 */
void
swap_opt(popt, count, destsex)
register pOPTR popt;
register long count;
long destsex;
{
    register long i;
    register long hostsex = gethostsex();

    typedef struct {
	union {
	    struct {
		unsigned ot: 8;
		unsigned value: 24;
	    } fields;
	    unsigned long word;
	} u;
	sRNDXR	rndx;
	unsigned long	offset;
    } sOPTR, *psOPTR;

    sOPTR soptr;

	for(i = 0 ; i < count ; i++){
	    soptr = ((psOPTR)popt)[i];
	    soptr.offset    = swap_word(soptr.offset);

	    if (destsex == hostsex) {
		popt[i] = *(pOPTR)&soptr;
		soptr.u.word      = swap_word(soptr.u.word);
		soptr.rndx.u.word = swap_word(soptr.rndx.u.word);

		popt[i].ot    = soptr.u.fields.ot;
		popt[i].value = soptr.u.fields.value;
		popt[i].rndx.index = soptr.rndx.u.fields.index;
		popt[i].rndx.rfd   = soptr.rndx.u.fields.rfd;
	    } else {
		soptr.u.fields.ot = popt[i].ot;
		soptr.u.fields.value = popt[i].value;
		soptr.rndx.u.fields.index = popt[i].rndx.index;
		soptr.rndx.u.fields.rfd = popt[i].rndx.rfd;

		soptr.u.word      = swap_word(soptr.u.word);
		soptr.rndx.u.word = swap_word(soptr.rndx.u.word);
		popt[i] = *(pOPTR)&soptr;
	    } /* if */
	}
}

/*
 * Convert count auxiliary table entries pointed to by paux of type to the other
 * byte sex.
 */
void
swap_aux(paux, type, destsex)
register pAUXU paux;
long type;
long destsex;
{
    typedef struct {
	union {
	    struct {
		unsigned tq3 : 4;
		unsigned tq2 : 4;
		unsigned tq1 : 4;
		unsigned tq0 : 4;
		unsigned tq5 : 4;
		unsigned tq4 : 4;
		unsigned bt  : 6;
		unsigned continued : 1;
		unsigned fBitfield : 1;
	    } fields;
	    unsigned long word;
	} u;
    } sTIR, *psTIR;
    sTIR stir;
    register long hostsex = gethostsex();

    sRNDXR srndxr;

    if (destsex != hostsex) {
	fprintf (stderr, "swap of auxs not supported when destsex != hostsex\n");
	return;
    } /* if */

	switch(type){
	case ST_AUX_TIR:
	    stir = *((psTIR)paux);
	    stir.u.word = swap_word(stir.u.word);

	    paux->ti.tq3 = stir.u.fields.tq3;
	    paux->ti.tq2 = stir.u.fields.tq2;
	    paux->ti.tq1 = stir.u.fields.tq1;
	    paux->ti.tq0 = stir.u.fields.tq0;
	    paux->ti.tq5 = stir.u.fields.tq5;
	    paux->ti.tq4 = stir.u.fields.tq4;
	    paux->ti.bt = stir.u.fields.bt;
	    paux->ti.continued = stir.u.fields.continued;
	    paux->ti.fBitfield = stir.u.fields.fBitfield;
	    break;

	case ST_AUX_RNDXR:
	    srndxr = *((psRNDXR)paux);
	    srndxr.u.word = swap_word(srndxr.u.word);

	    paux->rndx.index = srndxr.u.fields.index;
	    paux->rndx.rfd   = srndxr.u.fields.rfd;
	    break;

	case ST_AUX_DNLOW:
	case ST_AUX_DNMAC:
	case ST_AUX_ISYM:
	case ST_AUX_ISS:
	case ST_AUX_WIDTH:
	default:
	    paux->width = swap_word(paux->width);
	    break;
	}
}

/*
 * Convert count relocation entries pointed to by preloc to the other byte sex.
 */
void
swap_reloc(preloc, count, destsex)
register struct reloc *preloc;
register long count;
long destsex;
{
    register long i;
    register long hostsex = gethostsex();

    typedef struct {
	long	r_vaddr;
	union {
	    struct {
		unsigned r_extern:1;
		unsigned r_type:4;
		unsigned r_reserved:3;
		unsigned r_symndx:24;
	    } fields;
	    unsigned long word;
	} u;
    } sRELOC, *psRELOC;

    sRELOC sreloc;

	for(i = 0 ; i < count ; i++){
	    sreloc = ((psRELOC)preloc)[i];
	    sreloc.r_vaddr = swap_word(sreloc.r_vaddr);

	    if (destsex == hostsex) {
		preloc[i] = *(RELOC *)&sreloc;
		sreloc.u.word     = swap_word(sreloc.u.word);

		preloc[i].r_extern   = sreloc.u.fields.r_extern;
		preloc[i].r_type     = sreloc.u.fields.r_type;
		preloc[i].r_reserved = sreloc.u.fields.r_reserved;
		preloc[i].r_symndx   = sreloc.u.fields.r_symndx;
	    } else {
		sreloc.u.fields.r_extern = preloc[i].r_extern;
		sreloc.u.fields.r_type = preloc[i].r_type;
		sreloc.u.fields.r_reserved = preloc[i].r_reserved;
		sreloc.u.fields.r_symndx = preloc[i].r_symndx;

		sreloc.u.word     = swap_word(sreloc.u.word);
		preloc[i] = *(RELOC *)&sreloc;
	    } /* if */
	}
}

/*
 * Convert count ranlib structures pointed to by pranlib to the other byte sex.
 */
void
swap_ranlib(pranlib, count, destsex)
register struct ranlib *pranlib;
register long count;
long destsex;
{
    register long i;

	for(i = 0 ; i < count ; i++){
	    pranlib[i].ran_un.ran_strx = swap_word(pranlib[i].ran_un.ran_strx);
	    pranlib[i].ran_off	       = swap_word(pranlib[i].ran_off);
	}
}

/*
 * Convert count gph structures pointed to by pgph to the other byte sex.
 */
void
swap_gpt(pgpt, count, destsex)
register GPTAB *pgpt;
register long count;
long destsex;
{
    long i;

	for(i = 0 ; i < count ; i++){
	    pgpt[i].entry.g_value = swap_word(pgpt[i].entry.g_value);
	    pgpt[i].entry.bytes   = swap_word(pgpt[i].entry.bytes);
	}
}
