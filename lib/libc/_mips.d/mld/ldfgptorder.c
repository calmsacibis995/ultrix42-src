/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldfgptorder.c,v 2010.2.1.3 89/11/29 14:30:00 bettina Exp $ */

#include "scnhdr.h"

/*
 * ldfgptorder() checks to ensure the gp tables are canonically ordered.
 * Returns -1 if they are not and the total length if they are.  
 * It is passed pointers to the section headers of the data, small data,
 * small bss and bss sections.
 */
long
ldfgptorder(d_scnhdr, sd_scnhdr, sb_scnhdr, b_scnhdr)
SCNHDR *d_scnhdr, *sd_scnhdr, *sb_scnhdr, *b_scnhdr;
{

    long ptr, size;

	ptr = 0;

	if(d_scnhdr != (SCNHDR *)0 && d_scnhdr->s_nlnno != 0)
	    ptr = d_scnhdr->s_lnnoptr;
	else if(sd_scnhdr != (SCNHDR *)0 && sd_scnhdr->s_nlnno != 0)
	    ptr = sd_scnhdr->s_lnnoptr;
	else if(sb_scnhdr != (SCNHDR *)0 && sb_scnhdr->s_nlnno != 0)
	    ptr = sb_scnhdr->s_lnnoptr;
	else if(b_scnhdr != (SCNHDR *)0 && b_scnhdr->s_nlnno != 0)
	    ptr = b_scnhdr->s_lnnoptr;

	if(ptr == 0)
	    return(0);

	size = 0;
	if(d_scnhdr != (SCNHDR *)0 && d_scnhdr->s_nlnno != 0){
	    ptr += d_scnhdr->s_nlnno * GPTABSZ;
	    size += d_scnhdr->s_nlnno * GPTABSZ;
	}
	if(sd_scnhdr != (SCNHDR *)0 && sd_scnhdr->s_nlnno != 0){
	    if(ptr != sd_scnhdr->s_lnnoptr)
		return(-1);
	    ptr += sd_scnhdr->s_nlnno * GPTABSZ;
	    size += sd_scnhdr->s_nlnno * GPTABSZ;
	}
	if(sb_scnhdr != (SCNHDR *)0 && sb_scnhdr->s_nlnno != 0){
	    if(ptr != sb_scnhdr->s_lnnoptr)
		return(-1);
	    ptr += sb_scnhdr->s_nlnno * GPTABSZ;
	    size += sb_scnhdr->s_nlnno * GPTABSZ;
	}
	if(b_scnhdr != (SCNHDR *)0 && b_scnhdr->s_nlnno != 0){
	    if(ptr != b_scnhdr->s_lnnoptr)
		return(-1);
	    ptr += b_scnhdr->s_nlnno * GPTABSZ;
	    size += b_scnhdr->s_nlnno * GPTABSZ;
	}
	return(size);
}
