#include <ansi_compat.h>
#ifdef __mips
/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: scnhdr.h,v 2010.5.1.5 89/11/29 22:40:58 bettina Exp $ */
#ifndef __SCNHDR_H
#define __SCNHDR_H

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#if __mips
/*
 * The entries that refer to line numbers are not used for line numbers on
 * "mips" machines.  See symhdr.h for the entries to get to the line number
 * table.  The entries that were for line numbers are used for gp tables on
 * "mips" machines.  That is s_lnnoptr is the file ptr to the gp table and
 * s_nlnno is the number of table entries.  See the end of this file for the
 * structure.
 */
#endif

#ifdef __LANGUAGE_C
struct scnhdr {
	char		s_name[8];	/* section name */
	long		s_paddr;	/* physical address, aliased s_nlib */
	long		s_vaddr;	/* virtual address */
	long		s_size;		/* section size */
	long		s_scnptr;	/* file ptr to raw data for section */
	long		s_relptr;	/* file ptr to relocation */
	long		s_lnnoptr;	/* file ptr to gp histogram */
	unsigned short	s_nreloc;	/* number of relocation entries */
	unsigned short	s_nlnno;	/* number of gp histogram entries */
	long		s_flags;	/* flags */
	};

#endif /* __LANGUAGE_C */
#ifdef LANGUAGE_PASCAL
type
  scnhdr = packed record
      s_name : packed array[1..8] of char; /* section name		     */
      s_paddr : long;			/* physical address		     */
      s_vaddr : long;			/* virtual address		     */
      s_size : long;			/* section size 		     */
      s_scnptr : long;			/* file ptr to raw data for section  */
      s_relptr : long;			/* file ptr to relocation	     */
      s_lnnoptr : long; 		/* file ptr to gp histogram	     */
      s_nreloc : ushort;		/* number of relocation entries      */
      s_nlnno : ushort; 		/* number of gp histogram entries    */
      s_flags : long;			/* flags			     */
    end {record};
#endif /* LANGUAGE_PASCAL */

#ifdef __mips
/* SCNROUND is the size that sections are rounded off to */
#ifdef __LANGUAGE_C
#define SCNROUND ((long)16)
#endif /* __LANGUAGE_C */
#ifdef LANGUAGE_PASCAL
#define SCNROUND (16)
#endif /* LANGUAGE_PASCAL */
#endif /* __mips */

/* the number of shared libraries in a .lib section in an absolute output file
 * is put in the s_paddr field of the .lib section header, the following define
 * allows it to be referenced as s_nlib
 */

#define s_nlib	s_paddr
#define	SCNHDR	struct scnhdr
#define	SCNHSZ	sizeof(SCNHDR)




/*
 * Define constants for names of "special" sections
 */

#ifdef __LANGUAGE_C
#define	_TEXT	".text"
#define	_DATA	".data"
#define	_BSS	".bss"
#define	_TV	".tv"
#define _INIT ".init"
#define _FINI ".fini"
#define _LIB ".lib"
#endif /* __LANGUAGE_C */
#ifdef LANGUAGE_PASCAL
#ifdef PASTEL
#define	_TEXT	('.text'||chr(0))
#define	_DATA	('.data'||chr(0))
#define	_BSS	('.bss'||chr(0))
#define	_TV	('.tv'||chr(0))
#define	_INIT	('.init'||chr(0))
#define	_FINI	('.fini'||chr(0))
#define	_LIB	('.lib'||chr(0))
#else 
#define	_TEXT	".text\0"
#define	_DATA	".data\0"
#define	_BSS	".bss\0"
#define	_TV	".tv\0"
#define	_INIT	".init\0"
#define	_FINI	".fini\0"
#define	_LIB	".lib\0"
#endif /* PASTEL */
#endif /* LANGUAGE_PASCAL */

#if __mips
/*
 * Mips names for read only data (.rdata), small data (.sdata) and small bss
 * (.bss).  Small sections are used for global pointer relative data items.
 */
#ifdef __LANGUAGE_C
#define	_RDATA	".rdata"
#define	_SDATA	".sdata"
#define	_SBSS	".sbss"
#define _UCODE	".ucode"
#define _LIT8	".lit8"
#define _LIT4	".lit4"
#endif /* __LANGUAGE_C */
#ifdef LANGUAGE_PASCAL
#ifdef PASTEL
#define	_RDATA	('.rdata'||chr(0))
#define	_SDATA	('.sdata'||chr(0))
#define	_SBSS	('.sbss'||chr(0))
#define	_UCODE	('.ucode'||chr(0))
#define	_LIT8	('.lit8'||chr(0))
#define	_LIT4	('.lit4'||chr(0))
#else 
#define	_RDATA	".rdata\0"
#define	_SDATA	".sdata\0"
#define	_SBSS	".sbss\0"
#define	_UCODE	".ucode\0"
#define	_LIT8	".lit8\0"
#define	_LIT4	".lit4\0"
#endif /* PASTEL */
#endif /* LANGUAGE_PASCAL */
#endif


/*
 * The low 4 bits of s_flags is used as a section "type"
 */

#ifdef __LANGUAGE_C
#define STYP_REG	0x00		/* "regular" section:
						allocated, relocated, loaded */
#define STYP_DSECT	0x01		/* "dummy" section:
						not allocated, relocated,
						not loaded */
#define STYP_NOLOAD	0x02		/* "noload" section:
						allocated, relocated,
						 not loaded */
#define STYP_GROUP	0x04		/* "grouped" section:
						formed of input sections */
#define STYP_PAD	0x08		/* "padding" section:
						not allocated, not relocated,
						 loaded */
#define STYP_COPY	0x10		/* "copy" section:
						for decision function used
						by field update;  not
						allocated, not relocated,
						loaded;  reloc & lineno
						entries processed normally */
#define	STYP_TEXT	0x20		/* section contains text only */
#define STYP_DATA	0x40		/* section contains data only */
#define STYP_BSS	0x80		/* section contains bss only */
#if __mips
#define STYP_RDATA	0x100		/* section contains read only data */
#define STYP_SDATA	0x200		/* section contains small data only */
#define STYP_SBSS	0x400		/* section contains small bss only */
#define STYP_UCODE	0x800		/* section only contains ucodes */
#define STYP_LIT8	0x08000000	/* literal pool for 8 byte literals */
#define STYP_LIT4	0x10000000	/* literal pool for 4 byte literals */
#define S_NRELOC_OVFL	0x20000000	/* s_nreloc overflowed, the value is in
					   v_addr of the first entry */
#define STYP_LIB	0x40000000	/* section is a .lib section */
#define STYP_INIT	0x80000000	/* section only contains the text
					   instructions for the .init sec. */
#else
#define STYP_INFO	0x200		/* comment section : not allocated
						not relocated, not loaded */
#define STYP_LIB	0x800		/* for .lib section : same as INFO */
#define STYP_OVER	0x400		/* overlay section : relocated
						not allocated or loaded */
#endif /* __mips */
#endif /* __LANGUAGE_C */
#ifdef LANGUAGE_PASCAL
#define STYP_REG	16#00		/* "regular" section:
						allocated, relocated, loaded */
#define STYP_DSECT	16#01		/* "dummy" section:
						not allocated, relocated,
						not loaded */
#define STYP_NOLOAD	16#02		/* "noload" section:
						allocated, relocated,
						 not loaded */
#define STYP_GROUP	16#04		/* "grouped" section:
						formed of input sections */
#define STYP_PAD	16#08		/* "padding" section:
						not allocated, not relocated,
						 loaded */
#define STYP_COPY	16#10		/* "copy" section:
						for decision function used
						by field update;  not
						allocated, not relocated,
						loaded;  reloc & lineno
						entries processed normally */
#define	STYP_TEXT	16#20		/* section contains text only */
#define STYP_DATA	16#40		/* section contains data only */
#define STYP_BSS	16#80		/* section contains bss only */
#if __mips
#define STYP_RDATA	16#100		/* section contains read only data */
#define STYP_SDATA	16#200		/* section contains small data only */
#define STYP_SBSS	16#400		/* section contains small bss only */
#define STYP_UCODE	16#800		/* section only contains ucodes */
#define STYP_LIT8	16#08000000	/* literal pool for 8 byte literals */
#define STYP_LIT4	16#10000000	/* literal pool for 4 byte literals */
#define S_NRELOC_OVFL	16#20000000	/* s_nreloc overflowed, the value is in
					   v_addr of the first entry */
#define STYP_LIB	16#40000000	/* section is a .lib section */
#define STYP_INIT	16#80000000	/* section only contains the text
					   instructions for the .init sec. */
#else
#define STYP_INFO	16#200		/* comment section : not allocated
						not relocated, not loaded */
#define STYP_LIB	16#800		/* for .lib section : same as INFO */
#define STYP_OVER	16#400		/* overlay section : relocated
						not allocated or loaded */
#endif /* __mips */
#endif /* LANGUAGE_PASCAL */

/*
 *  In a minimal file or an update file, a new function
 *  (as compared with a replaced function) is indicated by S_NEWFCN
 */

#define S_NEWFCN  0x100

/*
 * In 3b Update Files (output of ogen), sections which appear in SHARED
 * segments of the Pfile will have the S_SHRSEG flag set by ogen, to inform
 * dufr that updating 1 copy of the proc. will update all process invocations.
 */

#define S_SHRSEG	0x20

#if __mips
/*
 * This table gives the section size corresponding to each applicable
 * Gnum (always including 0), sorted by smallest size first. It is pointed to
 * by the s_lnnoptr field in the section header and its number of entries
 * (including the header) is in the s_nlnno field in the section header.
 * This table only needs to exist for the .sdata and .sbss sections
 * sections.  If there is no "small" section then the gp table for it is
 * attached to the coresponding "large" section so the information still
 * gets to the loader.
 */
#ifdef __LANGUAGE_C
union gp_table {
  struct {
    long current_g_value; /* actual value */
    long unused;
  } header;
  struct {
    long g_value; /* hypothetical value */
    long bytes;	/* section size corresponding to hypothetical value */
  } entry;
}; 
#define GPTAB	union gp_table
#define GPTABSZ	sizeof(GPTAB)

#endif /* __LANGUAGE_C */

#ifdef LANGUAGE_PASCAL
type
  gp_table = record
    case boolean of
      false: (current_g_value: integer; unused: integer);
      true: (g_value: integer; bytes: integer);
    end;
  gpt_ptr = ^gp_table;
#endif /* LANGUAGE_PASCAL */

#endif /* __mips */

#ifdef __mips
/*
 * This is the definition of a mips .lib section entry.  Note the size and
 * offset are in sizeof(long)'s not bytes.
 */
#ifdef __LANGUAGE_C
struct libscn {
	long	size;		/* size of this entry (including target name) */
	long	offset;		/* offset from start of entry to target name  */
	long	tsize;		/* text size in bytes, padded to DW boundary  */
	long	dsize;		/* initialized data "  	  "    "  "   "       */
	long	bsize;		/* uninitialized data "   "    "  "   "       */
	long	text_start;	/* base of text used for this library	      */
	long	data_start;	/* base of data used for this library	      */
	long	bss_start;	/* base of bss used for this library	      */
	/* pathname of target shared library */
};

#endif /* __LANGUAGE_C */

#define	LIBSCN	struct libscn
#define	LSCNSZ	sizeof(LIBSCN)

#endif /* __mips */
/* $Log:	scnhdr.h,v $
 * Revision 2010.5.1.5  89/11/29  22:40:58  bettina
 * 2.10 BETA2
 * 
 * Revision 2010.1  89/09/26  20:47:43  lai
 * updated to 2.10
 * 
 * Revision 2010.1  89/09/26  20:07:27  lai
 * 2.10, also added wrapper and $LOG
 * 
*/
#endif
#endif
