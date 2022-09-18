#ifndef lint
static char *sccsid = "@(#)fdtype.c	4.1      ULTRIX 	10/16/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 * File:	fdtype.c
 * Author:	Adrian Thoms (thoms@wessex)
 * Description:
 *	This is derived directly from filetype.c of the file(1)
 *	utility.
 *	It offers a library interface to the magic number part of the
 *	file(1) file guessing algorithm
 *
 * Modification History:
 *
 * 28-Sep-90 - Adrian Thoms (thoms@wessex)
 *	Fixed unaligned access problem (Ref. #001)
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#ifdef vax
#include <a.out.h>
#endif vax
#include "magic_data.h"
#include "filetype.h"

/*
**	Structure of magic file entry
*/

Entry	*mtab;

char *execmodes();
int	ifile = -1;
int maxexecfn = 2;

/*
 * Functions defined here:
 */
long fdtype();
int binary_mktab();


long
fdtype(fd, pmbuf, pcount, buf, bufsiz, pflg)
	int fd;
	struct stat *pmbuf;
	int *pcount;
	char *buf;
	unsigned bufsiz;
	int pflg;
{
	reg int i;
	reg int in=0;
	static struct stat dummy_mbuf;
	long retnum=0;

	if (pmbuf == NULL) pmbuf = &dummy_mbuf;
	in = read(fd, buf, bufsiz);
	*pcount = in;

	if(in == 0){
		if (pflg) printf("empty\n");
		return(MASH(EMPTY, UNKNOWN));
	}
	if (retnum = ckmtab(pmbuf, pflg, buf))
		return(retnum);

	if (buf[0] == '\037' && buf[1] == '\235') {
		if (buf[2]&0x80 && pflg)
			printf("block ");
		if (pflg) printf("compressed %d bit code data\n", buf[2]&0x1f);
		return(MASH(COMPRESSED, UNKNOWN));
	}
	for(i=0; i < in; i++)
		if(buf[i]&0200){
			if (pflg) printf("data\n"); 
			return(MASH(DATA, UNKNOWN));
		};
	return(0);
}


#ifdef HAVE_MAGIC
extern Entry_init magic_tab[];

binary_mkmtab()
{
	mtab = (Entry *)magic_tab;
}
#endif

char *
execmodes(mbuf, fbuf)
struct stat *mbuf;
char *fbuf;
{
	static char msg_buf[26];

	sprintf(msg_buf, "%s%s%s",
		(mbuf->st_mode & S_ISUID) ? "setuid " : "",
		(mbuf->st_mode & S_ISGID) ? "setgid " : "",
		(mbuf->st_mode & S_ISVTX) ? "sticky " : "");
	return(msg_buf);
}

#ifdef vax
char *
symtable(mbuf, fbuf)
struct stat *mbuf;
char *fbuf;
{
	struct exec ex;
	struct stat stb;

	ex = *(struct exec *)fbuf;
	if (fstat(ifile, &stb) < 0)
		return("");

	if (((int *)fbuf[4])!= 0)
 	if ((int)N_STROFF(ex)+sizeof(off_t) > stb.st_size)
		return ("version 7 style symbol table");
	return ("");
}
#else !vax
char *
symtable(mbuf, fbuf)
struct stat *mbuf;
char *fbuf;
{
	return("");
}
#endif vax
char *(*execfns[])() = { symtable, symtable, execmodes };

long
ckmtab(mbuf, pflg, buf)
struct stat *mbuf;
int pflg;
char *buf;
{

	reg	Entry	*ep;
	reg	char	*p;
	reg	int	lev1 = 0;
	int fn;
	long retcode;

	auto	union	{
		long	l;
		char	ch[4];
		}	val, revval;

	struct matcher {
		char *start, *finish;
		} *match;
	char pbuf[256]; /* these fixed length arrays are bad. fix then sometime */
	char sbuf[256];
	char *sptr;
	int slen = 0;
	int match_len;

	for(ep = mtab; ep->e_off > -2L * (long)maxexecfn; ep++) {
		if(lev1) {
			if(ep->e_level != 1)
				break;
			if (pflg && (slen > 0)) putchar(' ');
			slen = 0;
		} else if(ep->e_level == 1)
			continue;

		if (pflg && ep->e_off < 0L) {
			fn = -1 * ep->e_off;
			if (fn > maxexecfn)
				sprintf(sbuf, ep->e_str, " ???? ");
			else
				sprintf(sbuf, ep->e_str, (char *)(*execfns[fn])(mbuf, buf));
			slen = strlen(sbuf);
			printf(sbuf);
			continue;
			}
			
		p = &buf[ep->e_off];
		switch(ep->e_type) {
		case STR:
		{
			int fre_exec_retval; /* Fix #001 */
	
			fre_exec_retval = fre_exec(p,ep->e_value.str);
			switch (fre_exec_retval) {
			case 0:
			case -1:
				continue;
			default:
				break;
			}
			match = (struct matcher *)fre_exec_retval;
			match_len = match->finish - match->start;
			strncpy(pbuf, match->start, match_len);
			pbuf[match_len] = '\0';
			break;
		}

		case BYTE:
			val.l = (long)(*(unsigned char *) p);
			break;

		case SHORT:
			val.l = (long)(*(unsigned short *) p);
			break;

		case LONG:
			val.l = (*(long *) p);
			break;
		}

		if (ep->e_type != STR)
		switch(ep->e_opcode & ~SUB) {
		case EQ:
			if(val.l != ep->e_value.num)
				continue;
			break;
		case GT:
			if(val.l <= ep->e_value.num)
				continue;
			break;

		case LT:
			if(val.l >= ep->e_value.num)
				continue;
			break;
		}
		if (pflg) {
			if(ep->e_opcode & SUB) { /* we really need the SysV printf here to return */
					         /* the number of characters printed.		*/
				sprintf(sbuf, ep->e_str, (ep->e_type == STR) ? (int)pbuf : val.l);
				sptr = sbuf;
				}
			else
				sptr=ep->e_str;
			slen = strlen(sptr);
			printf(sptr);
		}
		retcode = ep->e_retcode;
		lev1 = 1;
	}
	if(lev1) {
		if (pflg) putchar('\n');
		return(retcode);
	}
	return(0);
}
