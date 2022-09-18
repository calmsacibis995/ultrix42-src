#ifndef lint
static char *sccsid = "@(#)symtab.c	4.2	(ULTRIX)	7/17/90";
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

#include  <a.out.h>
#include "../crash.h"
#include <sys/file.h>
#include <sys/gnode.h>
#include <sys/text.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/buf.h>
#ifdef mips
#include <ldfcn.h>
#endif mips

extern char *namelist;
int symcnt;
char *strngtab;
int strtbsz;
int nfd = -1;

LDFILE *ldptr;

rdsymtab()
{
	pSYMR *symp;
	int i=0;

        if ((ldptr = ldopen(namelist, ldptr)) == NULL) 
		fatal("cannot open object file");

	symcnt = SYMHEADER(ldptr).isymMax+SYMHEADER(ldptr).iextMax;

}

struct Symbol *
symsrch (sname)
char	*sname;

{
    static struct Symbol s;
    char	*name;
    pSYMR	psym;
    SYMR	asym;
    int		isymMax;		/* max # of regular symbols */
    int		iextMax;		/* max # of externals */
    int		iMax;			/* total # of wanted syms + externals */
    int		iBase;			/* base index of sym or extern wanted */
    int		iextBase;		/* base index of externs for f5 sorts */
    int		isym;			/* index through symbols in file */
    char	*ldgetname();

    if (ISARCHIVE(ldptr->type)) {
	return (NULL);
    } 


	if (sname[0] == '_')
	    sname++;

	isymMax = SYMHEADER(ldptr).isymMax;
	iextMax = SYMHEADER(ldptr).iextMax;

	/* determine range of symbols to read */
	iMax = isymMax + iextMax;
	iBase = isymMax;

	psym = &asym;

	for (isym = iBase; isym < iMax; isym++) {

	    if (ldtbread (ldptr, isym, psym) != SUCCESS) {
		return (NULL);
	    } 

	    if (psym->st == stBlock && psym->index != indexNil) {
		isym = psym->index - 1;
		continue;
	    } else if (isym < isymMax && psym->st != stStatic &&
		psym->st != stStaticProc) {
		continue;
	    }
	    if (strcmp(name = ldgetname (ldptr, psym), sname) == 0) {
		s.s_value = psym->value;
		s.s_type = 1;
		s.s_name = name;
		return(&s);
	    } 

	}
    return (NULL);

}

int last_grep;

struct Symbol *
symgrep (sname)
char	*sname;

{
	static struct Symbol s;
	char	*name;
	pSYMR	psym;
	SYMR	asym;
	int	isymMax;	/* max # of regular symbols */
	int	iextMax;	/* max # of externals */
	int	iMax;		/* total # of wanted syms + externals */
	int	iBase;		/* base index of sym or extern wanted */
	int	iextBase;	/* base index of externs for f5 sorts */
	int	isym;		/* index through symbols in file */
	char	*ldgetname();

	if (ISARCHIVE(ldptr->type)) {
		return (NULL);
	} 


	if (sname[0] == '_')
		sname++;

	isymMax = SYMHEADER(ldptr).isymMax;
	iextMax = SYMHEADER(ldptr).iextMax;

	/* determine range of symbols to read */
	iMax = isymMax + iextMax;
	iBase = isymMax;

	psym = &asym;
	if (last_grep == 0)
		last_grep = iBase;
	else
		last_grep++;

	for (isym = last_grep; isym < iMax; isym++) {

	    if (ldtbread (ldptr, isym, psym) != SUCCESS) {
		return (NULL);
	    } 

	    if (psym->st == stBlock && psym->index != indexNil) {
		isym = psym->index - 1;
		continue;
	    } else if (isym < isymMax && psym->st != stStatic &&
		psym->st != stStaticProc) {
		continue;
	    }
	    if (strncmp(name = ldgetname (ldptr, psym), sname, 
			strlen(sname)) == 0) {
		s.s_value = psym->value;
		s.s_type = 1;
		s.s_name = name;
		last_grep = isym;
		return(&s);
	    } 

	}
    return (NULL);

}

prtabaddr(table, addr)
	struct tabsum *table;
	unsigned addr;
{
	int index;
	int offset;

	index = (addr - table->first)/table->size;
	offset = addr - (table->first + (index * table->size));
	if (offset == 0)
		printf ("\t%s[%d]", table->name, index);
	else
		printf ("\t%s[%d] + %d", table->name, index, offset);
}

prmapaddr(map, addr)
	struct mapsum *map;
	unsigned addr;
{
	int index;
	int offset;

	index = (addr - map->first)/1024;
	offset = addr - (map->first + (index * 1024));
	if (offset == 0)
		printf ("\t*%s[%d] (%s)", map->name, index, map->descrip);
	else
		printf ("\t*%s[%d] + %d (%s)",
			map->name, index, offset, map->descrip);
}

prsym(sp, addr)
	register struct Symbol *sp;
	unsigned addr;
{
	unsigned i;
	if(sp == 0)
		printf("\tno match\n");
	else if(sp->s_name[0] == '_')
		printf("\t%s", &sp->s_name[1]);
	else
		printf("\t%s", sp->s_name);
	if(sp) {
		i = addr - sp->s_value;
		if (i) printf(" + 0x%x.", i);
	}
}

prval (sp)
register struct Symbol *sp;
{
	register char *cp;

	printf("%08.8lx  ", sp->s_value);
#ifdef vax
	switch(sp->s_type & N_TYPE) {
	case N_TEXT:
		cp = " text";
		break;
	case N_DATA:
		cp = " data";
		break;
	case N_BSS:
		cp = " bss";
		break;
	case N_UNDF:
		cp = " undefined";
		break;
	case N_ABS:
		cp = " absolute";
		break;
	default:
		cp = " type unknown";
	}
#endif vax
	printf("%s", cp);	
}

struct Symbol *
nmsrch(s)
	register  char  *s;
{
	char	ct[200];
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


prnm(s)
	char *s;
{
	register struct Symbol *sp;
	struct tabloc t;

	printf("%-14s ", s);
	if (parse_tabaddr(s, &t)) {
		printf("%08.8lx", t.addr);		
	} else {
		if((sp = nmsrch(s)) == NULL) {
			printf("no match\n");
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
search(addr)
unsigned addr;
{
    struct Symbol s;
    char	*name;
    pSYMR	psym;
    SYMR	asym;
    int		isymMax;		/* max # of regular symbols */
    int		iextMax;		/* max # of externals */
    int		iMax;			/* total # of wanted syms + externals */
    int		iBase;			/* base index of sym or extern wanted */
    int		iextBase;		/* base index of externs for f5 sorts */
    int		isym;			/* index through symbols in file */
    char	*ldgetname();
    unsigned	value=0;
    int	found=0;
    static struct Symbol save;
    if (ISARCHIVE(ldptr->type)) {
	return (NULL);
    } 


	isymMax = SYMHEADER(ldptr).isymMax;
	iextMax = SYMHEADER(ldptr).iextMax;

	/* determine range of symbols to read */
	iMax = isymMax + iextMax;
	iBase = isymMax;

	psym = &asym;

	for (isym = iBase; isym < iMax; isym++) {

	    if (ldtbread (ldptr, isym, psym) != SUCCESS) {
		    printf("ldtbread: failed\n");
		return (NULL);
	    } 

	    if (psym->st == stBlock && psym->index != indexNil) {
		isym = psym->index - 1;
		continue;
	    } else if (isym < isymMax && psym->st != stStatic &&
		psym->st != stStaticProc) {
		continue;
	    }

	    if((psym->value <= addr) && (psym->value > value)) {
		value = psym->value;
		save.s_value = psym->value;
		save.s_type = 1;
		save.s_name = ldgetname (ldptr, psym);
		found=1;
		if (psym->value == addr)
		    return(&save);
	    } 

	}
    if (found)
    return (&save);
    else
	    return(NULL);

}

struct tableoff {
	int	slot;
	int	offset;
};

praddr(addr)
	unsigned addr;
{
	struct Symbol  *sp=NULL;
	struct tabsum *table;
	struct mapsum *map;
	struct tableoff where;
	
 	getoff(&where, (int) addr, gnodebuckets, GNODEBUCKETS,
	    sizeof(struct gnode));
	if(where.slot != -1) {
		if (where.offset == 0)
			printf("\tgnode[%d]", where.slot);
		else
			printf("\tgnode[%d]+0x%x", where.slot, where.offset);
		return;
	}
 	getoff(&where, (int) addr, filebuckets, FILEBUCKETS,
	    tab[FILE_T].size);
	if(where.slot != -1) {
		if (where.offset == 0)
			printf("\tfile[%d]", where.slot);
		else
			printf("\tfile[%d]+0x%x", where.slot, where.offset);
		return;
	}
 	getoff(&where, (int) addr, mountbuckets, MOUNTBUCKETS,
	    sizeof(struct mount));
	if(where.slot != -1) {
		if (where.offset == 0)
			printf("\tmount[%d]", where.slot);
		else
			printf("\tmount[%d]+0x%x", where.slot, where.offset);
		return;
	}
 	getoff(&where, (int) addr, bufbuckets, BUFBUCKETS,
	    sizeof(struct buf));
	if(where.slot != -1) {
		if (where.offset == 0)
			printf("\tbuf[%d]", where.slot);
		else
			printf("\tbuf[%d]+0x%x", where.slot, where.offset);
		return;
	}
 	getoff(&where, (int) addr, procbuckets, PROCBUCKETS,
	    sizeof(struct proc));
	if(where.slot != -1) {
		if (where.offset == 0)
			printf("\tproc[%d]", where.slot);
		else
			printf("\tproc[%d]+0x%x", where.slot, where.offset);
		return;
	}
	
	if (table = searchtabs(addr))
		prtabaddr(table, addr);
	else if (map = searchmaps(addr))
		prmapaddr(map, addr);
	else {
		sp = search(addr);
		if (sp != NULL)
			prsym(sp,addr);
		else
			printf("None.");
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




