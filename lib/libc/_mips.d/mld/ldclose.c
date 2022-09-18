/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldclose.c,v 2010.2.1.3 89/11/29 14:27:44 bettina Exp $ */
/*
* ldclose - close current object file.
*		if current object file is an archive member,
*		set up for next object file from archive.
*
* #ifdef PORTAR		printable ascii headers archive version
* #else #ifdef PORT5AR	UNIX 5.0 semi-portable archive version
* #else			pre UNIX 5.0 (old) archive version
* #endif
*/
#include <stdio.h>
#include <ar.h>
#include "filehdr.h"
#include "syms.h"
#include "ldfcn.h"

extern pCHDRR st_currentpchdr();


int
ldclose(ldptr)
	LDFILE *ldptr;
{
	extern int vldldptr();
	extern int freeldptr();
	extern long sgetl();

#ifdef PORTAR
	struct ar_hdr arhdr;
	long ar_size;

	if (vldldptr(ldptr) == FAILURE)
		return (SUCCESS);
	if (TYPE(ldptr) == ARTYPE &&
		FSEEK(ldptr, -((long)sizeof(arhdr)), BEGINNING) == OKFSEEK &&
		FREADM((char *)&arhdr, sizeof(arhdr), 1, ldptr) == 1 &&
		!strncmp(arhdr.ar_fmag, ARFMAG, sizeof(arhdr.ar_fmag)) &&
		sscanf(arhdr.ar_size, "%ld", &ar_size) == 1)
	{
		/*
		* Be sure OFFSET is even
		*/
		OFFSET(ldptr) += ar_size + sizeof(arhdr) + (ar_size & 01);
		if (ldinitheaders (ldptr) != FAILURE) {
			extern int ldlast_fnum_;
			ldptr->_fnum_ = ++ldlast_fnum_;
			return (FAILURE);
		}
	}
#else
#ifdef PORT5AR
	struct arf_hdr arhdr;
	long ar_size, nsyms;

	if (vldldptr(ldptr) == FAILURE)
		return (SUCCESS);
	if (TYPE(ldptr) == ARTYPE &&
		FSEEK(ldptr, -((long)sizeof(arhdr)), BEGINNING) == OKFSEEK &&
		FREADM((char *)&arhdr, sizeof(arhdr), 1, ldptr) == 1)
	{
		ar_size = sgetl(arhdr.arf_size);
		/*
		* Be sure offset is even
		*/
		OFFSET(ldptr) += ar_size + sizeof(arhdr) + (ar_size & 01);
		if (FSEEK(ldptr, 0L, BEGINNING) == OKFSEEK &&
			FREADM((char *)&(HEADER(ldptr)), FILHSZ, 1, ldptr) == 1)
		{
			return (FAILURE);
		}
	}
#else
	ARCHDR arhdr;

	if (vldldptr(ldptr) == FAILURE)
		return (SUCCESS);
	if (TYPE(ldptr) == ARTYPE &&
		FSEEK(ldptr, -((long)ARCHSZ), BEGINNING) == OKFSEEK &&
		FREADM((char *)&arhdr, ARCHSZ, 1, ldptr) == 1)
	{
		/*
		* Be sure OFFSET is even
		*/
		OFFSET(ldptr) += arhdr.ar_size + ARCHSZ + (arhdr.ar_size & 01);
		if (ldinitheaders (ldptr) == NULL) {
			return (FAILURE);
		}
	}
#endif
#endif
	fclose(IOPTR(ldptr));
	freeldptr(ldptr);
	return (SUCCESS);
}


LDFILE *
ldinitheaders (ldptr)

LDFILE	*ldptr;

{
    if (FSEEK(ldptr, 0L, BEGINNING) == OKFSEEK &&
	FREADM((char *)&(HEADER(ldptr)), FILHSZ, 1, ldptr) == 1) {

	/* check magic numbers */
	if (HEADER(ldptr).f_magic == SMIPSEBMAGIC ||
	    HEADER(ldptr).f_magic == SMIPSEBMAGIC_2 ||
	    HEADER(ldptr).f_magic == SMIPSEBMAGIC_3 ||
	    HEADER(ldptr).f_magic == SMIPSELMAGIC_2 ||
	    HEADER(ldptr).f_magic == SMIPSELMAGIC_3 ||
	    HEADER(ldptr).f_magic == SMIPSELMAGIC) {
	    LDSWAP(ldptr) = 1;
	    swap_filehdr(&(HEADER(ldptr)), gethostsex());
	} else if (HEADER(ldptr).f_magic == MIPSEBMAGIC ||
	    HEADER(ldptr).f_magic == MIPSEBMAGIC_2 ||
	    HEADER(ldptr).f_magic == MIPSEBMAGIC_3 ||
	    HEADER(ldptr).f_magic == MIPSELMAGIC ||
	    HEADER(ldptr).f_magic == MIPSELMAGIC_2 ||
	    HEADER(ldptr).f_magic == MIPSELMAGIC_3 ||
	    HEADER(ldptr).f_magic == MIPSEBUMAGIC ||
	    HEADER(ldptr).f_magic == MIPSELUMAGIC) {
	    LDSWAP(ldptr) = 0;
	} else {
	    LDERROR (ldinitheaders, "magic number incorrect (0x%x)\n",
		HEADER(ldptr).f_magic);
	    fclose(IOPTR(ldptr));
	    freeldptr(ldptr);
	    return (NULL);
	} /* if */

	if (!HEADER(ldptr).f_nsyms || !HEADER(ldptr).f_symptr) {
	    /* no symbol table */
	    PSYMTAB(ldptr) = chdrNil;
	    return (ldptr);

	} else {
	    if (lseek(FILENO(ldptr), OFFSET(ldptr)+HEADER(ldptr).f_symptr, 0)
		>= 0) {

		/* read the symbol table */
		if (st_readst(FILENO(ldptr), 'r', OFFSET(ldptr), 0, 0) == 0) {
		    PSYMTAB(ldptr) = st_currentpchdr();
		    /* after lseek set it right */
		    lseek(FILENO(ldptr), 0, 0);
		    REWIND(ldptr);
		    return (ldptr);
		} /* if */
		/* after lseek set it right */
		lseek(FILENO(ldptr), 0, 0);
		REWIND(ldptr);
	    }
	} /* if */
    } else {
	return (FAILURE);
    } /* if */
    /* no symbol table */
    PSYMTAB(ldptr) = chdrNil;
    return (ldptr);
} /* ldinitheaders */

ldreadst(ldptr,flags)
LDFILE	*ldptr;
{
    if (!PSYMTAB(ldptr))
	return(FAILURE);
    if ((flags & ~PSYMTAB(ldptr)->flags) == 0)
	return(SUCCESS);

    if (st_readst(FILENO(ldptr),'r',OFFSET(ldptr), PSYMTAB(ldptr), flags) == 0) {
	/* after lseek set it right */
	lseek(FILENO(ldptr), 0, 0);
	REWIND(ldptr);
	return(SUCCESS);
    }
    return(FAILURE);
}
static char ID[] = "@(#) ldclose.c: 1.3 2/16/83";
