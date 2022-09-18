#ifndef lint
static	char	*sccsid = "@(#)access.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 * Adb: access data in file/process address space.
 *
 * The routines in this file access referenced data using
 * the maps to access files, ptrace to access subprocesses,
 * or the system page tables when debugging the kernel,
 * to translate virtual to physical addresses.
 */

#include "defs.h"


MAP		kdb_txtmap;
MAP		kdb_datmap;
STRING		kdb_errflg;

/*
 * Primitives: put a value in a space, get a value from a space
 * and get a word or byte not returning if an error occurred.
 */
kdbput(addr, space, value) 
off_t addr; 
{ 
	(void) kdb_access(WT, addr, space, value); 
}

u_int
get(addr, space)
off_t addr; 
{ 
	return (kdb_access(RD, addr, space, 0)); 
};

u_int
chkget(addr, space)
off_t addr; 
{ 
	u_int w = get(addr, space); 
	chkerr(); 
	return(w); 
}

u_int
bchkget(addr, space) 
off_t addr; 
{ 
	return(chkget(addr, space) & LOBYTE); 
}

/*
 * Read/write according to mode at address addr in i/d space.
 * Value is quantity to be written, if write.
 *
 * This routine decides whether to get the data from the subprocess
 * address space with ptrace, or to get it from the files being
 * debugged.  
 *
 * When the kernel is being debugged with the -k flag we interpret
 * the system page tables for data space, mapping p0 and p1 addresses
 * relative to the ``current'' process (as specified by its p_addr in
 * <p) and mapping system space addresses through the system page tables.
 */
kdb_access(mode, addr, space, value)
int mode, space, value;
off_t addr;
{
	int rd = mode == RD;
	int  w;

	if (space == NSP)
		return(0);
	/*
		 * we have access to everything
		 */
	w = 0;
	if (!chkmap(&addr, space))
		return (0);
	if (rd)
		w = *(int *)addr;
	else	*(int *)addr = value;
	return (w);
}

rwerr(space)
int space;
{

	if (space & DSP)
		kdb_errflg = "data address not found";
	else
		kdb_errflg = "text address not found";
}

chkmap(addr,space)
register L_INT	*addr;
register INT		space;
{
	register MAPPTR amap;
	amap=((space&DSP?&kdb_datmap:&kdb_txtmap));
	if( space&STAR || !within(*addr,amap->b1,amap->e1) ){
		if( within(*addr,amap->b2,amap->e2) ){
			*addr += (amap->f2)-(amap->b2);
		} 
		else { 
			rwerr(space); 
			return(0);
		}
	} 
	else { 
		*addr += (amap->f1)-(amap->b1);
	}
	return(1);
}

within(addr,lbd,ubd)
u_int addr, lbd, ubd; 
{ 
	return(addr>=lbd && addr<ubd); 
}

