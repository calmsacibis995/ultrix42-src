/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: stio.c,v 2010.3.1.3 89/11/29 14:29:39 bettina Exp $ */
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
#include "sex.h"

extern int errno;

typedef char	CHART, *pCHART;

#define MAX(a,b) (a < b ? b : a)
#define VALIDSTAMP ((MS_STAMP << 8)|LS_STAMP)
#define cbCHART sizeof(CHART)
#define ST_LNCOUNTMAX 8

extern pCHDRR	st_pchdr;			/* comp time hdr */

#define pssExt pssext
#define cssExt cbssext

export int st_readbinary (filename, how)

char	*filename;
char	how;

{
    int	fn;
    int	result;

    /* dumb interface just gets name, opens file and calls st_readst to
     *	read the symbol table, closes the file and returns the result.
     */
    if ((fn = open (filename, 0, 0)) < 0)
	    return (-2);
    result = st_readst (fn, how, 0, 0, -1);
    close (fn);

    return (result);
} /* st_readbinary */


export int st_readst (fn, how, filebase, pchdr, flags)

int fn;
char how;
int filebase;
pCHDRR pchdr;
int flags;

{
    HDRR	hdr;
    pCFDR	pcfd;
    int		ifd;
    int		fappend;	/* set if we intend to append to symbol table */
    int		d1, d2;		/* temps */
    int		fordered;	/* set if symtable is ordered canonically */
    long	symptr;
    int		dummy;
    char	*ppackedlines = 0;

    /* 'r' mean read_only, this only affects allocation */
    fappend = (how == 'r') ? 0 : 1;
    if (fappend)
	flags = -1;	/* must read all if you're going to change it */

    if (pchdr == 0) {
	/* allocate it */
	st_pchdr = (pCHDRR) calloc (cbCHDRR, 1);
    } else {
	st_pchdr = pchdr;
    } /* if */

    /* only read those that haven't been read */
    flags = flags & ~st_pchdr->flags;

    /* seek and read the header */
    if (pchdr == 0 || (flags & ST_PHEADER) != 0) {
	symptr = lseek(fn, 0, 1) - filebase;
	flags |= ST_PHEADER|ST_PFDS;	/* have to read these */
	if (read (fn, (char *)&hdr, cbHDRR) != cbHDRR)
		return (-3);

	if (hdr.magic != magicSym) {
	    if (hdr.magic == swap_half(magicSym)) {
		swap_hdr(&hdr, gethostsex());
		st_pchdr->fswap = 1;
	    } else {
		st_error("bad magic in hdr. expected 0x%x, got 0x%x\n", 
		    magicSym, hdr.magic);
	    } /* if */
	} /* if */

	if (hdr.vstamp < ((1<<8)|30) && hdr.idnMax != 0)
	    st_internal("st_readst: dense number incompatible from versions less than 1.30, please recompile from scratch and use compatible components\n");

#ifdef CMPLRS
	/* check the stamps for validity */
	if (hdr.vstamp != VALIDSTAMP)
	    st_warning ("file's version stamp (%d.%d) is not current (%d.%d)", 
		(hdr.vstamp>>8)&0xff, hdr.vstamp&0xff, MS_STAMP, LS_STAMP);
#endif

	if (flags == -1)
	    fordered = ldfsymorder(&hdr, symptr);
	else
	    fordered = 0;
	st_pchdr->hdr = hdr;
    } else {
	fordered = 0;
	hdr = st_pchdr->hdr;
    } /* if */




    /* malloc the space for out incoming tables */

#define MALLOC(count,cb,ptr,type,stflag) \
    if ((flags & stflag) != 0 && st_pchdr->ptr == 0) \
	st_pchdr->ptr = (type) st_malloc((char *)st_pchdr->ptr,  \
	    &dummy, cb, count);

    MALLOC (hdr.isymMax,cbSYMR,psym,pSYMR,ST_PSYMS);
    MALLOC (hdr.iauxMax,cbAUXU,paux,pAUXU,ST_PAUXS);
    MALLOC (hdr.issMax,cbCHART,pss,pCHART,ST_PSSS);
    if ((flags&ST_PLINES) != 0)
	ppackedlines = st_malloc(ppackedlines, &dummy, cbCHART, hdr.cbLine);
    MALLOC (hdr.ilineMax,cbLINER,pline,pLINER,ST_PLINES);
    MALLOC (hdr.ioptMax,cbOPTR,popt,pOPTR,ST_POPTS);
    MALLOC (hdr.crfd,cbRFDT,prfd,pRFDT,ST_PRFDS);
    MALLOC (hdr.ipdMax,cbPDR,ppd,pPDR,ST_PPDS);
    /* +1 so as has room for mcount */
    MALLOC (hdr.iextMax+1,cbEXTR,pext,pEXTR,ST_PEXTS); 
    MALLOC (hdr.issExtMax+8,cbCHART,pssExt,pCHART,ST_PSSEXTS);
    MALLOC (hdr.idnMax,cbDNR,pdn,pDNR,ST_PDNS);
    MALLOC (hdr.ifdMax,cbFDR,pfd,pFDR,ST_PFDS);
    if ((flags&ST_PFDS) != 0 && st_pchdr->pcfd == 0) {
	MALLOC (hdr.ifdMax,cbCFDR,pcfd,pCFDR,ST_PFDS);
	bzero(st_pchdr->pcfd, cbCFDR * hdr.ifdMax);
    } /* if */
    
    if ((flags&ST_PEXTS) != 0)
	st_pchdr->cext = st_pchdr->cextMax = hdr.iextMax;
    if ((flags&ST_PSSEXTS) != 0)
	st_pchdr->cbssext = st_pchdr->cbssextMax = hdr.issExtMax;
    if ((flags&ST_PDNS) != 0)
	st_pchdr->cdn = st_pchdr->cdnMax = hdr.idnMax;
    if ((flags&ST_PFDS) != 0)
	st_pchdr->cfd = st_pchdr->cfdMax = hdr.ifdMax;

    /* read in the tables */


#define READ(offset,ptr,size,stflag) \
    if ((flags&stflag) != 0) \
	if ((d1 = st_read (fn, fordered, offset + filebase, ptr, size)) != 0) \
	    return (d1);
		    
    READ (hdr.cbLineOffset, (char *)ppackedlines, hdr.cbLine,ST_PLINES);
    READ (hdr.cbPdOffset, (char *)st_pchdr->ppd, hdr.ipdMax * cbPDR,ST_PPDS);
    READ (hdr.cbSymOffset, (char *)st_pchdr->psym, hdr.isymMax*cbSYMR,ST_PSYMS);
    READ (hdr.cbOptOffset, (char *)st_pchdr->popt, hdr.ioptMax*cbOPTR,ST_POPTS);
    READ (hdr.cbAuxOffset, (char *)st_pchdr->paux, hdr.iauxMax*cbAUXU,ST_PAUXS);
    READ (hdr.cbSsOffset, (char *)st_pchdr->pss, hdr.issMax,ST_PSSS);

    READ (hdr.cbSsExtOffset, (char *)st_pchdr->pssext,hdr.issExtMax,ST_PSSEXTS);
    READ (hdr.cbFdOffset, (char *)st_pchdr->pfd, hdr.ifdMax * cbFDR,ST_PFDS);
    READ (hdr.cbRfdOffset, (char *)st_pchdr->prfd, hdr.crfd * cbRFDT,ST_PRFDS);
    READ (hdr.cbExtOffset, (char *)st_pchdr->pext, hdr.iextMax*cbEXTR,ST_PEXTS);
    READ (hdr.cbDnOffset, (char *)st_pchdr->pdn, hdr.idnMax * cbDNR,ST_PDNS);
    if (st_pchdr->fswap) {
	if ((flags&ST_PPDS) != 0)
	    swap_pd (st_pchdr->ppd, hdr.ipdMax, gethostsex());
	if ((flags&ST_PSYMS) != 0)
	    swap_sym (st_pchdr->psym, hdr.isymMax, gethostsex());
	if ((flags&ST_POPTS) != 0)
	    swap_opt (st_pchdr->popt, hdr.ioptMax, gethostsex());
	if ((flags&ST_PFDS) != 0)
	    swap_fd (st_pchdr->pfd, hdr.ifdMax, gethostsex());
	if ((flags&ST_PRFDS) != 0)
	    swap_fi (st_pchdr->prfd, hdr.crfd, gethostsex());
	if ((flags&ST_PEXTS) != 0)
	    swap_ext (st_pchdr->pext, hdr.iextMax, gethostsex());
	if ((flags&ST_PDNS) != 0)
	    swap_dn (st_pchdr->pdn, hdr.idnMax, gethostsex());
    } /* if */

    /* initialize the file descriptors and their runtime fields */


    for (ifd = 0; ifd < hdr.ifdMax; ifd++) {

	    pcfd = st_pcfd_ifd (ifd);
	    pcfd->pfd = st_pchdr->pfd + ifd;
	    /* set the pointer field to point at the correct part in
	     *	memory for that table in the current file. If there
	     *	are entries, set the pointer and flag, saying it
	     *	was read in from disk-- we cannot add to tables read in
	     *	from disk.
	     */
	    if ((flags&ST_PSYMS) != 0 && pcfd->pfd->csym > 0) {
		pcfd->psym = st_pchdr->psym + pcfd->pfd->isymBase;
		pcfd->freadin |= ST_PSYMS;
	    }; /* if */
	    if ((flags&ST_PAUXS) != 0 && pcfd->pfd->caux > 0) {
		pcfd->paux = st_pchdr->paux + pcfd->pfd->iauxBase;
		pcfd->freadin |= ST_PAUXS;
	    }; /* if */
	    if ((flags&ST_POPTS) != 0 && pcfd->pfd->copt > 0) {
		pcfd->popt = st_pchdr->popt + pcfd->pfd->ioptBase;
		pcfd->freadin |= ST_POPTS;
	    }; /* if */
	    if ((flags&ST_PRFDS) != 0 && pcfd->pfd->crfd > 0) {
		pcfd->prfd = st_pchdr->prfd + pcfd->pfd->rfdBase;
		pcfd->freadin |= ST_PRFDS;
	    }; /* if */
	    if ((flags&ST_PSSS) != 0 && pcfd->pfd->cbSs > 0) {
		pcfd->pss = st_pchdr->pss + pcfd->pfd->issBase;
		pcfd->freadin |= ST_PSSS;
	    }; /* if */
	    if ((flags&ST_PPDS) != 0 && pcfd->pfd->cpd > 0) {
		pcfd->ppd = st_pchdr->ppd + pcfd->pfd->ipdFirst;
		pcfd->freadin |= ST_PPDS;
	    }; /* if */
	    if ((flags&ST_PLINES) != 0 && pcfd->pfd->cline > 0) {
		pcfd->pline = st_pchdr->pline + pcfd->pfd->ilineBase;
		pcfd->freadin |= ST_PLINES;
	    }; /* if */

    } /* if */

    /* expand the line numbers */

    if ((flags&ST_PLINES) != 0) for (ifd = hdr.ifdMax - 1; ifd >= 0; ifd--) {
	pPDR		ppd;
	int		ipd;
	int		iline;
	int		ln;			/* current line number */
	int		ilineMax;		/* last iline+1 for this proc */
	char		*pline;			/* pointer to packed lines */

	pcfd = st_pcfd_ifd (ifd);
	if (pcfd->pfd->cline == 0)
	    continue;
	/* loop through each procedure moving its line entries to their
	 *	proper position by unpacking it from the input buffer.
	 *	An entry on disk is a byte long. The upper bits are the
	 *	count (-1) and lower bits are the delta from the previous
	 *	line number. If the delta is -8, then it is too big and we
	 *	get it from the next 16 bits.
	 */
	pline = ppackedlines + pcfd->pfd->cbLineOffset;
	for (ipd = 0; ipd < pcfd->pfd->cpd; ipd ++) {
	    char	count;		/* count part of packed line number */
	    char	clinedelta;	/* delta part of packed line number */
	    short	linedelta;	/* delta part of packed line number */
	    int		ln;		/* current line number */
	    int		ipd1;

	    ppd = pcfd->ppd + ipd;

	    if (ppd->iline == ilineNil || ppd->lnLow == -1 ||
		ppd->lnHigh == -1)
		continue;

	    /* get stopping point for line number loop */
	    ipd1 = 1;
next:
	    if (ipd + ipd1 >= pcfd->pfd->cpd)
		ilineMax = pcfd->pfd->cline;
	    else if (ppd[ipd1].iline == -1 ||
	    	     (ppd[ipd1].lnHigh == -1 && ppd[ipd1].lnLow != -1)) {
		ipd1++;		/* aent have .iline !=0, lnHigh==-1 */
		goto next;
	    } else
		ilineMax = ppd[ipd1].iline;

	    ln = ppd->lnLow;
	    for (iline = ppd->iline; iline < ilineMax;) {
		count = *pline & 0x0f;
		clinedelta = *pline++ >> 4;
		if (clinedelta == -8) {
		    linedelta = (*pline << 8) | (pline[1] & 0xff);
		    pline += 2;
		    ln += linedelta;
		} else {
		    ln += clinedelta;
		} /* if */
		for (; count >= 0; count --)
		    pcfd->pline[iline++] = ln;
	    } /* for */
	} /* for */
    } /* for */

    st_pchdr->flags |= flags;
    return (0);

} /* st_readst */


static st_read (fd, ordered, offset, ptr, size)

FILE	*fd;
int	offset;
char	*ptr;
int	size;
int	ordered;

{
    /* seek to correct location if need and read */
    if (size == 0)
	return (0);

    if (!ordered) {
	if (lseek (fd, offset, 0) != offset) {
	    st_warning ("st_read: error seeking\n");
	    return (-5);
	} /* if */
    } /* if */
    if (read  (fd, (char *) ptr, size) != size) {
	st_warning ("st_read: error reading\n");
	return (-6);
    } /* if */
    return (0);
} /* st_read */


export void st_writebinary (filename, flags)

int		flags;
char *		filename;	/* file to put it out to */

{
    int		fn;

    fn = open (filename, O_CREAT|O_RDWR, 0666);
    if (fn < 0) {
 	st_error ("cannot open symbol table file %s\n", filename);
    } /* if */
    st_writest (fn, flags);
    close (fn);

} /* writebinary */



/* macro to write fields of the file descriptor table */
#define ipdBase ipdFirst
#define irfdMax crfd
#define irfdBase rfdBase

char msg_werr[] = "cannot write pfield";

#define WRITE(B,M,cf,pf) \
 cbOffset += cbtotal, cbtotal=0; \
 for (ifd =0; ifd<ifdmax; ifd++) { \
	pcfd=st_pcfd_ifd(ifd); pfd = pcfd->pfd; pfd->B=fd.B+fd.cf;\
	cbtotal += cb = pfd->cf*sizeof(*pcfd->pf); \
	if (pfd->cf && fwrite((char *)(pcfd->pf),\
	sizeof(*pcfd->pf),pfd->cf,file) != pfd->cf) st_error(msg_werr); \
	fd.B=fd.B+fd.cf; hdr.M += fd.cf = pfd->cf; }

#define ROUNDIT(table) \
    if ((((unsigned)-cbtotal)&3) != 0) { \
	if (fwrite(zero, 1, (((unsigned)-cbtotal)&3), file) != (((unsigned)-cbtotal)&3)) \
	    st_error("cannot write round bytes for table\n"); \
	cbtotal = (cbtotal + 3) & ~3; \
    } /* if */
    
char msg_err[] = "cannot write cur table\n";

#define WRITEIT(M,Of,cu,pu) \
    hdr.M=st_pchdr->cu; cbOffset += cbtotal; cbtotal=0; \
    if (hdr.M) { \
	cbtotal=st_pchdr->cu * sizeof(*st_pchdr->pu); hdr.Of=cbOffset; \
	if(st_pchdr->cu && fwrite((char *)(st_pchdr->pu),sizeof(*st_pchdr->pu),st_pchdr->cu,file)!=st_pchdr->cu) \
	st_error(msg_err); }


export void st_writest (fn, flags)

int	flags;
int	fn;
{
    /* dump the symbol table in binary */
    static	char zero[4];	/* permanent zero buf for rounding */
    pCFDR	pcfd;
    FDR		fd;
    pFDR	pfd;
    HDRR	hdr;
    int		cb;		/* count bytes for current file's table */
    int		cbtotal;	/* count bytes for all file's table */
    int		cbOffset;	/* current file offset */
    int		ifd;
    int		ifdmax;
    int		hdroffset;	/* offset for hdr in file */
    FILE	*file;


    /* initialize stuff */
    cbtotal = 0;
    bzero (&fd, cbFDR);
    bzero (&hdr, cbHDRR);
    ifdmax = st_ifdmax();
    hdroffset = lseek (fn, 0, 1);
    cbOffset = lseek (fn, cbHDRR, 1);
    st_pchdr->cfd = ifdmax;
    file = fdopen (fn, "w");
    if (file == NULL)
	st_error("st_writest: cannot write to file number %d\n", fn);


    if (flags & ST_PLINES) {
	int	ipd;
	int	ipd1;
	int	ilineMax;
	pPDR	ppd;

	cbtotal = 0;
	for (ifd = 0; ifd < ifdmax; ifd++) {
	    cb = 0;			/* count of bytes for the file */
	    pcfd = st_pcfd_ifd (ifd);
	    pfd = pcfd->pfd;

	    if (pfd->cline == 0 || pcfd->pline == 0)
		continue;

	    hdr.ilineMax += pfd->cline;
	    pfd->cbLineOffset = cbtotal;
	    /* loop through each procedure, pack the info as speced in
	     * the read routine.
	     */
	    for (ipd = 0; ipd < pfd->cpd; ipd++) {
		
		short	linedelta;	/* current linedelta */
		int	sign;		/*  hold the sign bit */
		short	olinedelta = 0;	/* last line delta */
		char	clinedelta;	/* char version of olinedelta */
		int	ln;		/* current line */
		int	count;		/* count of same lines in a row */
		pLINER	pline;		/* point to in memory line array */
		char	buf [128];	/* buffer to pack lines */
		char	*pbuf;		/* traverse buf with this */

		ppd = pcfd->ppd + ipd;
		if (ppd->iline == ilineNil || ppd->lnLow == -1 ||
		    ppd->lnHigh == -1)
		    continue;

		ppd->cbLineOffset = cbtotal - pfd->cbLineOffset;
		/* get the iline from thext proc to bound the number of line
		 *	entries we deal with.
		 */
		ipd1 = 1;
next:
		if (ipd + ipd1 >= pfd->cpd)
		    ilineMax = pcfd->pfd->cline;
		else if (ppd[ipd1].iline == -1 ||
			(ppd[ipd1].lnHigh == -1 && ppd[ipd1].lnLow != -1)) {
		    ipd1++;
		    goto next;
		} else
		    ilineMax = ppd[ipd1].iline;

		if (ilineMax <= 0 || ppd->iline < 0)
		    continue;

		pline = &pcfd->pline[ppd->iline];
		ln = ppd->lnLow;
		count = -1;
		olinedelta = 0;
		for (pbuf = buf; pline <= pcfd->pline + ilineMax; pline++) {

		    /* check if we have room in the buffer */

		    if (pline == pcfd->pline + ilineMax)
			/* last time through just to force out the last lines */
			linedelta = 1; 
		    else
			linedelta = (*pline == 0 ? ppd->lnLow : *pline) - ln;
		    if (linedelta != 0 || count == ST_LNCOUNTMAX) {
			sign = olinedelta >= 0 ? 0 : 1;
			if (count != -1 && olinedelta >= -7 && olinedelta < 8) {
			    clinedelta = olinedelta;
			    *pbuf++ = (clinedelta << 4) | count;
			} else if (count != -1) {
			    *pbuf = count;
			    *pbuf++ |= 0x80;
			    *pbuf++ = olinedelta  >> 8;
			    *pbuf++ = olinedelta & 0xff;
			} /* if */
			count = 0;
			olinedelta = linedelta;
			ln += linedelta;
		    } else {
			count++;
		    } /* if */

		    if (pbuf + 3 >= &buf[sizeof(buf)] ||
			(pline == pcfd->pline + ilineMax && pbuf - buf > 0)) {
			fwrite (buf, pbuf - buf, sizeof(char), file);
			cbtotal += pbuf - buf;
			cb += pbuf - buf;
			pbuf = buf;
		    } /* if */
		} /* foreach line in a proc */

	    } /* for each  proc */
	    pfd->cbLine = cb;
	} /* for each file */
		    

	if (hdr.ilineMax) {
	    ROUNDIT(lines);
	    hdr.cbLineOffset = cbOffset;
	    hdr.cbLine = cbtotal;
	} /* if */
    } /* if */
    if (flags & ST_PPDS) {
	WRITE(ipdBase,ipdMax,cpd,ppd);
	if (hdr.ipdMax) hdr.cbPdOffset = cbOffset;
    } /* if */
    if (flags & ST_PSYMS) {
	WRITE(isymBase,isymMax,csym,psym);
	if (hdr.isymMax) hdr.cbSymOffset = cbOffset;
    } /* if */
    if (flags & ST_POPTS) {
	WRITE(ioptBase,ioptMax,copt,popt);
	if (hdr.ioptMax) hdr.cbOptOffset = cbOffset;
    } /* if */
    if (flags & ST_PAUXS) {
	WRITE(iauxBase,iauxMax,caux,paux);
	if (hdr.iauxMax) hdr.cbAuxOffset = cbOffset;
    } /* if */

    if (flags & ST_PSSS) {
	WRITE (issBase,issMax,cbSs,pss);
	/* round */
	ROUNDIT(strings);
	hdr.issMax = cbtotal;
	if (hdr.issMax) hdr.cbSsOffset = cbOffset;
    } /* if */
    if (flags & ST_PSSEXTS) {
	WRITEIT (issExtMax,cbSsExtOffset,cbssext,pssExt);
	/* round */
	ROUNDIT(strings);
	hdr.issExtMax = cbtotal;
    } /* if */

    if (flags & ST_PFDS) {
	WRITEIT (ifdMax,cbFdOffset,cfd,pfd);
    } /* if */
    if (flags & ST_PRFDS) {
	WRITE(irfdBase,irfdMax,crfd,prfd);
	if (hdr.crfd) hdr.cbRfdOffset = cbOffset;
    } /* if */
    /* write out externals */
    if (flags & ST_PEXTS) {
	WRITEIT (iextMax,cbExtOffset,cext,pext);
    } /* if */

    if ((flags & ST_PDNS) && st_pchdr->cdn != 0) {
	st_pchdr->pdn[0].rfd = 0;
	st_pchdr->pdn[0].index = 0;
	st_pchdr->pdn[1].rfd = 0;
	st_pchdr->pdn[1].index = 0;
	WRITEIT (idnMax,cbDnOffset,cdn,pdn);
    } /* if */

    /* go back and write the header */
    fflush (file);
    fseek (file, hdroffset, 0);
    hdr.vstamp = VALIDSTAMP;
    hdr.magic = magicSym;
    if (fwrite ((char *)&hdr, 1, cbHDRR, file) != cbHDRR)
	st_error ("cannot write symbol header\n");

    fflush (file);
    /* NOW a KLUDGE, there is no libc call to free a stdio iop structure
     *	so until, we get fdclose, here it is.
     */
    memset(file, 0, sizeof(*file));

} /* st_writebinary */
