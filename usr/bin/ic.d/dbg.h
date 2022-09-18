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
 * @(#)dbg.h	4.1	(ULTRIX)	7/17/90
 *
 *	Header file for debugging functions
 *
 */

#ifndef EBUG
#define EBUG 1
#endif

extern int cnv_dbg;		/* debug conversion tables		*/
extern int cod_dbg;		/* debug code table			*/
extern int col_dbg;		/* debug collation table		*/
extern int frm_dbg;		/* debug format table			*/
extern int fct_dbg;		/* debug functions			*/
extern int lex_dbg;		/* debug lexical analyser		*/
extern int sym_dbg;		/* debug symbol entries			*/
extern int val_dbg;		/* debug the value fields		*/
extern int yac_dbg;		/* debug parser				*/

#define DBGFUN1		1	/* debug function entry and exit only	*/
#define DBGIN		1	/* increment debugging indent		*/
#define DBGOUT		2	/* decrement debugging indent		*/
#define DBGSAME		4	/* stay at current debugging indent	*/
#define DBGTIN		8	/* temporary indentation		*/
#define DBGNOID		16	/* do not print indentation		*/

/*
 * debugging functions
 */
extern void col_dmp();		/* dump a collation table -- col.c DEBUG*/
extern void frm_dmp();		/* dump a format table -- frm.c DEBUG	*/
extern void cnv_dmp();		/* dump a conversion -- cnv.c DEBUG	*/
extern void dbg_prt();		/* indented debug output -- dbg.c DEBUG	*/
extern void val_dmp();		/* dump a value list -- dbg.c DEBUG	*/
extern void sym_dmp();		/* dump a symbol -- dbg.c DEBUG		*/
