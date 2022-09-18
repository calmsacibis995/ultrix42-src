/* 	@(#)a.out.h	4.3	(ULTRIX)	9/4/90	*/
/************************************************************************
 *									*
 *			Copyright (c) 1986,1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
#include <ansi_compat.h>
#ifdef __vax
/*
 *
 *   Modification History:
 *
 * 02 Apr 86 -- depp
 *	Moved struct exec and associated magic numbers to <sys/exec.h>
 *
 */
/*	a.out.h	4.1	83/05/03	*/

/*
 * Include file that now contains struct exec and the associate magic numbers,
 * since they are now shared with the kernel.
 */
#include <sys/exec.h>

/*
 * Macros which take exec structures as arguments and tell whether
 * the file has a reasonable magic number or offsets to text|symbols|strings.
 */
#define	N_BADMAG(x) \
(((x).a_magic)!=OMAGIC && ((x).a_magic)!=NMAGIC && ((x).a_magic)!=ZMAGIC)

#define	N_TXTOFF(x) \
	((x).a_magic==ZMAGIC ? 1024 : sizeof (struct exec))
#define N_SYMOFF(x) \
	(N_TXTOFF(x) + (x).a_text+(x).a_data + (x).a_trsize+(x).a_drsize)
#define	N_STROFF(x) \
	(N_SYMOFF(x) + (x).a_syms)

/*
 * Format of a relocation datum.
 */
struct relocation_info {
	int	r_address;	/* address which is relocated */
unsigned int	r_symbolnum:24,	/* local symbol ordinal */
		r_pcrel:1, 	/* was relocated pc relative already */
		r_length:2,	/* 0=byte, 1=word, 2=long */
		r_extern:1,	/* does not include value of sym referenced */
		:4;		/* nothing, yet */
};

/*
 * Format of a symbol table entry; this file is included by <a.out.h>
 * and should be used if you aren't interested the a.out header
 * or relocation information.
 */
struct	nlist {
	union {
		char	*n_name;	/* for use when in-core */
		long	n_strx;		/* index into file string table */
	} n_un;
unsigned char	n_type;		/* type flag, i.e. N_TEXT etc; see below */
	char	n_other;	/* unused */
	short	n_desc;		/* see <stab.h> */
unsigned long	n_value;	/* value of this symbol (or sdb offset) */
};
#define	n_hash	n_desc		/* used internally by ld */

/*
 * Simple values for n_type.
 */
#define	N_UNDF	0x0		/* undefined */
#define	N_ABS	0x2		/* absolute */
#define	N_TEXT	0x4		/* text */
#define	N_DATA	0x6		/* data */
#define	N_BSS	0x8		/* bss */
#define	N_COMM	0x12		/* common (internal to ld) */
#define	N_FN	0x1f		/* file name symbol */

#define	N_EXT	01		/* external bit, or'ed in */
#define	N_TYPE	0x1e		/* mask for all the type bits */

/*
 * Sdb entries have some of the N_STAB bits set.
 * These are given in <stab.h>
 */
#define	N_STAB	0xe0		/* if any of these bits set, a SDB entry */

/*
 * Format for namelist values.
 */
#define	N_FORMAT	"%08x"
#endif
#ifdef __mips
/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: a.out.h,v 2010.5.1.5 89/11/29 22:40:53 bettina Exp $ */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include <nlist.h>	/* included for all machines */
/*
 * See syms.h for "mips" symbol table
 */

#if __u3b || __vax || __M32 || __u3b15 || __u3b5 || __u3b2 || __mips

 /*		COMMON OBJECT FILE FORMAT

 	File Organization:

 	_______________________________________________    INCLUDE FILE
 	|_______________HEADER_DATA___________________|
 	|					      |
 	|	File Header			      |    "filehdr.h"
 	|.............................................|
 	|					      |
 	|	Auxilliary Header Information	      |	   "aouthdr.h"
 	|					      |
 	|_____________________________________________|
 	|					      |
 	|	".text" section header		      |	   "scnhdr.h"
 	|					      |
 	|.............................................|
 	|					      |
 	|	".data" section header		      |	      ''
 	|					      |
 	|.............................................|
 	|					      |
 	|	".bss" section header		      |	      ''
 	|					      |
 	|_____________________________________________|
 	|______________RAW_DATA_______________________|
 	|					      |
 	|	".text" section data (rounded to 4    |
 	|				bytes)	      |
 	|.............................................|
 	|					      |
 	|	".data" section data (rounded to 4    |
 	|				bytes)	      |
 	|_____________________________________________|
 	|____________RELOCATION_DATA__________________|
 	|					      |
 	|	".text" section relocation data	      |    "reloc.h"
 	|					      |
 	|.............................................|
 	|					      |
 	|	".data" section relocation data	      |       ''
 	|					      |
 	|_____________________________________________|
 	|__________LINE_NUMBER_DATA_(SDB)_____________|
 	|					      |
 	|	".text" section line numbers	      |    "linenum.h"
 	|					      |
 	|.............................................|
 	|					      |
 	|	".data" section line numbers	      |	      ''
 	|					      |
 	|_____________________________________________|
 	|________________SYMBOL_TABLE_________________|
 	|					      |
 	|	".text", ".data" and ".bss" section   |    "syms.h"
 	|	symbols				      |	   "storclass.h"
 	|					      |
 	|_____________________________________________|
	|________________STRING_TABLE_________________|
	|					      |
	|	    long symbol names		      |
	|_____________________________________________|



 		OBJECT FILE COMPONENTS

 	HEADER FILES:
 			/usr/include/filehdr.h
			/usr/include/aouthdr.h
			/usr/include/scnhdr.h
			/usr/include/reloc.h
			/usr/include/linenum.h
			/usr/include/syms.h
			/usr/include/storclass.h

	STANDARD FILE:
			/usr/include/a.out.h    "object file" 
   */

#include "filehdr.h"
#include "aouthdr.h"
#include "scnhdr.h"
#include "reloc.h"
#ifndef __mips
#include "linenum.h"
#endif /* !__mips */
/* Note if mips is defined syms.h includes sym.h and symconst.h */
#include "syms.h"
#ifndef __mips
#include "storclass.h"
#endif /*  !__mips */

#ifdef __mips
/*
 * Coff files produced by the mips loader are guaranteed to have the raw data
 * for the sections follow the headers in this order: .text, .rdata, .data and
 * .sdata the sum of the sizes of last three is the value in dsize in the
 * optional header.  This is all done for the benefit of the programs that
 * have to load these objects so only the file header and optional header
 * have to be inspected.  The macro N_TXTOFF() takes pointers to file header
 * and optional header and returns the file offset to the start of the raw
 * data for the .text section.  The raw data for the three data sections
 * follows the start of the .text section by the value of tsize in the optional
 * header.
 *
 * Object files produced by pre 0.23 versions of the compiler had their sections
 * rounded to 8 byte boundaries.  0.23 and later versions have their sections
 * rounded to 16 (SCNROUND in scnhdr.h) byte boundaries.
 */
#define N_TXTOFF(f, a) \
 ((a).magic == ZMAGIC || (a).magic == LIBMAGIC ? 0 : \
  ((a).vstamp < 23 ? \
   ((FILHSZ + AOUTHSZ + (f).f_nscns * SCNHSZ + 7) & 0xfffffff8) : \
   ((FILHSZ + AOUTHSZ + (f).f_nscns * SCNHSZ + SCNROUND-1) & ~(SCNROUND-1)) ) )
#endif /* __mips */


#else /* u370 || pdp11 */


/*
 * Format of an a.out header
 */
 

struct	exec {	/* a.out header */
#if __u370
	int		a_magic;	/* magic number */
	int		a_stamp;	/* The version of a.out	*/
					/* format of this file.	*/
#else
	short		a_magic;	/* magic number */
#endif
	unsigned	a_text;		/* size of text segment */
					/* in bytes		*/
					/* padded out to next	*/
					/* page boundary with	*/
					/* binary zeros.	*/
	unsigned	a_data;		/* size of initialized data */
					/* segment in bytes	*/
					/* padded out to next	*/
					/* page boundary with	*/
					/* binary zeros.	*/
	unsigned	a_bss;		/* Actual size of	*/
					/* uninitialized data	*/
					/* segment in bytes.	*/
	unsigned	a_syms;		/* size of symbol table */
	unsigned	a_entry;	/* entry point */
#if __u370
	unsigned	a_trsize;	/* size of text relocation */
	unsigned	a_drsize;	/* size of data relocation */
	unsigned	a_origin;	/* The origin to which 	*/
					/* this file was	*/
					/* relocated.		*/
	unsigned	a_actext;	/* The actual size of	*/
					/* the text segment in	*/
					/* bytes.		*/
	unsigned	a_acdata;	/* The actual size of	*/
					/* the data segment in	*/
					/* bytes.		*/
#endif
#if __pdp11
	char		a_unused;	/* not used */
	unsigned char	a_hitext;	/* high order text bits */
	char		a_flag;		/* reloc info stripped */
	char		a_stamp;	/* environment stamp */
#endif
};

#define	A_MAGIC1	0407		/* normal */
#define	A_MAGIC0	0401		/* lpd (UNIX/RT) */
#define	A_MAGIC2	0410		/* read-only text */
#define	A_MAGIC3	0411		/* separated I&D */
#define	A_MAGIC4	0405		/* overlay */
#define	A_MAGIC5	0437		/* system overlay, separated I&D */

#if __u370
struct relocation_info {
	  long  r_address;	/* relative to current segment */
	  unsigned int
		r_symbolnum:24,	/* if extern then symbol table */
				/* ordinal (0, 1, 2, ...) else */
				/* segment number (same as symbol types) */
	        r_pcrel:1, 	/* if so, segment offset has already */
				/* been subtracted */
	  	r_length:2,	/* 0=byte, 1=word, 2=long */
	  	r_extern:1,	/* does not include value */
				/* of symbol referenced */
	  	r_offset:1,	/* already includes origin */
				/* of this segment (?) */
		r_pad:3;	/* nothing, yet */
};
#endif

/* in invocation of BADMAG macro, argument should not be a function. */

#define	BADMAG(X) (X.a_magic != A_MAGIC1 &&\
		X.a_magic != A_MAGIC2 &&\
		X.a_magic != A_MAGIC3 &&\
		X.a_magic != A_MAGIC4 &&\
		X.a_magic != A_MAGIC5 &&\
		X.a_magic != A_MAGIC0)

	/* values for type flag */

#define	N_UNDF	0	/* undefined */
#define	N_TYPE	037
#define	N_FN	037	/* file name symbol */

#if __pdp11
#define	N_ABS	01	/* absolute */
#define	N_TEXT	02	/* text symbol */
#define	N_DATA	03	/* data symbol */
#define	N_BSS	04	/* bss symbol */
#define	N_REG	024	/* register name */
#define	N_EXT	040	/* external bit, or'ed in */
#define	FORMAT	"%.6o"	/* to print a value */
#else
#define	N_ABS	02	/* absolute */
#define	N_TEXT	04	/* text */
#define	N_DATA	06	/* data */
#define	N_BSS	010
#define	N_GSYM	0040	/* global sym: name,,type,0 */
#define	N_FNAME 0042	/* procedure name (f77 kludge): name,,,0 */
#define	N_FUN	0044	/* procedure: name,,linenumber,address */
#define	N_STSYM 0046	/* static symbol: name,,type,address */
#define	N_LCSYM 0050	/* .lcomm symbol: name,,type,address */
#define	N_BSTR	0060	/* begin structure: name,,, */
#define	N_RSYM	0100	/* register sym: name,,register,offset */
#define	N_SLINE	0104	/* src line: ,,linenumber,address */
#define	N_ESTR	0120	/* end structure: name,,, */
#define	N_SSYM	0140	/* structure elt: name,,type,struct_offset */
#define	N_SO	0144	/* source file name: name,,,address */
#define	N_BENUM	0160	/* begin enum: name,,, */
#define	N_LSYM	0200	/* local sym: name,,type,offset */
#define	N_SOL	0204	/* #line source filename: name,,,address */
#define	N_ENUM	0220	/* enum element: name,,,value */
#define	N_PSYM	0240	/* parameter: name,,type,offset */
#define	N_ENTRY	0244	/* alternate entry: name,,linenumber,address */
#define	N_EENUM	0260	/* end enum: name,,, */
#define	N_LBRAC	0300	/* left bracket: ,,nesting level,address */
#define	N_RBRAC	0340	/* right bracket: ,,nesting level,address */
#define	N_BCOMM	0342	/* begin common: name,,, */
#define	N_ECOMM	0344	/* end common: name,,, */
#define	N_ECOML	0350	/* end common (local name): ,,,address */
#define	N_STRU	0374	/* 2nd entry for structure: str tag,,,length */
#define	N_LENG	0376	/* second stab entry with length information */
#define	N_EXT	01	/* external bit, or'ed in */
#define	FORMAT	"%.8x"
#define	STABTYPES 0340
#endif

#endif
#endif
