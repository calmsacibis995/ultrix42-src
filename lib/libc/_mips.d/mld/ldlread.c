/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldlread.c,v 2010.2.1.3 89/11/29 14:27:52 bettina Exp $ */
#include	<stdio.h>
#include	"filehdr.h"
#include	"scnhdr.h"
#include	"syms.h"
#include	"ldfcn.h"

static long		lnnoptr = 0L;
static unsigned short	maxlnnos = 0;
static LDFILE		*saveldptr = NULL;
static long		size;
static long		lnLow;			/* procedure's line base */


int
ldlread(ldptr, fcnindx, linenum, linent)

LDFILE		*ldptr;
long		fcnindx;
unsigned short	linenum;
LINER		*linent;

{
    extern int	ldlinit( );
    extern int	ldlitem( );


    if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
	    return (NULL);
    if (ldlinit(ldptr, fcnindx) == SUCCESS) {
	return(ldlitem(ldptr, linenum, linent));
    }

    return(FAILURE);

}


int
ldlinit(ldptr, fcnindx)

LDFILE		*ldptr;
long		fcnindx;

{

    long		endlnptr;
    pPDR		pd;

    extern int		ld_ifd_symnum ();
    int			ifd = ld_ifd_symnum (ldptr, fcnindx);
    pPDR		st_ppd_ifd_isym();

    if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
	    return (FAILURE);
    if (ldreadst(ldptr, ST_PFDS|ST_PPDS|ST_PLINES) == FAILURE)
	return(FAILURE);
    saveldptr = ldptr;

    pd = st_ppd_ifd_isym (ifd, fcnindx - PFD(ldptr)[ifd].ipdFirst);
    if (pd != pdNil && pd->iline != ilineNil) {
	lnnoptr = pd->iline + PFD(ldptr)[ifd].ilineBase;
	lnLow = pd->lnLow;
	if (pd == PSYMTAB(ldptr)->ppd + 
	    PFD(ldptr)[ifd].ipdFirst + PFD(ldptr)[ifd].cpd)
	    maxlnnos = lnnoptr + PFD(ldptr)[ifd].cline;
	else
	    maxlnnos = lnnoptr + (pd[1].iline - pd->iline);
	return (SUCCESS);

      }

    lnnoptr = 0L;
    maxlnnos = 0;
    saveldptr = NULL;
    return(FAILURE);

}




int
ldlitem(ldptr, linenum, linent)

LDFILE		*ldptr;
unsigned short	linenum;
LINER		*linent;

{
    LINER		line;
    char		lineCh;
    int			lflag;
    unsigned short	i;


    if (vldldptr(ldptr) != SUCCESS || PSYMTAB(ldptr) == NULL)
	    return (NULL);
    if (ldreadst(ldptr, ST_PFDS|ST_PPDS|ST_PLINES) == FAILURE)
	return(NULL);
    lflag = FAILURE;

    if (linenum >= 0 && (ldptr == saveldptr) && (lnnoptr != 0)) {

	for (i = lnnoptr; i < maxlnnos; i++) {
	    line = PSYMTAB(ldptr)->pline[lnnoptr];
	    line += lnLow;
	    if (line == linenum) {
		*linent = line;
		return(SUCCESS);
	    } else if (line > linenum &&
		((lflag == FAILURE) || (*linent > line))) {
		lflag = SUCCESS;
		*linent = line;
	    } /* if */
	} /* for */
    } /* if */

    return(lflag);
}

static char	ID[ ] = "@(#) ldlread.c: 1.1 1/7/82";
