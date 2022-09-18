/*	@(#)machine.h	4.1	(ULTRIX)	7/17/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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

#ifndef machine_h
#define machine_h
typedef unsigned int Address;
typedef unsigned char Byte;
typedef unsigned int Word;

#define NREG 16

#define ARGP 12
#define FRP 13
#define STKP 14
#define PROGCTR 15

#define BITSPERBYTE 8
#define BITSPERWORD (BITSPERBYTE * sizeof(Word))

#define nargspassed(frame) argn(0, frame)
#define mkuchar(i)  ((i) & 0x000000ff)
#define mkushort(i) ((i) & 0x0000ffff)
/*
#include "source.h"
#include "symbols.h"
*/
Address pc;
Address prtaddr;
#ifdef DSFFDS
printinst(/* lowaddr, highaddr */);
printninst(/* count, addr */);
Address printdata(/* lowaddr, highaddr, format */);
printndata(/* count, startaddr, format */);
printvalue(/* v, format */);
printerror(/*  */);
printsig (/* signo */);
endprogram(/*  */);
dostep(/* isnext */);
Address nextaddr(/* startaddr, isnext */);
setbp(/* addr */);
unsetbp(/* addr */);
Boolean isbperr(/*  */);
beginproc(/* p, argc */);
#endif
#endif
