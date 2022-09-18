/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldfsymorder.c,v 2010.2.1.3 89/11/29 14:28:34 bettina Exp $ */

#include "syms.h"

ldfsymorder (phdr, symptr)

pHDRR	phdr;
long	symptr;

{
    /* check the offsets and lengths to ensure the symbol table is
     *	canonically ordered. returns -1 if they are not and the total
     *	length if they are.
     */

    int	cbOffset;
    int	length;

    cbOffset = symptr + cbHDRR;
    length = 0;

#define CHECK(offset,imax,cb) \
    if (phdr->offset != 0 && phdr->offset != cbOffset + length) \
	return (0); \
    length += phdr->imax * cb; \
    if ((length & 0x3) != 0) \
	return (0);

    CHECK(cbLineOffset,cbLine,sizeof(char));
    CHECK(cbPdOffset,ipdMax,cbPDR);
    CHECK(cbSymOffset,isymMax,cbSYMR);
    CHECK(cbOptOffset,ioptMax,cbOPTR);
    CHECK(cbAuxOffset,iauxMax,cbAUXU);
    CHECK(cbSsOffset,issMax,sizeof(char));
    CHECK(cbSsExtOffset,issExtMax,sizeof(char));
    CHECK(cbFdOffset,ifdMax,cbFDR);
    CHECK(cbRfdOffset,crfd,cbFIT);
    CHECK(cbExtOffset,iextMax,cbEXTR);
    CHECK(cbDnOffset,idnMax,cbDNR);

    return (length);

} /* ldfsymorder */
