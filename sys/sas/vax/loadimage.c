/*
 * loadimage.c
 */

#ifndef lint
static char *sccsid = "@(#)loadimage.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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


/***********************************************************************
 *
 * Modification History:
 *
 * 06-Jun-90  rafiey (Ali Rafieymehr)
 *	For VAXes, flush "rei" has to be performed for instruction streams.
 *	We have not done them before and we have been fortunate. VAX9000 
 *	wouldn't boot without fixing the problem.
 *
 **********************************************************************/


#include "../h/param.h"
#include <a.out.h>
#include "vmb.h"


/*
 * Functional Discription:
 *	This routine reads in a loads an ultrix image, usually vmunix.
 *
 * Inputs:
 *	R11,R10 and the io channel
 *
 * Outputs:
 *	-1 returned on error
 *	otherwise, the image is called.
 *
 */
load_image (howto, devtype, io)
	register howto, devtype, io;		/* howto=r11, devtype=r10 */
{
	struct exec     x;
	register int    i;
	char   *addr;
	int	size;
	extern	struct	vmb_info *vmbinfo;

	i = read (io, (char *) & x, sizeof x);
	if (i != sizeof x ||
		(x.a_magic != 0407 && x.a_magic != 0413 && x.a_magic != 0410)){
#ifdef SECONDARY
		printf ("Bad a.out format\n");
#endif SECONDARY
		return (-1);
	}
	size = x.a_text + x.a_data + x.a_bss;
	if (size > vmbinfo->memsiz) {
#ifdef SECONDARY
		printf("\nImage size of %d bytes exceeds available\n",
		       size);
		printf("    contiguous memory, which is %d bytes.\n\n",
		       vmbinfo->memsiz);
#endif SECONDARY
		return (-1);
	}

#ifdef SECONDARY
	printf ("\nSizes:\ntext = %d\n", x.a_text);
#endif SECONDARY
	if (x.a_magic == 0413 && lseek (io, 0x400, 0) == -1)
		goto shread;
	if (read (io, (char *) 0, x.a_text) != x.a_text)
		goto shread;
	addr = (char *) x.a_text;
	if (x.a_magic == 0413 || x.a_magic == 0410)
		while ((int) addr & CLOFSET)
			*addr++ = 0;
#ifdef SECONDARY
	printf ("data = %d\n", x.a_data);
#endif SECONDARY
	if (read (io, addr, x.a_data) != x.a_data)
		goto shread;
	addr += x.a_data;
#ifdef SECONDARY
	printf ("bss  = %d\n", x.a_bss);
#endif SECONDARY
	x.a_entry &= 0x7fffffff;
#ifdef SECONDARY
	printf ("Starting at 0x%x\n\n", x.a_entry);
#endif SECONDARY
	flush_istream();
	(*((int (*) ()) x.a_entry)) (vmbinfo);
	stop();
shread: 
#ifdef SECONDARY
	printf ("Short read\n");
#endif SECONDARY
	return (-1);
}

