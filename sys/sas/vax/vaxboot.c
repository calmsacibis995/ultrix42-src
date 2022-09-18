/*
 * vaxboot.c
 */

#ifndef lint
static char *sccsid = "@(#)vaxboot.c	4.1	(ULTRIX)	7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
#include "vmb.h"
#include "../../h/param.h"
#include <a.out.h>

char    ultrixboot[] = "ultrixboot";
char    vmb[] = "vmb.exe";

char   *imagename;
extern int      mode;


/*
 * Functional Discription:
 *	This is the main of the `vaxboot' program.  It's function is to
 *	load either ultrixboot or vmb.exe depending on the boot mode.
 *	ROM_BOOT = load vmb.exe
 *	VMB_BOOT = load ultrixboot
 *	R11 and R10 are preserved
 *
 * Inputs:
 *	none
 *
 * Outputs:
 *	none
 *
 */
main () {
	register        howto, devtype;		/* howto=r11, devtype=r10 */
	int     io;

	if (mode & VMB_BOOT)
		imagename = ultrixboot;
	else
		if (mode & ROM_BOOT)
			imagename = vmb;
	if ((io = open (imagename, 0)) < 0)	/* Open the image */
		stop();
	if (mode & ROM_BOOT) {
		if (load_vmb(io))		/* 750 style boot */
			start_vmb();		/* go start it */
	} else
		copyunix (howto, devtype, io);	
	stop();
}

extern int      vmbinfo;

/*
 * Functional Discription:
 *	This routine loads ultrixboot.
 *
 * Inputs:
 *	R11,R10 are preserved
 *	io channel
 *
 * Outputs:
 *	-1 returned on error
 *	otherwise ultrixboot is called
 *
 */
copyunix (howto, devtype, io)
register        howto, devtype, io;		/* howto=r11, devtype=r10 */
{
	struct exec     x;
	register int    i;
	char   *addr;

	i = read (io, (char *) & x, sizeof x);
	if (i != sizeof x ||
	    (x.a_magic != 0407 && x.a_magic != 0413 && x.a_magic != 0410))
		return (-1);
	if (x.a_magic == 0413 && lseek (io, 0x400, 0) == -1)
		goto shread;
	if (read (io, (char *) 0, x.a_text) != x.a_text)
		goto shread;
	addr = (char *) x.a_text;
	if (x.a_magic == 0413 || x.a_magic == 0410)
		while ((int) addr & CLOFSET)
			*addr++ = 0;
	if (read (io, addr, x.a_data) != x.a_data)
		goto shread;
	addr += x.a_data;
	x.a_bss += 128 * 512;			/* slop */
	for (i = 0; i < x.a_bss; i++)
		*addr++ = 0;
	x.a_entry &= 0x7fffffff;
	(*((int (*) ()) x.a_entry)) (vmbinfo);
	stop();
shread: 
	return (-1);
}

/*
 * Functional Discription:
 *	For a ROM_BOOT mode, vmb.exe is loaded by this routine.
 *	It always assumes good memory and is loaded at 0x200.
 *
 * Inputs:
 *	io channel
 *
 * Outputs:
 *	0 = bad load
 *	1 = good load
 *
 */
load_vmb (io)
int     io;
{
	int     cnt, i;
	char   *addr = (char *) 0x200;

	for (i = 0;; i += 10240) {
		if ((cnt = read (io, addr + i, 10240)) < 0)
			return(0);	/* no good */
		if (!cnt)
			break;
	}
	return(1);
}
