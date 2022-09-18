#include <ansi_compat.h>
#ifdef __mips
/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: syms.h,v 2010.5.1.5 89/11/29 22:41:17 bettina Exp $ */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Defines for "special" symbols   */

#if __vax
#define _ETEXT	"_etext"
#define	_EDATA	"_edata"
#define	_END	"_end"
#endif /* __vax */

#if __mips
#ifdef __LANGUAGE_C
#define _ETEXT	"etext"
#define	_EDATA	"edata"
#define	_END	"end"
#endif /* __LANGUAGE_C */
#ifdef LANGUAGE_PASCAL
#ifdef PASTEL
#define _ETEXT	('etext'||chr(0))
#define	_EDATA	('edata'||chr(0))
#define	_END	('end'||chr(0))
#else
#define _ETEXT	'etext\0'
#define	_EDATA	'edata\0'
#define	_END	'end\0'
#endif /* PASTEL */
#endif /* LANGUAGE_PASCAL */
/*
 * The displacement of the gp is from the start of the small data section
 * is GP_DISP.  The GP_PAD is the padding of the gp area so small negitive
 * offset from gp relocation values can allways be used.
 */
#define GP_PAD	16
#define GP_DISP	(32768 - GP_PAD)
#define GP_SIZE (GP_DISP+32767)
/* "special" symbols for starts of sections */
#ifdef __LANGUAGE_C
#define	_FTEXT	"_ftext"
#define	_FDATA	"_fdata"
#define	_FBSS	"_fbss"
#define	_GP	"_gp"
#define	_PROCEDURE_TABLE	"_procedure_table"
#define	_PROCEDURE_TABLE_SIZE	"_procedure_table_size"
#define	_PROCEDURE_STRING_TABLE	"_procedure_string_table"
#define	_COBOL_MAIN	"_cobol_main"
#endif /* __LANGUAGE_C */
#ifdef LANGUAGE_PASCAL
#ifdef PASTEL
#define	_FTEXT	('_ftext'||chr(0))
#define	_FDATA	('_fdata'||chr(0))
#define	_FBSS	('_fbss'||chr(0))
#define	_GP	('_gp'||chr(0))
#define	_PROCEDURE_TABLE	('_procedure_table'||chr(0))
#define	_PROCEDURE_TABLE_SIZE	('_procedure_table_size'||chr(0))
#define	_PROCEDURE_STRING_TABLE	('_procedure_string_table'||chr(0))
#else
#define	_FTEXT	'_ftext\0'
#define	_FDATA	'_fdata\0'
#define	_FBSS	'_fbss\0'
#define	_GP	'_gp\0'
#define	_PROCEDURE_TABLE	'_procedure_table\0'
#define	_PROCEDURE_TABLE_SIZE	'_procedure_table_size\0'
#define	_PROCEDURE_STRING_TABLE	'_procedure_string_table\0'
#endif /* PASTEL */
#endif /* LANGUAGE_PASCAL */

#else /* !defined(mips) */

#define _ETEXT	"etext"
#define _EDATA	"edata"
#define _END	"end"
#endif /* __mips */

#ifdef __LANGUAGE_C
#define _START	"__start"
#endif /* __LANGUAGE_C */
#ifdef LANGUAGE_PASCAL
#ifdef PASTEL
#define _START	('__start'||chr(0))
#else
#define _START	'__start\0'
#endif /* PASTEL */
#endif /* LANGUAGE_PASCAL */

#if __mips

#include "symconst.h"
#ifdef __LANGUAGE_C
#include "sym.h"
#include "cmplrs/stsupport.h"
#endif /* __LANGUAGE_C */

#else /* !defined(mips) */

/*		Storage Classes are defined in storclass.h  */
#include "storclass.h"

/*		Number of characters in a symbol name */
#define  SYMNMLEN	8
/*		Number of characters in a file name */
#define  FILNMLEN	14
/*		Number of array dimensions in auxiliary entry */
#define  DIMNUM		4


struct syment
{
	union
	{
		char		_n_name[SYMNMLEN];	/* old COFF version */
		struct
		{
			long	_n_zeroes;	/* new == 0 */
			long	_n_offset;	/* offset into string table */
		} _n_n;
		char		*_n_nptr[2];	/* allows for overlaying */
	} _n;
#ifndef __pdp11
	unsigned
#endif
	long			n_value;	/* value of symbol */
	short			n_scnum;	/* section number */
	unsigned short		n_type;		/* type and derived type */
	char			n_sclass;	/* storage class */
	char			n_numaux;	/* number of aux. entries */
};

#define n_name		_n._n_name
#define n_nptr		_n._n_nptr[1]
#define n_zeroes	_n._n_n._n_zeroes
#define n_offset	_n._n_n._n_offset

/*
   Relocatable symbols have a section number of the
   section in which they are defined.  Otherwise, section
   numbers have the following meanings:
*/
        /* undefined symbol */
#define  N_UNDEF	0
        /* value of symbol is absolute */
#define  N_ABS		-1
        /* special debugging symbol -- value of symbol is meaningless */
#define  N_DEBUG	-2
	/* indicates symbol needs transfer vector (preload) */
#define  N_TV		(unsigned short)-3

	/* indicates symbol needs transfer vector (postload) */

#define  P_TV		(unsigned short)-4

/*
   The fundamental type of a symbol packed into the low 
   4 bits of the word.
*/

#define  _EF	".ef"

#define  T_NULL     0
#define  T_ARG      1          /* function argument (only used by compiler) */
#define  T_CHAR     2          /* character */
#define  T_SHORT    3          /* short integer */
#define  T_INT      4          /* integer */
#define  T_LONG     5          /* long integer */
#define  T_FLOAT    6          /* floating point */
#define  T_DOUBLE   7          /* double word */
#define  T_STRUCT   8          /* structure  */
#define  T_UNION    9          /* union  */
#define  T_ENUM     10         /* enumeration  */
#define  T_MOE      11         /* member of enumeration */
#define  T_UCHAR    12         /* unsigned character */
#define  T_USHORT   13         /* unsigned short */
#define  T_UINT     14         /* unsigned integer */
#define  T_ULONG    15         /* unsigned long */

/*
 * derived types are:
 */

#define  DT_NON      0          /* no derived type */
#define  DT_PTR      1          /* pointer */
#define  DT_FCN      2          /* function */
#define  DT_ARY      3          /* array */

/*
 *   type packing constants
 */

#define  N_BTMASK     017
#define  N_TMASK      060
#define  N_TMASK1     0300
#define  N_TMASK2     0360
#define  N_BTSHFT     4
#define  N_TSHIFT     2

/*
 *   MACROS
 */

	/*   Basic Type of  x   */

#define  BTYPE(x)  ((x) & N_BTMASK)

	/*   Is  x  a  pointer ?   */

#define  ISPTR(x)  (((x) & N_TMASK) == (DT_PTR << N_BTSHFT))

	/*   Is  x  a  function ?  */

#define  ISFCN(x)  (((x) & N_TMASK) == (DT_FCN << N_BTSHFT))

	/*   Is  x  an  array ?   */

#define  ISARY(x)  (((x) & N_TMASK) == (DT_ARY << N_BTSHFT))

	/* Is x a structure, union, or enumeration TAG? */

#define ISTAG(x)  ((x)==C_STRTAG || (x)==C_UNTAG || (x)==C_ENTAG)

#define  INCREF(x) ((((x)&~N_BTMASK)<<N_TSHIFT)|(DT_PTR<<N_BTSHFT)|(x&N_BTMASK))

#define  DECREF(x) ((((x)>>N_TSHIFT)&~N_BTMASK)|((x)&N_BTMASK))

/*
 *	AUXILIARY ENTRY FORMAT
 */

union auxent
{
	struct
	{
		long		x_tagndx;	/* str, un, or enum tag indx */
		union
		{
			struct
			{
				unsigned short	x_lnno;	/* declaration line number */
				unsigned short	x_size;	/* str, union, array size */
			} x_lnsz;
			long	x_fsize;	/* size of function */
		} x_misc;
		union
		{
			struct			/* if ISFCN, tag, or .bb */
			{
				long	x_lnnoptr;	/* ptr to fcn line # */
				long	x_endndx;	/* entry ndx past block end */
			} 	x_fcn;
			struct			/* if ISARY, up to 4 dimen. */
			{
				unsigned short	x_dimen[DIMNUM];
			} 	x_ary;
		}		x_fcnary;
		unsigned short  x_tvndx;		/* tv index */
	} 	x_sym;
	struct
	{
		char	x_fname[FILNMLEN];
	} 	x_file;
        struct
        {
                long    x_scnlen;          /* section length */
                unsigned short  x_nreloc;  /* number of relocation entries */
                unsigned short  x_nlinno;  /* number of line numbers */
        }       x_scn;

	struct
	{
		long		x_tvfill;	/* tv fill value */
		unsigned short	x_tvlen;	/* length of .tv */
		unsigned short	x_tvran[2];	/* tv range */
	}	x_tv;	/* info about .tv section (in auxent of symbol .tv)) */
};

#define	SYMENT	struct syment
#define	SYMESZ	18	/* sizeof(SYMENT) */

#define	AUXENT	union auxent
#define	AUXESZ	18	/* sizeof(AUXENT) */

#endif /* __mips */
#endif
