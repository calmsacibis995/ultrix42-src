/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldopen.c,v 2010.2.1.3 89/11/29 14:28:04 bettina Exp $ */
/*
* ldopen - get LDFILE, header info for object file.
*		if it is an archive, get the first file from the archive.
*		if it is an already opened archive, assume ldclose() set
*		up everything already.
*
* #ifdef PORTAR		printable ascii header archive version
*/
#include <stdio.h>
#include <ar.h>
#include "filehdr.h"
#include "syms.h"
#include "ldfcn.h"


LDFILE *
ldopen(filename, ldptr)
	char *filename;
	LDFILE *ldptr; 
{
    extern int vldldptr();
    extern LDFILE *allocldptr();
    extern int freeldptr();
    extern long sgetl();
    FILE *ioptr;
    unsigned short type;
    struct ar_hdr arbuf;
    char buf[SARMAG];

    if (vldldptr(ldptr) == FAILURE)
    {
	if ((ioptr = fopen(filename, "r+")) == NULL)
	    if ((ioptr = fopen(filename, "r")) == NULL) {
		    LDERROR (ldopen, "cannot open %s\n", filename);
		    return (NULL);
	    }

	if (fread(buf, sizeof(char) * SARMAG, 1, ioptr) != 1)
		buf[0] = '\0';
	fseek(ioptr, 0L, 0);
	if (fread((char *)&type, sizeof(type), 1, ioptr) != 1 ||
		(ldptr = allocldptr()) == NULL)
	{
		fclose(ioptr);
		LDERROR (ldopen, "cannot read magic number %s\n", filename);
		return (NULL);
	}
	if (strncmp(buf, ARMAG, SARMAG) == 0)
	{
	    long ar_size;

	    TYPE(ldptr) = (unsigned short)ARTYPE;
	    if (fseek(ioptr, (long)(sizeof(char) * SARMAG), 0) ==
		OKFSEEK &&
		fread((char *)&arbuf, sizeof(arbuf), 1, ioptr)
		== 1 &&
		!strncmp(arbuf.ar_fmag, ARFMAG,
		sizeof(arbuf.ar_fmag)) &&
		AR_ISSYMDEF(arbuf.ar_name) &&
		sscanf(arbuf.ar_size, "%ld", &ar_size) == 1) {
		    OFFSET(ldptr) = sizeof(char) * SARMAG +
			    2 * sizeof(struct ar_hdr) +
			    ((ar_size + 01) & ~01);
	    } else {
		    OFFSET(ldptr) = sizeof(char) * SARMAG +
			    sizeof(struct ar_hdr);
	    } /* if */

	} else {
		TYPE(ldptr) = type;
		OFFSET(ldptr) = 0L;
	} /* if */

	IOPTR(ldptr) = ioptr;
	if (ldinitheaders(ldptr) != NULL)
	    return (ldptr);

    } else if (FSEEK(ldptr, 0L, BEGINNING) == OKFSEEK)
	return (ldptr);

    fclose(IOPTR(ldptr));
    freeldptr(ldptr);
    return (NULL);

} /* ldopen */

static char ID[] = "@(#) ldopen.c: 1.3 2/16/83";
