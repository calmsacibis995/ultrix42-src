/*
 * qdcons.c
 */

#ifndef lint
static char *sccsid = "@(#)qdcons.c	4.1	(ULTRIX)	7/2/90";
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
#include "../../h/types.h"
#define NVR_ADRS	0x200B8024

int qdputc(), qdgetc();

extern (*v_putc)(),(*v_getc)();


#define QDSSCSR  0x20001F00

int conspg = 0;

qd_init()
{
	register short *NVR;
	caddr_t qdaddr;

 	qdaddr = (caddr_t) QDSSCSR;
        if (badloc(qdaddr, sizeof(short)))
            return(0);

	v_getc = qdgetc;
	v_putc = qdputc;

	NVR = (short *) NVR_ADRS;
	conspg  = *NVR++ & 0xFF;
	conspg |= (*NVR++ & 0xFF) << 8;
	conspg |= (*NVR++ & 0xFF) << 16;
	conspg |= (*NVR++ & 0xFF) << 24;

	return(1);

}
