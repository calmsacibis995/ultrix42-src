/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: llib-lmld.c,v 2010.2.1.3 89/11/29 14:30:08 bettina Exp $ */
/* $Log:	llib-lmld.c,v $
 * Revision 2010.2.1.3  89/11/29  14:30:08  bettina
 * 2.10 BETA2
 * 
 * Revision 2010.1  89/09/27  12:09:50  lai
 * rev 2.10
 *  */

/* this is the stub file used to create the lint library for libmld */

/*LINTLIBRARY*/
#include <stdio.h>
#include "filehdr.h"
#include "sex.h"
#include "syms.h"
#include "ldfcn.h"
#include "lddef.h"
#include "syntax.h"
#       include <sys/inst.h>
#include <opnames.h>
#include <disassembler.h>
#include <errno.h>
#include <ar.h>
#include "scnhdr.h"
#include <sys/inst.h>
#include "a.out.h"
#include <values.h>
#include "stext.h"
#include <sys/types.h>
#include <fcntl.h>
#include "stamp.h"
#include "cmplrs/usys.h"
#undef OFFSET	/* we need to undef this because it is used in ldfcn.h and ucode.h */
#include "cmplrs/ucode.h"
#include "aouthdr.h"
#include "reloc.h"


/* from allocldptr.c */
LDLIST  *_ldhead;
int ldlast_fnum_;

LDFILE *allocldptr() { return(0); }

/* from disassembler.c */
char *dis_reg_names[3][32];
void dis_init(addr_format, value_format, reg_names, print_jal_targets)
 char *addr_format; char *value_format; char *reg_names[]; int print_jal_targets; { }
int disasm(buffer, address, iword, regmask, symbol_value, ls_register)
 char *buffer; unsigned address, iword, *regmask, *symbol_value, *ls_register; { return(0); }
void dis_regs(buffer, regmask, reg_values)
 char *buffer; unsigned regmask; unsigned reg_values[]; {}
int disassembler(iadr, regstyle, get_symname, get_regvalue, get_bytes, print_header)
 unsigned iadr; int regstyle; char *(*get_symname)(); int (*get_regvalue)(); long (*get_bytes)(); void (*print_header)(); { return(0);}


/* from freeldptr.c */

int freeldptr(ldptr) LDFILE  *ldptr; { return(0);}


/* from ldaclose.c */

int ldaclose(ldptr) LDFILE    *ldptr; {return(0);}


/* from ldahread.c */

int ldahread(ldptr, arhead) LDFILE *ldptr; ARCHDR *arhead; { return(0);}

/* from ldaopen.c */

LDFILE *ldaopen(filename, oldptr) char *filename; LDFILE *oldptr; {return(0);}


/* from ldclose.c */

int ldclose(ldptr) LDFILE *ldptr; { return(0);}
LDFILE *ldinitheaders (ldptr) LDFILE  *ldptr; { return(ldptr);}
ldreadst(ldptr,flags) LDFILE  *ldptr; { return(0);}


/* from ldfgptorder.c */

long ldfgptorder(d_scnhdr, sd_scnhdr, sb_scnhdr, b_scnhdr)
 SCNHDR *d_scnhdr, *sd_scnhdr, *sb_scnhdr, *b_scnhdr; {return(0);}


/* from ldfhread.c */

int ldfhread(ldptr, filehead) LDFILE    *ldptr; FILHDR    *filehead; {return(0);}


/* from ldflitorder.c */

int ldflitorder(l8, l4) SCNHDR *l8, *l4; {return(0);}


/* from ldfscnorder.c */

int ldfscnorder(t, i, rd, d, l8, l4, sd, l)
 SCNHDR *t, *i, *rd, *d, *l8, *l4, *sd, *l; {return(0);}


/* from ldfsymorder.c */

ldfsymorder (phdr, symptr) pHDRR   phdr; long    symptr; {return(0);}


/* from ldgetaux.c */

AUXU *ldgetaux(ldptr, iaux) LDFILE *ldptr; int    iaux; {return(0);}


/* from ldgetname.c */

char * ldgetname(ldptr, psym) LDFILE *ldptr; pSYMR  psym; {return(0);}


/* from ldgetpd.c */

long ldgetpd(ldptr, ipd, ppd) LDFILE *ldptr; int    ipd; pPDR ppd; {return(0);}


/* from  ldgetrfd.c */

RFDT ldgetrfd(ldptr, rfd) LDFILE *ldptr; int     rfd; {return(0);}
ldgetsymstr (ldptr, psym, isym, straux) LDFILE  *ldptr; pSYMR   psym; char    *straux; {return(0);}
ldgetrndx(ldptr, ifd, paux, prfd, praux) LDFILE  *ldptr; AUXU    *paux, *praux; long    *prfd; {return(0);}


/* from ldlread.c */

int ldlread(ldptr, fcnindx, linenum, linent) LDFILE          *ldptr;
 long            fcnindx; unsigned short  linenum; LINER           *linent; {return(0);}
int ldlinit(ldptr, fcnindx) LDFILE          *ldptr; long            fcnindx; {return(0);}


/* from ldlseek.c */

int ldlseek(ldptr, sectnum) LDFILE          *ldptr; unsigned short  sectnum; {return(0);}


/* from  ldnlseek.c */

int ldnlseek(ldptr, sectname) LDFILE  *ldptr; char    *sectname; {return(0);}


/* from  ldnrseek.c */

int ldnrseek(ldptr, sectname) LDFILE  *ldptr; char    *sectname; {return(0);}



/* from  ldnshread.c */

int ldnshread(ldptr, sectname, secthdr)
 LDFILE  *ldptr; char    *sectname; SCNHDR  *secthdr; {return(0);}


/* from  ldnsseek.c */

int ldnsseek(ldptr, sectname)
 LDFILE  *ldptr; char    *sectname;
 {return(0);}


/* from  ldohseek.c */

int ldohseek(ldptr) LDFILE          *ldptr; {return(0);}


/* from  ldopen.c */

LDFILE *ldopen(filename, ldptr) char *filename; LDFILE *ldptr; {return(0);}


/* from  ldrseek.c */

int ldrseek(ldptr, sectnum) LDFILE          *ldptr; unsigned short  sectnum; {return(0);}


/* from  ldshread.c */

int ldshread(ldptr, sectnum, secthdr)
 LDFILE  *ldptr; unsigned short  sectnum; SCNHDR  *secthdr; {return(0);}


/* from  ldsseek.c */

int ldsseek(ldptr, sectnum) LDFILE          *ldptr; unsigned short  sectnum; {return(0);}


/* from  ldtbindex.c */

long ldtbindex(ldptr) LDFILE  *ldptr; {return(0);}


/* from  ldtbread.c */

int ldtbread(ldptr, symnum, psym) LDFILE  *ldptr; long    symnum; pSYMR   psym; {return(0);}
ld_ifd_symnum (ldptr, symnum) LDFILE  *ldptr; int     symnum; {return(0);}
ld_ifd_iaux(ldptr, iaux) LDFILE  *ldptr; int     iaux; {return(0);}


/* from  ldtbseek.c */

int ldtbseek(ldptr) LDFILE          *ldptr; {return(0);}


/* from  neednop.c */

typedef union mips_instruction mips_inst;
int need_nop (i0, i1, i2) mips_inst i0, i1, i2; {return(0);}


/* from  nlist.c */

nlist (filename, nlbase) char    *filename; struct nlist *nlbase; {return(0);}


/* from  opnames.c */

char *op_name[64];
char *spec_name[64];
char *bcond_name[32];
char *cop1func_name[64];
char *bc_name[32];
char *c0func_name[64];
char *c0reg_name[32];


/* from  ranhash.c */

int ranhashinit (pran, pstr, size) struct ranlib   *pran; char            *pstr; int size; {return(0);}
ranhash(s) unsigned char *s; {return(0);}
struct ranlib *ranlookup(name) char            *name;{return(0);}


/* from  sex.c */

int gethostsex() {return(0);}


/* from  sgetl.c */

long sgetl(buffer) char *buffer; {return(0);}


/* from  sputl.c */

int sputl(w, buffer) long w; char *buffer; {}


/* from  staux.c */

st_tqhigh_iaux (iaux) long    iaux; {return(0);}
void st_shifttq (iaux, tq) long    iaux; long    tq; {}
long st_iaux_copyty (ifd, psym) long    ifd; pSYMR   psym; {return(0);}
void st_changeaux (iaux, aux) long    iaux; AUXU    aux; {}
void st_addtq (iaux, tq) long    iaux; long    tq; {}
void st_changeauxrndx (iaux, ifd, index_) long iaux,ifd,index_; {}
long st_auxbtadd (bt) long    bt; {return(0);}
long st_auxisymadd (isym) long isym; {return(0);}
long st_auxrndxadd (ifd, index_) long ifd; long index_; {return(0);}
long st_auxbtsize (iaux, width) {return(0);}
long st_auxrndxadd_idn (idn) int idn; {return(0);}
void st_addcontinued(iaux) long iaux; {}


/* from  stcu.c */

pCHDRR  st_pchdr;
pCHDRR st_cuinit () {return(st_pchdr);}
void st_setchdr(pchdr) pCHDRR  pchdr; {}
pCHDRR st_currentpchdr() {return(st_pchdr);}
void st_free () {}
long st_extadd (iss, value, st, sc, index_) long iss, value, st, sc, index_; {return(0);}
long st_extstradd(cp) char *cp; {return(0);}
char *st_str_extiss (iss) long iss; {return(0);}
long st_idn_index_fext (index_, fext) long fext; long index_; {return(0);}
long st_idn_dn (dn) DNR     dn; {return(0);}
long st_idn_rndx (rndx) RNDXR   rndx; {return(0);}
RNDXR st_rndx_idn (idn) long    idn; { RNDXR rndx; return (rndx); }
pDNR st_pdn_idn (idn) long idn; { return 0; }
void st_setidn (idndest, idnsrc) long idndest; long idnsrc; {}
pEXTR st_pext_dn (dn) DNR     dn; {return(0);}
pEXTR st_pext_iext (index_) long    index_; {return(0);}
long st_iextmax () {return(0);}
char *st_errname;
st_setmsgname(name) char *name; {}


/* from  sterror.c */

st_error (s, a, b, c, d) char *s; {}


/* from  stfd.c */

long st_currentifd () {return(0);}
long st_ifdmax () {return(0);}
void st_setfd (index_) long index_; {}
void st_fdadd (filename, lang, merge, glevel) char    *filename; {}
long st_symadd (iss, value, st, sc, index_) long iss, value, st, sc, index_; {return(0);}
long st_auxadd (aux) AUXU    aux; {return(0);}
long st_pdadd (isym) long    isym; {return(0);}
long st_lineadd (lineno) long    lineno; {return(0);}
long st_stradd( cp ) char *cp; {return(0);}
pSYMR st_psym_ifd_isym (ifd, isym) long ifd, isym; {return(0);}
pAUXU st_paux_ifd_iaux (ifd, iaux) long    ifd,iaux; {return(0);}
pLINER st_pline_ifd_iline (ifd, iline) long ifd, iline; {return(0);}
pAUXU st_paux_iaux (iaux) long    iaux; {return(0);}
pPDR st_ppd_ifd_isym (ifd, isym) long ifd, isym; {return(0);}
long st_ifd_pcfd (pcfd1)pCFDR   pcfd1; {return(0);}
pCFDR st_pcfd_ifd (ifd) long    ifd; {return(0);}
char *st_str_iss (iss) {return(0);}
char *st_str_ifd_iss (ifd, iss) {return(0);}
char * st_malloc (ptr,size,itemsize,basesize) 
 char    *ptr; long    *size; long    itemsize; long    basesize; {return(0);}


/* from  stfe.c */

long st_filebegin (filename, lang, merge, glevel) char    *filename; {return(0);}
void st_endallfiles(){}
long st_fileend (idn) long idn; {return(0);}
long st_textblock() {return(0);}
long st_blockbegin(iss, value, sc) long iss, value, sc; {return(0);}
long st_blockend(size) long    size; {return(0);}
void st_blockpop (){}
long st_procend(idn) long    idn; {return(0);}
long st_procbegin (idn) {return(0);}
char *st_sym_idn (idn,sc,st,value,index_) long idn; long *sc, *st, *value, *index_;
 {return(0);}
char *st_str_idn (idn) long idn; {return(0);}
long st_fglobal_idn (idn) long idn; {return(0);}
pSYMR st_psym_idn_offset (idn, offset)long    idn; long    offset; {return(0);}
void st_fixextindex(idn, index_) long idn; long index_; {}
void st_fixextsc(idn, sc) long idn; long sc; {}
long st_pdadd_idn  (idn)long idn; {return(0);}
st_fixiss (idn, iss) long iss; long idn; {}
st_changedn (idn, rfd, index_) {}


/* from  stinternal.c */

st_internal (s, a, b, c, d) char *s; {}


/* from  stio.c */

int st_readbinary (filename, how) char    *filename; char    how; {return(0);}
int st_readst (fn, how, filebase, pchdr, flags) 
 int fn; char how; int filebase; pCHDRR pchdr; int flags; {return(0);}
void st_writebinary (filename, flags) char *filename; {}
char msg_werr[];
char msg_err[];
void st_writest (fn, flags) int     flags; int     fn; {}


/* from  stprint.c */

char     *st_mlang_ascii [];
char     *st_mst_ascii [] ;
char     *st_msc_ascii [];
char     *st_mbt_ascii [];
char     *st_mtq_ascii [];
void st_dump (fd, flags) FILE    *fd; long    flags; {}
func_print_tq(pfd, ppaux, ppaux2, piaux) FILE  *pfd; pAUXU *ppaux, *ppaux2; long  *piaux; {}
void st_printfd (fd, ifd, flags)FILE    *fd; long    ifd; {}


/* from  stwarning.c */

st_warning (s, a, b, c, d) char *s; {}


/* from  swap.c */

void swap_filehdr(pfilehdr, destsex) FILHDR *pfilehdr; long destsex; {}
void swap_aouthdr(paouthdr, destsex) AOUTHDR *paouthdr; long destsex; {}
void swap_scnhdr(pscnhdr, destsex) SCNHDR *pscnhdr; long destsex; {}
void swap_libscn(plibscn, destsex) LIBSCN *plibscn; long destsex; {}
void swap_hdr(phdr, destsex) pHDRR phdr; long destsex; {}
void swap_fd(pfd, count, destsex) pFDR pfd;  long destsex; {}
void swap_fi(pfi, count, destsex) pFIT pfi; long count; long destsex; {}
void swap_sym(psym, count, destsex)pSYMR psym; long count; long destsex; {}
void swap_ext(pext, count, destsex)pEXTR pext; long count; long destsex; {}
void swap_pd(ppd, count, destsex) pPDR ppd; long count; long destsex; {}
void swap_dn(pdn, count, destsex)
 pDNR pdn; long count; long destsex; {}
void swap_rpd(prpd, count, destsex) pRPDR prpd; long count; long destsex; {}
void swap_opt(popt, count, destsex) pOPTR popt; long count; long destsex; {}
void swap_aux(paux, type, destsex) pAUXU paux; long type; long destsex; {}
void swap_reloc(preloc, count, destsex) struct reloc *preloc;long count; long destsex; {}
void swap_ranlib(pranlib, count, destsex)struct ranlib *pranlib; long count; long destsex; {}
void swap_gpt(pgpt, count, destsex) GPTAB *pgpt; long count; long destsex; {}


/* from  vldldptr.c */

int vldldptr(ldptr) LDFILE  *ldptr; {return(0);}

