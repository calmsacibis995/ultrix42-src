/* @(#)i_defs.h	4.3 (ULTRIX) 11/14/90 */

/************************************************************************
 *									*
 *			Copyright (c) 1987,1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *		            Bull, France				*
 *			   Siemens AG, FR Germany			*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
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
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 * 003	David Lindner Thu Nov  8 09:12:55 EST 1990
 *	- Removed incorrect ansi ifdef changes.
 *
 * 002	David Lindner Fri Jan 19 12:45:21 EST 1990
 *	- Increased I_NAML to 50
 *
 * 001	David Lindner Tue Dec 19 16:51:32 EST 1989
 * 	- Added definitions of PRP_PRFX, COL_PRFX, and FRM_PRFX
 *	  for prefix names of tables.
 *
 */

/*
 * Required for defination of _lc_strtab[]
 */
#include <ansi_compat.h>
#ifndef _LC_MAX
#include "locale.h"
#endif

/*------------------------- general defines ----------------------------*/

#define	I_MAGIC		0x0205		/* magic number for intlinfo db */
#define	I_16		1		/* 16 bit codeset		*/
#define	I_NAML		50		/* length for names in intlinfo	*/
#define	CNV_COD		0x0001		/* is converted to a code	*/
#define	CNV_STR		0x0002		/* is converted to a string	*/
#define INTLINFO	"INTLINFO"	/* name of environment variable	*/
#define DEFLANG		"lang"		/* default value for LANG	*/
#define	INTLPATH	"/usr/lib/intln/%c/%L" /* default database pathname*/
#define PRP_DEF		"PROP_DFLT"	/* name of default property tab	*/
#define COL_DEF		"COLL_DFLT"	/* name of default collation tab*/
#define FRM_DEF		"STRG_DFLT"	/* name of default string table	*/
#define PRP_PRFX	"PROP_"
#define COL_PRFX	"COLL_"
#define FRM_PRFX	"STRG_"

/*
 * definitions of character properties (basic properties only)
 * ATTENTION: The order of the first 8 items is the same as in
 *	CTYPE(3C). This is to allow easy replacement of current
 *	_ctype_ table.
 */
#define	I_UPPER		0x0001		/* upper case letter		*/
#define	I_LOWER		0x0002		/* lower case letter		*/
#define	I_DIGIT		0x0004		/* digit character		*/
#define	I_SPACE		0x0008		/* "white space" character	*/
#define	I_PUNCT		0x0010		/* punctuation mark		*/
#define	I_CTRL		0x0020		/* control character		*/

#define	I_HEX		0x0040		/* hexadecimal character	*/
#define	I_BLANK		0x0080		/* blank character		*/

#define	I_MISCEL	0x0100		/* miscellaneous symbols	*/
#define	I_ARITH		0x0200		/* arithmetic sign		*/
#define	I_DIACR		0x0400		/* diacritical sign		*/
#define	I_DIPHT		0x0800		/* diphtong			*/
#define	I_FRACT		0x1000		/* fraction character		*/
#define	I_FIRST		0x2000		/* first of a double letter	*/
#define	I_SUPSUB	0x4000		/* printable character		*/
#define	I_CURENCY	0x8000		/* character with graphic rep.	*/

#define	I_ALPHA	(I_LOWER|I_UPPER)	/* alphabetic character		*/
#define	I_ALNUM	(I_UPPER|I_LOWER|I_DIGIT) /* alphanumeric character	*/
#define	I_XDIGIT (I_DIGIT|I_HEX)	/* hexadecimal digit		*/
/*-------------------------- typedefs ----------------------------------*/

#ifdef NOVOID
typedef char void;			/* for old compilers		*/
#endif
typedef unsigned short	i_char;		/* definition of an intl char	*/
typedef unsigned short	bit16;		/* definition of a 16 bit word	*/
typedef struct i_dbhead	i_dbhead;	/* data base header		*/
typedef struct pr_head	pr_head;	/* property table header	*/
typedef struct cl_head	cl_head;	/* collation table header	*/
typedef struct st_head	st_head;	/* format table header		*/
typedef struct cv_head	cv_head;	/* conversion table header	*/
typedef struct coll	coll;		/* collation information	*/
typedef struct i_dblt	i_dblt;		/* double letter structure	*/
typedef struct col_tab	col_tab;	/* incore collation table	*/
typedef struct cnv_tab	cnv_tab;	/* incore conversion table	*/
typedef struct prp_tab	prp_tab;	/* incore property table	*/
typedef struct st_head	str_tab;	/* incore string table		*/
typedef struct intl	intl;		/* incore database header	*/

/*--------------------------- structures -------------------------------*/

/* The i_dbhead structure defines the general header for the database.	*
 * There is exactly ONE such structure per database, and it always	*
 * begins the file.							*
 */
struct i_dbhead {		/* INTLINFO DATABASE HEADER		*/
	bit16	i_magic;	/* magic number				*/
	bit16	i_flags;	/* 8 - 16 bits codeset			*/
	bit16	i_nbspl;	/* number of simple letters		*/
	bit16	i_nblet;	/* total number of codes		*/

				/* DOUBLE LETTER TABLE SECTION		*/
	long	i_dbtab;	/* offset to double letter table	*/
	bit16	i_dblsz;	/* size of double letter table in bytes	*/
	bit16	i_nbdbl;	/* nb of entries in double letter table	*/

				/* PROPERTY TABLE SECTION HEADER	*/
	long	i_prtab;	/* offset to property table section	*/
	bit16	i_prhsz;	/* size of property table section 	*/
	bit16	i_prtsz;	/* size of one property table		*/
	bit16	i_nbrpr;	/* number of property tables		*/

				/* COLLATION TABLE SECTION HEADER	*/
	long	i_cltab;	/* offset to collation table section	*/
	bit16	i_clhsz;	/* size of collation table section 	*/
	bit16	i_cltsz;	/* size of one collation table		*/
	bit16	i_nbrcl;	/* number of collation tables		*/

				/* STRING TABLES SECTION HEADER		*/
	long	i_sttab;	/* offset to string table section	*/
	bit16	i_sthsz;	/* size of string table section		*/
	bit16	i_nbrst;	/* number of string tables		*/

				/* CONVERSION TABLE SECTION HEADER	*/
	long	i_cvtab;	/* offset to conversion table section	*/
	bit16	i_cvhsz;	/* size of conversion table section	*/
	bit16	i_nbrcv;	/* number of conversion tables		*/
};

/* Double letter table section. The double letter table is entirely	*
 * composed of these structures, i.e. there is NO header.		*
 * There is exactly ONE double letter table per database.		*
 * The starting location of the table is given by i_dbhead->i_dbtab	*
 */
struct i_dblt	{		/* DOUBLE LETTER TABLE			*/
	i_char	symb;		/* the double letter			*/
	bit16	indx;		/* index of the double letter		*/
};

/* Property Table Section						*
 * The beginning location of the section is given by i_dbhead->i_prtab.	*
 * The property table section contains one "header" per property table,	*
 * each header giving the name and the starting location of one		*
 * property table. All of the "headers" are grouped together at the	*
 * beginning of the section, followed by the property tables.		*
 */

struct pr_head	{		/* PROPERTY TABLE HEADER ENTRY		*/
	char	pr_name[I_NAML];/* name of property table		*/
	long	pr_offst;	/* offset to property table section	*/
};

/* Collation Table Section						*
 * The beginning location of the section is given by i_dbhead->i_cltab.	*
 * The collation table section contains one "header" per collation table*
 * each header giving the name and the starting location of one		*
 * collation table. All of the "headers" are grouped together at the	*
 * beginning of the section, followed by the collation tables.		*
 */
struct cl_head	{		/* COLLATION TABLE HEADER ENTRY		*/
	char	cl_name[I_NAML];/* name of collation			*/
	char	cl_pnam[I_NAML];/* name of the corr. prop. tab.		*/
	long	cl_offst;	/* offset to collation table section	*/
};

struct coll {			/* COLLATION TABLE ENTRY		*/
	i_char	cl_prim;	/* symbol primary weight	   	*/
	i_char	cl_sec;		/* symbol secondary weight	  	*/
};

/* String Table Section							*
 * The beginning location of the section is given by i_dbhead->i_sttab.	*
 * The string table section contains one "header" per string table,	*
 * each header giving the name, the starting location, and size of	*
 * one string table. All of the "headers" are grouped together at the	*
 * beginning of the section, followed by the string tables.		*
 * ---------------------------------------------------------------------*
 * THIS STRUCTURE IS ALSO USED FOR THE HEADER OF THE STRING TABLE ITSELF*
 * WHERE st_name IS THE NAME OF THE STRING, st_offset IS THE OFFSET OF	*
 * THE STRING IN THE TABLE AND st_siz IS THE SIZE OF THE STRING.	*
 */
struct st_head	{		/* STRINGS TABLE HEADER ENTRY		*/
	char	st_name[I_NAML];/* name of string table			*/
	long	st_offst;	/* offset to string table		*/
	bit16	st_siz;		/* size of this string table		*/
};

/* Conversion Table Section						*
 * The beginning location of the section is given by i_dbhead->i_cvtab.	*
 * The conversion table section contains one "header" per conversion	*
 * table, each header giving the name, the starting location, and other	*
 * info for one conversion table.					*
 * All of the "headers" are grouped together at the beginning of	*
 * the section, followed by the conversion tables.			*
 */
struct cv_head	{		/* CONVERSION TABLE HEADER ENTRY	*/
	char	cv_name[I_NAML];/* name of conversion			*/
	long	cv_offst;	/* offset to conversion table 		*/
	bit16	cv_size;	/* size of conversion table		*/
	bit16	cv_type;	/* type of conversion			*/
};

/* The following structures are defined for the incore representation	*
 * of the database							*
 */

struct col_tab {		/* INCORE REPRESENTATION OF A COLL TAB	*/
	prp_tab *col_prp;	/* ptr to the corr. prop. table 	*/
	coll   col_col[1];	/* the collation table			*/
};		

struct cnv_tab {		/* INCORE REPRESNTATION OF A CNV TAB	*/
	cv_head *cnv_hdr;	/* ptr to the corresponding header	*/
	prp_tab *cnv_prp;	/* ptr to corresponding property table	*/
	union {
	      i_char _cnv_cod[1];/*	when code conversion		*/
	      char   _cnv_str[1];/*	when string conversion		*/
	} _cnv;
};
#define cnv_cod _cnv._cnv_cod
#define cnv_str _cnv._cnv_str

struct prp_tab {		/* INCORE LIST OF LOADED PRP TAB'S	*/
	char	prp_nam[I_NAML];/* name of property table		*/
	bit16	prp_nbspl;	/* number of simple letters		*/
	bit16	prp_nbdbl;	/* number of double letters		*/
	i_dblt *prp_dblt;	/* pointer to double letter table	*/
	prp_tab *prp_nxt;	/* pointer to next property table	*/
	i_char	prp_tbl[1];	/* the property table			*/
};

struct intl {			/* INCORE DATABASE DESCRIPTION		*/
	char	*in_name;	/* ptr to the name of the database 	*/
	i_dblt	*in_dblt;	/* double letter table			*/
	prp_tab	*in_prdflt;	/* properties table			*/
	col_tab	*in_cldflt;	/* default collation table		*/
	st_head	*in_sgdflt;	/* default string table			*/
	cnv_tab	*in_ilower;	/* lower conversion table		*/
	cnv_tab	*in_iupper;	/* upper conversion table		*/
	i_dbhead in_dbhead;	/* header of the intlinfo file		*/
	int	 in_ifd;	/* database file descriptor		*/
	pr_head	*in_prhead;	/* property table header		*/
	prp_tab	*in_propt;	/* loaded property tables		*/
	cl_head	*in_clhead;	/* collation table header		*/
	st_head	*in_sthead;	/* string table header			*/
	cv_head	*in_cvhead;	/* conversion table header		*/
	intl	*in_nxt;	/* chain of loaded databases		*/
};

/*--------------------------- externals --------------------------------*/

extern int	i_errno;	/* internationalization error global	*/
extern intl	*i_defdb;	/* global accessible default db descrip	*/
extern char	*__loc1;	/* start of match for i_regexp		*/
extern char	*__reerr;	/* i_regexp error messages		*/

extern col_tab	*_lc_cldflt;	/* ANSI collation table			*/
extern prp_tab	*_lc_prdflt;	/* ANSI property table			*/
extern cnv_tab  *_lc_tolower;	/* ANSI lower case conversion table	*/
extern cnv_tab  *_lc_toupper;	/* ANSI upper case conversion table	*/
extern char	_lc_thosep;	/* ANSI numeric characters		*/
extern char	_lc_radix;
extern char	_lc_exl;
extern char	_lc_exu;
				/* ANSI string table pointers foreach	*/
				/* setlocale category			*/
extern str_tab	*_lc_strtab[_LC_MAX + 1];
extern intl	*i_init();	/* initialize database access		*/
extern prp_tab	*i_ldprp();	/* load a property table		*/
extern col_tab	*i_ldcol();	/* load a collation table		*/
extern cnv_tab	*i_ldcnv();	/* load a conversion table		*/
extern str_tab	*i_ldstr();	/* load a string table			*/
extern i_char	 i_conv();	/* code conversion for one char		*/
extern char	*i_trans();	/* string conversion for one char	*/
extern char	*i_getstr();	/* get string from stringtable		*/
extern void	i_perror();	/* print error message			*/
extern void	i_iperror();	/* print internationalization error msg	*/
extern char	*i_asctime();	/* user time formatter			*/
extern i_char	*i_regcmp();	/* regular expression compiler		*/
extern char	*i_regex();	/* regular expression executor		*/
extern char	*langinfo();	/* get a string from the database	*/
extern intl	*langtoid();	/* convert a name to a database pointer	*/
extern char	*idtolang();	/* convert a database pointer to a name	*/
extern intl	*currlangid();	/* get current database pointer		*/
extern char	*nl_asctime();	/* internationalized asctime		*/
extern char	*nl_ctime();	/* internationalized ctime		*/
extern char	*nl_gcvt();	/* internationalized gcvt		*/

/*
 * function definitions local to library functions but visible from outside
 */
extern int cv_indx();		/* double letter to index in dblt table	*/
extern int sknrd();		/* secure seek and read			*/
