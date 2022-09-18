/*
 * @(#)sh.char.h	4.1  (ULTRIX)        7/17/90
 */
/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: sh.char.h,v 1.3 86/07/11 10:25:28 dce Exp $ */
/*
 * Modification History
 */

/*
 * Table for spotting special characters quickly
 *
 * Makes for very obscure but efficient coding.
 */

extern unsigned short _cmap[];

#define _Q	0x01		/* '" */
#define _Q1	0x02		/* ` */
#define _SP	0x04		/* space and tab */
#define _NL	0x08		/* \n */
#define _META	0x10		/* lex meta characters, sp #'`";&<>()|\t\n */
#define _GLOB	0x20		/* glob characters, *?{[` */
#define _ESC	0x40		/* \ */
#define _DOL	0x80		/* $ */
#define _DIG   0x100		/* 0-9 */
#define _LET   0x200		/* a-z, A-Z, _ */

#define cmap(c, bits)	(_cmap[(unsigned char)(c)] & (bits))

#define isglob(c)	cmap(c, _GLOB)
#define isspace(c)	cmap(c, _SP)
#define isspnl(c)	cmap(c, _SP|_NL)
#define ismeta(c)	cmap(c, _META)
#define digit(c)	cmap(c, _DIG)
#define letter(c)	cmap(c, _LET)
#define alnum(c)	(digit(c) || letter(c))
