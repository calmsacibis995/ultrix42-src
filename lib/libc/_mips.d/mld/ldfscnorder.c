/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ldfscnorder.c,v 2010.2.1.3 89/11/29 14:28:36 bettina Exp $ */

#include "scnhdr.h"
#include "reloc.h"

/*
 * ldfscnorder() returns a non-zero value if the order of the "raw data" and
 * "relocation entries" for a Mips object file is in the following order:
 *	raw data for .text section
 *	raw data for .init section
 *	raw data for .rdata section
 *	raw data for .data section
 *	raw data for 8 byte literal pool
 *	raw data for 4 byte literal pool
 *	raw data for .sdata section
 *	raw data for .lib section
 *	relocation entries for .text section
 *	relocation entries for .init section
 *	relocation entries for .rdata section
 *	relocation entries for .data section
 *	relocation entries for .sdata section
 * and all the file offsets are 4 byte aligned.
 *
 * Ldfscnorder() is passed pointers section headers for the .text, .init,
 * .rdata, .data, .lit8, .lit4, .sdata and .lib in the object file to check,
 * in that order.  If any of the pointers values is zero it is assumed that
 * section is not present.
 */
int
ldfscnorder(t, i, rd, d, l8, l4, sd, l)
SCNHDR *t, *i, *rd, *d, *l8, *l4, *sd, *l;
{
    int	offset = 0;

	if(t != (SCNHDR *)0 && t->s_size != 0){
	    offset = t->s_scnptr;
	    if((offset & 3) != 0)
		return(0);
	    offset += t->s_size;
	    if((offset & 3) != 0)
		return(0);
	}
	if(i != (SCNHDR *)0 && i->s_size != 0){
	    if(offset != 0 && offset != i->s_scnptr)
		return(0);
	    else
		offset = i->s_scnptr;
	    if((offset & 3) != 0)
		return(0);
	    offset += i->s_size;
	    if((offset & 3) != 0)
		return(0);
	}
	if(rd != (SCNHDR *)0 && rd->s_size != 0){
	    if(offset != 0 && offset != rd->s_scnptr)
		return(0);
	    else
		offset = rd->s_scnptr;
	    if((offset & 3) != 0)
		return(0);
	    offset += rd->s_size;
	    if((offset & 3) != 0)
		return(0);
	}
	if(d != (SCNHDR *)0 && d->s_size != 0){
	    if(offset != 0 && offset != d->s_scnptr)
		return(0);
	    else
		offset = d->s_scnptr;
	    if((offset & 3) != 0)
		return(0);
	    offset += d->s_size;
	    if((offset & 3) != 0)
		return(0);
	}
	if(l8 != (SCNHDR *)0 && l8->s_size != 0){
	    if(offset != 0 && offset != l8->s_scnptr)
		return(0);
	    else
		offset = l8->s_scnptr;
	    if((offset & 3) != 0)
		return(0);
	    offset += l8->s_size;
	    if((offset & 3) != 0)
		return(0);
	}
	if(l4 != (SCNHDR *)0 && l4->s_size != 0){
	    if(offset != 0 && offset != l4->s_scnptr)
		return(0);
	    else
		offset = l4->s_scnptr;
	    if((offset & 3) != 0)
		return(0);
	    offset += l4->s_size;
	    if((offset & 3) != 0)
		return(0);
	}
	if(sd != (SCNHDR *)0 && sd->s_size != 0){
	    if(offset != 0 && offset != sd->s_scnptr)
		return(0);
	    else
		offset = sd->s_scnptr;
	    if((offset & 3) != 0)
		return(0);
	    offset += sd->s_size;
	    if((offset & 3) != 0)
		return(0);
	}
	if(l != (SCNHDR *)0 && l->s_size != 0){
	    if(offset != 0 && offset != l->s_scnptr)
		return(0);
	    else
		offset = l->s_scnptr;
	    if((offset & 3) != 0)
		return(0);
	    offset += l->s_size;
	    if((offset & 3) != 0)
		return(0);
	}
	if(t != (SCNHDR *)0 && t->s_nreloc != 0){
	    if(offset != 0 && offset != t->s_relptr)
		return(0);
	    else
		offset = t->s_relptr;
	    if((offset & 3) != 0)
		return(0);
	    offset += t->s_nreloc * RELSZ;
	    if((offset & 3) != 0)
		return(0);
	}
	if(i != (SCNHDR *)0 && i->s_nreloc != 0){
	    if(offset != 0 && offset != i->s_relptr)
		return(0);
	    else
		offset = i->s_relptr;
	    if((offset & 3) != 0)
		return(0);
	    offset += i->s_nreloc * RELSZ;
	    if((offset & 3) != 0)
		return(0);
	}
	if(rd != (SCNHDR *)0 && rd->s_nreloc != 0){
	    if(offset != 0 && offset != rd->s_relptr)
		return(0);
	    else
		offset = rd->s_relptr;
	    if((offset & 3) != 0)
		return(0);
	    offset += rd->s_nreloc * RELSZ;
	    if((offset & 3) != 0)
		return(0);
	}
	if(d != (SCNHDR *)0 && d->s_nreloc != 0){
	    if(offset != 0 && offset != d->s_relptr)
		return(0);
	    else
		offset = d->s_relptr;
	    if((offset & 3) != 0)
		return(0);
	    offset += d->s_nreloc * RELSZ;
	    if((offset & 3) != 0)
		return(0);
	}
	if(sd != (SCNHDR *)0 && sd->s_nreloc != 0){
	    if(offset != 0 && offset != sd->s_relptr)
		return(0);
	    else
		offset = sd->s_relptr;
	    if((offset & 3) != 0)
		return(0);
	    offset += sd->s_nreloc * RELSZ;
	    if((offset & 3) != 0)
		return(0);
	}

	return(1);
}
