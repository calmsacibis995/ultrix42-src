/* 	@(#)exec.h	4.2	(ULTRIX)	9/4/90 	*/

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
/*
 *
 *   Modification History:
 *
 * 03 Nov 87 -- map
 *	Change magic number definition for compatibility modes.
 * 02 Apr 86 -- depp
 *	New file
 *
 */

/*
 *	@(#)exec.h	1.2 (Berkeley) 6/8/85
 */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#ifdef __vax
/*
 * Header prepended to each a.out file.
 */
struct exec {
unsigned short	a_magic;	/* magic number */
unsigned short	a_mode;		/* mode parameter */
unsigned long	a_text;		/* size of text segment */
unsigned long	a_data;		/* size of initialized data */
unsigned long	a_bss;		/* size of uninitialized data */
unsigned long	a_syms;		/* size of symbol table */
unsigned long	a_entry;	/* entry point */
unsigned long	a_trsize;	/* size of text relocation */
unsigned long	a_drsize;	/* size of data relocation */
};
#endif /* __vax */

#define	OMAGIC	0407		/* old impure format */
#define	NMAGIC	0410		/* read-only text */
#define	ZMAGIC	0413		/* demand load format */

/*
 *	Compatibility modes
 */
#define	A_BSD	0		/* All pre V2.4 a.outs and BSD */
#define	A_SYSV	1		/* SVID compliant process */
#define	A_POSIX 2		/* IEEE P1003.1 compliant process */

#ifdef __mips

struct filehdr {
	unsigned short	f_magic;	/* magic number */
	unsigned short	f_nscns;	/* number of sections */
	long		f_timdat;	/* time & date stamp */
	long		f_symptr;	/* file pointer to symbolic header */
	long		f_nsyms;	/* sizeof(symbolic hdr) */
	unsigned short	f_opthdr;	/* sizeof(optional hdr) */
	unsigned short	f_flags;	/* flags */
};

struct aouthdr {
	short	magic;		/* see magic.h				*/
	short	vstamp;		/* version stamp			*/
	long	tsize;		/* text size in bytes, padded to FW
				   bdry					*/
	long	dsize;		/* initialized data "  "		*/
	long	bsize;		/* uninitialized data "   "		*/
	long	entry;		/* entry pt.				*/
	long	text_start;	/* base of text used for this file	*/
	long	data_start;	/* base of data used for this file	*/
	long	bss_start;	/* base of bss used for this file	*/
	long	gprmask;	/* general purpose register mask	*/
	long	cprmask[4];	/* co-processor register masks		*/
	long	gp_value;	/* the gp value used for this object    */
};

struct scnhdr {
	char		s_name[8];	/* section name */
	long		s_paddr;	/* physical address */
	long		s_vaddr;	/* virtual address */
	long		s_size;		/* section size */
	long		s_scnptr;	/* file ptr to raw data for section */
	long		s_relptr;	/* file ptr to relocation */
	long		s_lnnoptr;	/* file ptr to line numbers (not used)*/
	unsigned short	s_nreloc;	/* number of relocation entries */
	unsigned short	s_nlnno;	/* number of line numbers (not used) */
	long		s_flags;	/* flags */
};

struct exec {
	struct filehdr	ex_f;
	struct aouthdr	ex_o;
};

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
 */
#define	FILHDR	struct filehdr
#define	FILHSZ	sizeof(FILHDR)
#define	AOUTHSZ	sizeof(struct aouthdr)
#define	SCNHSZ	sizeof(struct scnhdr)
/* SCNROUND is the size that sections are rounded off to */
#define SCNROUND ((long)16)

#define N_TXTOFF(f, a) \
 ((a).magic == ZMAGIC ? 0 : \
  ((a).vstamp < 23 ? \
   ((FILHSZ + AOUTHSZ + (f).f_nscns * SCNHSZ + 7) & 0xfffffff8) : \
   ((FILHSZ + AOUTHSZ + (f).f_nscns * SCNHSZ + SCNROUND-1) & ~(SCNROUND-1)) ) )

/*
 * for vax compatibility
 */
#define a_data	ex_o.dsize
#define a_text	ex_o.tsize
#define a_bss	ex_o.bsize
#define a_entry	ex_o.entry
#define a_magic	ex_o.magic

#ifdef __LANGUAGE_C
#define  MIPSEBMAGIC	0x0160
#define  MIPSELMAGIC	0x0162
#endif /* __LANGUAGE_C */
#ifdef LANGUAGE_PASCAL
#define  MIPSEBMAGIC	16#0160
#define  MIPSELMAGIC	16#0162
#endif /* LANGUAGE_PASCAL */

#ifdef __MIPSEL
#define OBJMAGIC	MIPSELMAGIC
#endif /* __MIPSEL */
#ifdef __MIPSEB
#define OBJMAGIC	MIPSEBMAGIC
#endif /* __MIPSEB */

#endif /* __mips */
