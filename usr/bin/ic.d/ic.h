/* @(#)ic.h	4.1 (ULTRIX) 7/17/90 */

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
 *
 * General include for all ic parts. Definitions of structures and globals.
 *
 */

/* 
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 *
 * 001	David Lindner Tue Dec 19 10:16:18 EST 1989
 *	- Added definition of BUFSIZE, include of strings.h, and 
 *	  modified comment header.
 *
 */

#include <stdio.h>
#include <strings.h>		/* DJL 001 */
#include <i_defs.h>

#ifdef EBUG
#include "dbg.h"
#endif

/*
 * DJL 001
 * definition of general purpose buffer size
 */
#define BUFSIZE		1024

/*
 * definition for maximal string length
 */
#define STRMAX		256

/*
 * define max weight (primary and secondary) a property can have if the
 * strxfrm and the following collation shall collate correct.
 */
#define WEIGHTMAX       255

/*
 * additional definition of character property
 */
#define I_ILLEGAL	0x0000		/* illegal character		*/

/*
 * definitions for sym_typ
 */
#define SYM_UDF		0x0000		/* not defined			*/
#define SYM_CDF		0x0001		/* defined as a code		*/
#define SYM_FDF		0x0002		/* defined as a format		*/
#define SYM_COD		0x0010		/* name of code set table	*/
#define SYM_COL		0x0020		/* name of collation table	*/
#define SYM_FRM		0x0040		/* name of format table		*/
#define SYM_CNV		0x0080		/* name of conversion table	*/
#define SYM_PRP		0x0100		/* name of a property list	*/

/*
 * definitions for val_typ
 */
#define VAL_COD		0x0001		/* character value		*/
#define VAL_STR		0x0002		/* string value			*/
#define VAL_SAM		0x0004		/* pseudo value SAME (cnv only)	*/
#define VAL_VOI		0x0008		/* pseudo value VOID (cnv only)	*/

/*
 * macros
 */
#define new(type, cnt)	((type *)calloc((unsigned)cnt, sizeof(type)))
#define max(a,b)	((a > b) ? a : b)

/*------------------ structures and typedefs ---------------------------*/
typedef struct cod	cod;		/* character and properties	*/
typedef struct sym	sym;		/* symbol table entry		*/
typedef struct val	val;		/* value table entry		*/

struct cod {			/* DESCRIPTION OF ONE CHARACTER CODE	*/
	i_char cod_rep;		/* character bit pattern		*/
	i_char cod_prp;		/* character properties			*/
};

#define CHRSIZ	sizeof(i_char)
#define CODSIZ	(sizeof(cod)/sizeof(i_char))

struct sym {			/* SYMBOL TABLE ENTRY 			*/
	bit16 sym_typ;		/* type of symbol			*/
	char *sym_nam;		/* name of symbol			*/
	union {			/* value of this symbol			*/
		val *_symval;	/*	when SYM_CDF/SYM_FDF		*/
		cl_head *_symcol;/*	when SYM_COL			*/
		st_head *_symfrm;/*	when SYM_FRM			*/
		cv_head *_symcnv;/*	when SYM_CNV			*/
		pr_head *_symprp;/*	when SYM_PRP			*/
	} _sym_val;
	sym *sym_hshnxt;	/* next in hash list			*/
	sym *sym_nxt;		/* chain of same symbols		*/
};

/*
 * access macros for sym value field:
 */
#define sym_val	_sym_val._symval
#define sym_cnv	_sym_val._symcnv
#define sym_frm _sym_val._symfrm
#define sym_col _sym_val._symcol
#define sym_prp _sym_val._symprp

struct val {			/* ENTRY FOR A VALUE 			*/
	bit16 val_typ;		/* type of value			*/
	bit16 val_len;		/* length of value in bytes		*/
	union {			/* value field				*/
		cod *_valcod;	/* 	when VAL_COD			*/
		char *_valstr;	/* 	when VAL_STR			*/
	} _val_val;
	val *val_nxt;		/* next part of value			*/
};

/*
 * access macros for val value field:
 */
#define val_cod	_val_val._valcod
#define val_str	_val_val._valstr

/*------------------------ external declarations ----------------------*/

/*
 * standard functions that have to be declared
 */
extern FILE *popen();
extern char *calloc();
extern char *strcpy();
extern char *strcat();
extern char *index();

/*
 * symbol table handling
 */
extern int sym_chk();		/* check type of symbol -- sym.c	*/
extern int sym_lookup();	/* symbol table lookup -- sym.c		*/
extern void sym_set();		/* set type of a symbol -- sym.c	*/
extern void sym_del();		/* delete a symbol chain -- sym.c	*/
extern sym *sym_fake();		/* make up a symbol -- sym.c		*/
extern sym *sym_ins();		/* insert symbol in sorted list -- sym.c*/
extern sym *sym_find();		/* find a symbol -- sym.c		*/

/*
 * code table and property handling
 */
extern sym *cod_add();		/* add a code to list of codes -- cod.c	*/
extern sym *cod_make();		/* make a code -- cod.c			*/
extern void cod_set();		/* remember the code set -- cod.c	*/
extern i_char prp_add();	/* add a property -- prp.c		*/
extern i_char prp_make();	/* make a property -- prp.c		*/
extern i_char prp_set();	/* set properties of a code -- prp.c	*/
extern void prp_init();		/* initialize a property list -- prp.c	*/
extern void prp_end();		/* finish a property list -- prp.c	*/
extern cod *cod_first();	/* find first of double letter -- cod.c	*/

/*
 * collation table handling
 */
extern void col_end();		/* finish up a collation -- col.c	*/
extern void col_init();		/* start a collation table -- col.c	*/
extern void col_range();	/* collation for a value range -- col.c	*/
extern void col_rest();		/* collation for rest values -- col.c	*/
extern void col_set();		/* set collation for a value -- col.c	*/
extern void col_dipht();	/* diphtong collation -- col.c		*/
extern void col_prp();		/* assign a property table -- col.c	*/
extern void weight_tst();	/* weight within limit ? -- col.c       */

/*
 * format table handling
 */
extern sym *frm_add();		/* add a format -- frm.c		*/
extern void frm_end();		/* finish up a format table -- frm.c	*/
extern void frm_init();		/* start a format table -- frm.c	*/
extern sym *frm_set();		/* set a format value -- frm.c		*/

/*
 * value handling general
 */
extern val *con_make();		/* make a constant value -- val.c	*/
extern val *def_make();		/* make a default value -- val.c	*/
extern val *str_make();		/* string to value -- val.c		*/
extern val *strtolist();	/* convert string to code list -- val.c	*/
extern val *val_add();		/* build a composite value -- val.c	*/
extern bit16 val_chk();		/* check value types -- val.c		*/
extern void val_del();		/* remove a value -- val.c		*/
extern val *valtocod();		/* value to code value -- val.c		*/
extern val *valtolist();	/* value to code list -- val.c		*/
extern val *var_make();		/* symbol to value -- val.c		*/
extern int val_len();		/* get length of a value -- val.c	*/
extern val *chrtoval();		/* i_char to value -- val.c		*/
extern cod *idxtocod();		/* code index to character -- val.c	*/

/*
 * conversion handling
 */
extern void cnv_end();		/* finish up a conversion -- cnv.c	*/
extern sym *cnv_init();		/* start a conversion table -- cnv.c	*/
extern void cnv_range();	/* convert a range of values -- cnv.c	*/
extern void cnv_set();		/* remember a conversion -- cnv.c	*/
extern void def_set();		/* set the default conv value -- cnv.c	*/
extern int chrtoidx();		/* character to code index -- cnv.c	*/

/*
 * message handling
 */
extern void error();		/* print an error message -- message.c	*/
extern void warning();		/* print a warning message -- message.c	*/
extern void fatal();		/* print msg and exit -- message.c	*/
extern void message();		/* print a message -- message.c		*/
extern void bug();		/* This cannot happen msg -- message.c	*/

/*
 * io functions
 */
extern void fil_del();		/* delete a file -- io.c		*/
extern void tmp_del();		/* delete all tmp files -- io.c		*/
extern FILE *tmp_make();	/* make a temp file -- io.c		*/
extern void append();		/* append a file to the data base	*/
extern void put_code();		/* write a code -- io.c			*/

/*
 * other external functions
 */
extern int cpp();		/* run input through cpp -- subr.c	*/
extern void i_end();		/* finish up the INTLINFO db -- main.c	*/
extern void ic_init();		/* start of an INTLINFO db -- main.c	*/
extern char *strsave();		/* save a string -- subr.c		*/
extern long fillen();		/* get size of a file -- subr.c		*/

/*
 * globally known anchors to the lists
 */
extern sym *cod_anc;		/* pointer to code definition		*/
extern sym *cod_list;		/* pointer to head of code definition	*/
extern sym *prp_anc;		/* pointer to head of property info	*/
extern sym *col_anc;		/* pointer to head of collation list	*/
extern sym *frm_anc;		/* pointer to head of format definition	*/
extern sym *cnv_anc;		/* pointer to head of conversion tables	*/

/*
 * various global values
 */
extern sym **codeset;		/* codeset table			*/
extern i_dbhead i_hdr;		/* head of the data base		*/
extern int equal;		/* true if same weight required 	*/
extern i_char prm_wgt;		/* primary weight			*/
extern i_char sec_wgt;		/* secondary weight			*/
