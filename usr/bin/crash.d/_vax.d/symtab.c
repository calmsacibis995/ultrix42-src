#ifndef lint
static char *sccsid = "@(#)symtab.c	4.1	(ULTRIX)	7/17/90";
#endif

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

#include "../crash.h"
#include <a.out.h>
#include <sys/file.h>
#include <sys/smp_lock.h>
#include <sys/gnode.h>
#include <sys/text.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/buf.h>

struct nlist *stbl;
extern char *namelist;
int symcnt;
char *strngtab;
int strtbsz;
int nfd = -1;

rdsymtab()
{

	struct exec filehdr;
	struct nlist *sp;
	char *malloc();
	
	if ((nfd = open (namelist,O_RDONLY,0)) < 0)
		fatal("cannot open object file");
	if (read (nfd, (char *)&filehdr, sizeof(filehdr)) != sizeof(filehdr))
		fatal("error reading object file");
	if (filehdr.a_magic != NMAGIC)
		fatal("object file not in correct format");

	if ((stbl=(struct nlist *)malloc((unsigned)filehdr.a_syms)) == NULL)
		fatal("not enough space for symbol table");
        lseek (nfd, (int)N_SYMOFF(filehdr),0);
        if (read (nfd, (char *)stbl, (int)filehdr.a_syms) != filehdr.a_syms)
                fatal("error reading symbol table");
        symcnt = filehdr.a_syms / sizeof(struct nlist);

	if (read(nfd, (char *)&strtbsz, sizeof(strtbsz)) != sizeof(strtbsz))
                fatal ("object file has no string table");
	if (strtbsz) {
		if ((strngtab = malloc((unsigned)strtbsz)) == NULL)
                	fatal("cannot allocate space for string table");
	        strtbsz -= sizeof(strtbsz);
		if (read (nfd, (char *)(strngtab + sizeof(strtbsz)), strtbsz)
		    != strtbsz)
                        fatal("error reading string table"); 
	}

        for (sp = stbl; sp < &stbl[symcnt]; sp++) {
                if (sp->n_un.n_strx >= strtbsz)
                     fatal("invalid offset in symbol table");
                if (sp->n_un.n_strx)
                     sp->n_un.n_name = strngtab + sp->n_un.n_strx;
        }

/*	close (nfd);*/
}

struct Symbol found;

struct Symbol *
symsrch(s)
register char *s;
{
	register struct nlist *sp;
	for(sp = stbl; sp < &stbl[symcnt]; sp++) {
		if (/*(sp->n_type & N_STAB) == 0   /not sure about this*/
                     strncmp(sp->n_un.n_name, s, strlen(s)) == 0) {
			found.s_name = sp->n_un.n_name;
			found.s_value = sp->n_value;
			found.s_type = sp->n_type;
			return(&found);
		}
	}
	return(NULL);
}
/*
 * locate all symbol table entries containing a given string
 */
struct nlist *last_grep;
struct Symbol *
symgrep(s)
register char *s;
{
	register struct nlist *sp;

	if (last_grep == 0)
		last_grep=stbl;
	else
		last_grep++;
	
	for(sp = last_grep; sp < &stbl[symcnt]; sp++) {
		if (strncmp(sp->n_un.n_name, s, strlen(s)) == 0) {
			found.s_name = sp->n_un.n_name;
			found.s_value = sp->n_value;
			found.s_type = sp->n_type;
			last_grep=sp;
			return(&found);
			break;
		}
	}
	last_grep=NULL;
	return(NULL);
}

prtabaddr(table, addr)
	struct tabsum *table;
	unsigned addr;
{
	int index;
	int offset;

	index = (addr - table->first)/table->size;
	offset = addr - (table->first + (index * table->size));
	if (offset !=0)
		printf ("%s[%d] + %d", table->name, index, offset);
	else
		printf ("%s[%d]", table->name, index);
}

prmapaddr(map, addr)
	struct mapsum *map;
	unsigned addr;
{
	int index;
	int offset;

	index = (addr - map->first)/1024;
	offset = addr - (map->first + (index * 1024));
	if (offset != 0)
		printf ("*%s[%d] + %d (%s)",
			map->name, index, offset, map->descrip);
	else
		printf ("*%s[%d] (%s)",
			map->name, index, map->descrip);
}

prsym(sp, addr)
	register struct Symbol *sp;
	unsigned addr;
{
	unsigned i;
	if(sp == 0)
		printf("no match");
	else if(sp->s_name[0] == '_')
		printf("%s", &sp->s_name[1]);
	else
		printf("%s", sp->s_name);
	if(sp) {
		i = addr - sp->s_value;
		if (i) printf(" + 0x%x", i);
	}
}

prval (sp)
register struct Symbol *sp;
{
	register char *cp;

	printf("%08.8lx  ", sp->s_value);
	switch(sp->s_type & S_TYPE) {
	case S_TEXT:
		cp = " text";
		break;
	case S_DATA:
		cp = " data";
		break;
	case S_BSS:
		cp = " bss";
		break;
	case S_UNDF:
		cp = " undefined";
		break;
	case S_ABS:
		cp = " absolute";
		break;
	default:
		cp = " type unknown";
	}
	printf("%s", cp);	
}

prnm(s)
	char *s;
{
	register struct Symbol *sp;
	struct tabloc t;
	extern struct Symbol *nmsrch();
	printf("%-14s ", s);
	if (parse_tabaddr(s, &t)) {
		printf("%08.8lx", t.addr);		
	} else {
		if((sp = nmsrch(s)) == NULL) {
			printf("no match");
			return;
		}
		prval(sp);
	}
}

struct tabsum *
searchtabs(addr)
	unsigned addr;
{
	int i;
	for (i = 0; i < TABSUM_MAX; i++) {
		if ((tab[i].first <= addr) && (tab[i].last > addr))
		return (&tab[i]);
	}
	return ((struct tabsum *)NULL);
}

struct mapsum *
searchmaps(addr)
	unsigned addr;
{
	int i;
	for (i = 0; i < MAPSUM_MAX; i++) {
		if ((map[i].first <= addr) && (map[i].last >= addr))
		return (&map[i]);
	}
	return ((struct mapsum *)NULL);
}

struct Symbol *
nmsrch(s)
	register  char  *s;
{
	char	ct[20];
	register  struct  Symbol  *sp;

	if(strlen(s) > 19)
		return(0);
	if((sp = symsrch(s)) == NULL) {
		strcpy(ct, "_");
		strcat(ct, s);
		sp = symsrch(ct);
	}
	return(sp);
}


struct Symbol *
search(addr)
unsigned addr;
{
	register struct nlist *sp;
	unsigned value;

	value = 0;
	for(sp = stbl; sp < &stbl[symcnt]; sp++) {
		if((sp->n_type & N_EXT) && (sp->n_value <= addr)
		   && (sp->n_value > value)) {
			value = sp->n_value;
			found.s_name = sp->n_un.n_name;
			found.s_value = sp->n_value;
			found.s_type = sp->n_type;
			if (sp->n_value == addr)
				return(&found);
		}
	}

	if (value == 0)
		return(NULL);
	else
		return(&found);
}
struct tableoff {
	int	slot;
	int	offset;
};

praddr(addr)
	unsigned addr;
{
	struct Symbol  *sp;
	struct tabsum *table;
	struct mapsum *map;
	struct tableoff where;
	
 	getoff(&where, (int) addr, gnodebuckets, GNODEBUCKETS,
	    sizeof(struct gnode));
	if(where.slot != -1) {
		if (where.offset != 0)
			printf("gnode[%d]+0x%-4x", where.slot, where.offset);
		else
			printf("gnode[%d]", where.slot);
		return;
	}
 	getoff(&where, (int) addr, filebuckets, FILEBUCKETS,
	    tab[FILE_T].size);
	if(where.slot != -1) {
		if (where.offset != 0)
			printf("file[%d]+0x%-4x", where.slot, where.offset);
		else
			printf("file[%d]", where.slot);
		return;
	}
 	getoff(&where, (int) addr, mountbuckets, MOUNTBUCKETS,
	    sizeof(struct mount));
	if(where.slot != -1) {
		if (where.offset != 0)
			printf("mount[%d]+0x%-4x", where.slot, where.offset);
		else
			printf("mount[%d]", where.slot);
		return;
	}
 	getoff(&where, (int) addr, bufbuckets, BUFBUCKETS,
	    sizeof(struct buf));
	if(where.slot != -1) {
		if (where.offset != 0)
			printf("buf[%d]+0x%-4x", where.slot, where.offset);
		else
			printf("buf[%d]", where.slot);
		return;
	}
 	getoff(&where, (int) addr, procbuckets, PROCBUCKETS,
	    sizeof(struct proc));
	if(where.slot != -1) {
		if (where.offset != 0)
			printf("proc[%d]+0x%-4x", where.slot, where.offset);
		else
			printf("proc[%d]", where.slot);
		return;
	}
	
	if (table = searchtabs(addr))
		prtabaddr(table, addr);
	else if (map = searchmaps(addr))
		prmapaddr(map, addr);
	else {
		sp = search(addr);
		if (sp != 0)
			prsym(sp,addr);
		else
			printf("0x%x",addr);
	}
}


getoff(tp, addr, bh, bucketsize, size)
	struct tableoff *tp;
	int addr;
	struct buckets *bh[];
	int bucketsize, size;
{
	register struct buckets *bp;
	register int i;
	
	for(i = 0; i < bucketsize; i++) {
		bp = bh[i];
		while(bp != NULL) {
			if((addr >= (int) bp->addr) &&
			    (addr < ((int) bp->addr + size))) {
				tp->slot = bp->index;
				tp->offset = addr - (int)bp->addr;
				return;
			}
			bp = bp->next;
		}
	}
	tp->slot = -1;
}

/*
 * Given an address and a description of a kernel array, return
 * the index of the array element that <addr> points to.  Return
 * -1 if <addr> is not within the array bounds or does not point
 * to the first byte of an element.
 */
int
symindex(sp, len, size, addr)
	struct Symbol *sp;
	int size, len;
	unsigned addr;
{
	unsigned first = sp->s_value;
	unsigned last = first + (size * len) - 1;
	int index;

	if ((addr < first) || (addr > last))
		return(-1);
	index = (addr - first) / size;
	if ((first + index*size) != addr)
		return(-1);
	return(index);
}












