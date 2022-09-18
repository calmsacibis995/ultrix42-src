/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldflitorder.c,v 2010.2.1.3 89/11/29 14:28:32 bettina Exp $ */

#include "scnhdr.h"

/*
 * ldflitorder() returns a the total size of the literal pools if they
 * are in the following order:
 *	raw data for 8 byte literal pool
 *	raw data for 4 byte literal pool
 * and all the file offsets are 4 byte aligned.
 * Else it returns -1.
 *
 * Ldflitorder() is passed pointers section headers for the .lit8 and .lit4
 * in the object file to check, in that order.  If any of the pointers values
 * is zero it is assumed that section is not present.
 */
int
ldflitorder(l8, l4)
SCNHDR *l8, *l4;
{
    int	offset = 0;

	if(l8 != (SCNHDR *)0 && l8->s_size != 0){
	    offset = l8->s_scnptr;
	    if((offset & 3) != 0)
		return(-1);
	    offset += l8->s_size;
	    if((offset & 3) != 0)
		return(-1);
	}
	if(l4 != (SCNHDR *)0 && l4->s_size != 0){
	    if(offset != 0 && offset != l4->s_scnptr)
		return(-1);
	    else
		offset = l4->s_scnptr;
	    if((offset & 3) != 0)
		return(-1);
	    offset += l4->s_size;
	    if((offset & 3) != 0)
		return(-1);
	}
	if (l8 == (SCNHDR *)0 && l4 == (SCNHDR *)0)
	    return 0;
	if (l8 == (SCNHDR *)0)
	    return l4->s_size;
	if (l4 == (SCNHDR *)0)
	    return l8->s_size;
	return(l8->s_size + l4->s_size);
}
