/*	@(#)ldfcn.h	4.3	ULTRIX	9/4/90	*/
#include <ansi_compat.h>
#ifdef __mips
/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldfcn.h,v 2010.5.1.5 89/11/29 22:40:47 bettina Exp $ */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	The following two declarations appear in the IH versions of
 *	"stdio.h" but do not appear in the normal 1.2 versions.
 */
#ifdef __mips
long ftell();
char *fgets();
#endif /* __mips */

#ifndef LDFILE
struct	ldfile {
	int		_fnum_;	/* so each instance of an LDFILE is unique */
	FILE		*ioptr;	/* system I/O pointer value */
	long		offset;	/* absolute offset to the start of the file */
	FILHDR		header;	/* the file header of the opened file */
#ifdef __mips
	pCHDRR		pchdr;  /* pointer to the symbol table */
	long		lastindex; /* index of last symbol accessed */
	unsigned short	type;	/* indicator of the type of the file */
	unsigned	fswap : 1;	/* if set, we must swap */
	unsigned	fBigendian : 1;	/* if set, we must swap aux for the
					 * last retrieved symbol
					 */
#else
	unsigned short	type;		/* indicator of the type of the file */
#endif /* __mips */
};


/*
	provide a structure "type" definition, and the associated
	"attributes"
*/

#define	LDFILE		struct ldfile
#define IOPTR(x)	x->ioptr
#define OFFSET(x)	x->offset
#define TYPE(x)		x->type
#define	HEADER(x)	x->header
#define LDFSZ		sizeof(LDFILE)
#ifdef __mips
#define PSYMTAB(x)	x->pchdr
#define SYMTAB(x)	(ldreadst(x, -1), x->pchdr)
#define SYMHEADER(x)	x->pchdr->hdr
#define PFD(x)		x->pchdr->pfd
#define LDSWAP(x)	x->fswap
#define LDAUXSWAP(x,ifd) (PFD(x)[ifd].fBigendian != x->fBigendian)
#define LDERROR(x,y,z)  fprintf(stderr,"x: "); fprintf (stderr, y, z);
#endif /* __mips */

/*
	define various values of TYPE(ldptr)
*/

#define ARTYPE 	0177545
#define ISARCHIVE(x) ((x) == ARTYPE)

/*
	define symbolic positioning information for FSEEK (and fseek)
*/

#define BEGINNING	0
#define CURRENT		1
#define END		2

/*
	define a structure "type" for an archive header
*/

typedef struct
{
	char ar_name[16];
	long ar_date;
	int ar_uid;
	int ar_gid;
	long ar_mode;
	long ar_size;
} archdr;

#define	ARCHDR	archdr
#define ARCHSZ	sizeof(ARCHDR)


/*
	define some useful symbolic constants
*/

#define SYMTBL	0	/* section nnumber and/or section name of the Symbol Table */

#define	SUCCESS	 1
#define	CLOSED	 1
#define	FAILURE	 0
#define	NOCLOSE	 0
#define	BADINDEX	-1L

#define	OKFSEEK	0

/*
	define macros to permit the direct use of LDFILE pointers with the
	standard I/O library procedures
*/

LDFILE *ldopen();
LDFILE *ldaopen();
#ifdef __mips
LDFILE *ldinitheaders();
#endif /* __mips */

#define GETC(ldptr)	getc(IOPTR(ldptr))
#define GETW(ldptr)	getw(IOPTR(ldptr))
#define FEOF(ldptr)	feof(IOPTR(ldptr))
#define FERROR(ldptr)	ferror(IOPTR(ldptr))
#define FGETC(ldptr)	fgetc(IOPTR(ldptr))
#define FGETS(s,n,ldptr)	fgets(s,n,IOPTR(ldptr))
#define FILENO(ldptr)	fileno(IOPTR(ldptr))
#define FREADM(p,s,n,ldptr)	fread(p,s,n,IOPTR(ldptr))
#define FSEEK(ldptr,o,p)	fseek(IOPTR(ldptr),(p==BEGINNING)?(OFFSET(ldptr)+o):o,p)
#define FTELL(ldptr)	ftell(IOPTR(ldptr))
#define FWRITEM(p,s,n,ldptr)       fwrite(p,s,n,IOPTR(ldptr))
#define REWIND(ldptr)	rewind(IOPTR(ldptr))
#define SETBUF(ldptr,b)	setbuf(IOPTR(ldptr),b)
#define UNGETC(c,ldptr)		ungetc(c,IOPTR(ldptr))
#ifndef __mips
#define STROFFSET(ldptr)	(HEADER(ldptr).f_symptr + HEADER(ldptr).f_nsyms * 18) /* 18 == SYMESZ */
#else /* mips */
#define STROFFSET(ldptr)	(SYMHEADER(ldptr).cbSsOffset)
#endif /* __mips */
#endif
#endif /* __mips */
