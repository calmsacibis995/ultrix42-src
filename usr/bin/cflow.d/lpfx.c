#ifndef	lint
static char *sccsid = "@(#)lpfx.c	4.2	ULTRIX 2/14/91";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1984,1988 by			*
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
 *   This software is  derived  from  software  received from  Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with AT&T.		*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *
 *			Modification History
 *  5-Feb-91    Chris Clark
 *      Added new strings to the array types which handles basic types,
 *      for "signed", "vararg", and "void" (mips) 
 *      and for "const *" and "volatile *" (vax)
 *      This should prevent the strcpy for crashing when this routine is
 *      is handed a void type.
 *
 * 14-Nov-88	Tim Newhouse
 *	Changed offsets,shifts for looking at language types to use
 *	symbolic references instead of hard coded numbers.  These
 *	symbolic masks are defined in the 'manifest' include file.
 *      Also used conditional compile lines to make the definition
 *      of tstrbuf happy even when not being built vax.
 *
 *	Stephen Reilly, 19-Apr-84
 * 000- Modified this module to handle FLEXNAMES
 *
 ***********************************************************************/

#include <stdio.h>
#include "manifest"
#include "lmanifest"

#ifndef	FLEXNAMES
#define FNSIZE LFNM
#define NSIZE LCHNM
#endif

typedef struct {
	union rec r;

#ifdef	FLEXNAMES
	char *fname;
#else
	char fname[FNSIZE + 1];
#endif
	} funct;

typedef struct LI {
	struct LI *next;
	funct fun;
	} li;

#ifdef	FLEXNAMES
char *getstr();
#endif

/*
 * lpfx - read lint1 output, sort and format for dag
 *
 *	options -i_ -ix (inclusion)
 *
 *	while (read lint1 output into "funct" structures)
 *		if (this is a filename record)
 *			save filename
 *		else
 *			read arg records and throw on floor
 *			if (this is to be included)
 *				copy filename into "funct"
 *				insert into list
 *	format and print
 */


main(argc, argv)
	int argc;
	char **argv;
	{
	extern int optind;
	extern char *optarg;
	funct fu;
#ifdef FLEXNAMES
	char *filename, *strncpy();
#else
	char filename[FNSIZE + 1], *strncpy();
#endif
	int uscore, fmask, c;
	void rdargs(), insert(), putout();

	fmask = LDI | LDX | LRV;
	uscore = 0;
	while ((c = getopt(argc, argv, "i:")) != EOF)
		if (c == 'i')
			if (*optarg == '_')
				uscore = 1;
			else if (*optarg == 'x')
				fmask &= ~(LDI | LDX);
			else
				goto argerr;
		else
		argerr:
			(void)fprintf(stderr, "lpfx: bad option %c ignored\n", c);
	while (0 < fread((char *)&fu.r, sizeof(fu.r), 1, stdin))
		if (fu.r.l.decflag & LFN)
			{
#ifdef	FLEXNAMES
			filename = getstr();
#else
			(void)strncpy(filename, fu.r.f.fn, FNSIZE);
			filename[FNSIZE] = '\0';
#endif
			}
		else
			{
#ifdef	FLEXNAMES
			fu.r.l.name = getstr();
#endif
			rdargs(&fu);
			if (((fmask & LDI) ? ISFTN(fu.r.l.type.aty) :
			      !(fu.r.l.decflag & fmask)) &&
			    ((uscore) ? 1 : (*fu.r.l.name != '_')))
				{
#ifdef	FLEXNAMES
				fu.fname = filename;
#else
				(void)strncpy(fu.fname, filename, FNSIZE);
				fu.fname[FNSIZE] = '\0';
#endif
				insert(&fu);
				}
			}
	putout();
	}

/*
 * rdargs - read arg records and throw on floor
 *
 *	if ("funct" has args)
 *		get absolute value of nargs
 *		if (too many args)
 *			panic and die
 *		read args into temp array
 */

void rdargs(pf)
	register funct *pf;
	{
	struct ty atype[50];
	void exit();

		if (pf->r.l.nargs < 0)
			pf->r.l.nargs = -pf->r.l.nargs;
		if (pf->r.l.nargs > 50)
			{
			(void)fprintf(stderr, "lpfx: PANIC nargs=%d\n", pf->r.l.nargs);
			exit(1);
			}
		fread((char *)atype, sizeof(ATYPE), pf->r.l.nargs, stdin);
	}

/*
 * insert - insertion sort into (singly) linked list
 *
 *	stupid linear list insertion
 */

static li *head = NULL;

void insert(pfin)
	register funct *pfin;
	{
	register li *list_item, *newf;
	char *malloc();
	void exit();

	if ((newf = (li *)malloc(sizeof(li))) == NULL)
		{
		(void)fprintf(stderr, "lpfx: out of heap space\n");
		exit(1);
		}
	newf->fun = *pfin;
	if (list_item = head)
		if (newf->fun.r.l.fline < list_item->fun.r.l.fline)
			{
			newf->next = head;
			head = newf;
			}
		else
			{
			while (list_item->next &&
			  list_item->next->fun.r.l.fline < newf->fun.r.l.fline)
				list_item = list_item->next;
			while (list_item->next &&
			  list_item->next->fun.r.l.fline == newf->fun.r.l.fline &&
			  list_item->next->fun.r.l.decflag < newf->fun.r.l.decflag)
					list_item = list_item->next;
			newf->next = list_item->next;
			list_item->next = newf;
			}
	else
		{
		head = newf;
		newf->next = NULL;
		}
	}

/*
 * putout - format and print sorted records
 *
 *	while (there are records left)
 *		copy name and null terminate
 *		if (this is a definition)
 *			if (this is a function**)
 *				save name for reference formatting
 *			print definition format
 *		else if (this is a reference)
 *			print reference format
 *
 *	** as opposed to external/static variable
 */

void putout()
	{
	register li *pli;
#ifdef	FLEXNAMES
	char *lname, *prtype(), *strncpy(), *name;
#else
	char lname[NSIZE+1], *prtype(), *strncpy(), *strcpy(), name[NSIZE+1];
#endif	
	pli = head;
#ifdef	FLEXNAMES
	name = lname = '\0';
#else
	name[0] = lname[0] = '\0';
#endif
	while (pli != NULL)
		{
#ifdef	FLEXNAMES
		name = pli->fun.r.l.name;
#else
		(void)strncpy(name, pli->fun.r.l.name, NSIZE);
		name[NSIZE] = '\0';
#endif
		if (pli->fun.r.l.decflag & (LDI | LDC ))
			{
			if (ISFTN(pli->fun.r.l.type.aty))
#ifdef	FLEXNAMES
				lname = name;
#else
				(void)strcpy(lname, name);
#endif
			(void)printf("%s = %s, <%s %d>\n", name, prtype(pli),
			    pli->fun.fname, pli->fun.r.l.fline);
			}
		else if (pli->fun.r.l.decflag & (LUV | LUE | LUM))
			(void)printf("%s : %s\n", lname, name);
		pli = pli->next;
		}
	}

static char *types[] = {
	"undef", "farg", "char", "short", "int", "long", "float",
	"double", "struct", "union", "enum", "moety", "uchar",
	"ushort", "uint", "ulong",
#if mips
        "signed", "vararg", "void"
#endif
#if vax
	/* 16 and 18 are not defined in manifest */
	"???", "const*", "???", "volatile*"
#endif
        };

/*
 * prtype - decode type fields
 *
 *	strictly arcana
 */

char *prtype(pli)
	register li *pli;
	{
	static char bigbuf[64];
	char buf[32], *shift(), *strcpy(), *strcat();
	register char *bp;
	register int typ;

	typ = pli->fun.r.l.type.aty;
	(void)strcpy(bigbuf, types[typ & BTMASK]);
	*(bp = buf) = '\0';
	for (typ >>= BTSHIFT; typ > 0; typ >>= TSHIFT)
		switch (typ & 03)
			{
			case 1:
				bp = shift(buf);
				buf[0] = '*';
				break;
			case 2:
				*bp++ = '(';
				*bp++ = ')';
				*bp = '\0';
				break;
			case 3:
				*bp++ = '[';
				*bp++ = ']';
				*bp = '\0';
				break;
			}
	(void)strcat(bigbuf, buf);
	return(bigbuf);
	}

char *shift(s)
	register char *s;
	{
	register char *p1, *p2;
	char *rp;

	for (p1 = s; *p1; ++p1)
		;
	rp = p2 = p1++;
	while (p2 >= s)
		*p1-- = *p2--;
	return(++rp);
	}
#ifdef FLEXNAMES
char *
getstr()
{
	char buf[BUFSIZ];
	register char *cp = buf;
	register int c;

	if (feof(stdin) || ferror(stdin))
		return("");
	while ((c = getchar()) > 0)
		*cp++ = c;
	if (c < 0) {
		perror("intermediate file format error (getstr)");
		exit(1);
	}
	*cp++ = 0;
	return (hash(buf));
}

#define	NSAVETAB	4096
char	*savetab;
int	saveleft;

char *
savestr(cp)
	register char *cp;
{
	register int len;

	len = strlen(cp) + 1;
	if (len > saveleft) {
		saveleft = NSAVETAB;
		if (len > saveleft)
			saveleft = len;
		savetab = (char *)malloc(saveleft);
		if (savetab == 0) {
			perror("ran out of memory (savestr)");
			exit(1);
		}
	}
	strncpy(savetab, cp, len);
	cp = savetab;
	savetab += len;
	saveleft -= len;
	return (cp);
}

/*
 * The definition for the segmented hash tables.
 */
#define	MAXHASH	20
#define	HASHINC	1013
struct ht {
	char	**ht_low;
	char	**ht_high;
	int	ht_used;
} htab[MAXHASH];

char *
hash(s)
	char *s;
{
	register char **h;
	register i;
	register char *cp;
	struct ht *htp;
	int sh;

	sh = hashstr(s) % HASHINC;
	cp = s;
	/*
	 * There are as many as MAXHASH active
	 * hash tables at any given point in time.
	 * The search starts with the first table
	 * and continues through the active tables
	 * as necessary.
	 */
	for (htp = htab; htp < &htab[MAXHASH]; htp++) {
		if (htp->ht_low == 0) {
			register char **hp =
			    (char **) calloc(sizeof (char **), HASHINC);
			if (hp == 0) {
				perror("ran out of memory (hash)");
				exit(1);
			}
			htp->ht_low = hp;
			htp->ht_high = htp->ht_low + HASHINC;
		}
		h = htp->ht_low + sh;
		/*
		 * quadratic rehash increment
		 * starts at 1 and incremented
		 * by two each rehash.
		 */
		i = 1;
		do {
			if (*h == 0) {
				if (htp->ht_used > (HASHINC * 3)/4)
					break;
				htp->ht_used++;
				*h = savestr(cp);
				return (*h);
			}
			if (**h == *cp && strcmp(*h, cp) == 0)
				return (*h);
			h += i;
			i += 2;
			if (h >= htp->ht_high)
				h -= HASHINC;
		} while (i < HASHINC);
	}
	perror("ran out of hash tables");
	exit(1);
}
#ifdef mips
char **tstrbuf;
#else
char	*tstrbuf[1];
#endif

#endif
