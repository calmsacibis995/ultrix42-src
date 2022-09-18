# ifndef lint
static char	*sccsid = "@(#)ps.c	4.4	(ULTRIX)	11/15/90";
# endif not lint

/************************************************************************
 *									*
 *		Copyright (c) 1985, 1987, 1988, 1990 by			*
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

/* ------------------------------------------------------------------------
 * Modification History: /usr/src/bin/ps.c
 *
 *  14 Nov 90 -- paradis
 *	Add a new column to the "STAT" display field: "V" indicates
 *	that the process is a vector process (only applicable to
 *	vector-capable VAX machines)
 *
 *  22 Feb 90 -- lan
 *	Fixed getcmd's locating of arg & envrion strings to handle
 *	common initial stack modifications (e.g., mail's changing
 *	argv[argc] to -1 from NULL and reorg of argv by shell shifts or
 *	Xtools, etc.  Also corrected two unreported bugs:
 *	 the char 0x80 was used as a str terminator; and
 *	 output u.u_comm in ()'s when cmd str[0] is unprintable, not
 *	  just less than a space char.
 *
 *   8 Dec 89 --jmartin
 *	Fix computation of physical address for "-k" option and new
 *	MIPS PTE format.
 *
 *  04 Dec 89 -- lan
 *	Changed to fetch entire user environment area allocated when
 *	process exec'd, including #args & pointers, not just strings;
 *	and have getcmd use this data when trying to get args./env.vars.
 *
 *  13 Nov 89 -- dws
 *	Removed hard coded names for system process; they are now named
 *	in the kernel.
 *
 *  09 Nov 89 -- bp
 *	Fixed getcmd to not fetch the user environment when a partial
 *	dump is being examined.  Also made the malloc in getcmd not
 * 	exit but to attempt and do something for the user.
 *
 *  16 Oct 89 -- bp
 *	Added code to fetch the entire user environment when required.
 *
 *  16 Aug 89 -- sue
 *	Made getname more efficient.  Replace getpwent loop with a
 *	getpwuid call.
 *
 *  12 Jun 89 -- gg
 *	fix vax/mips for dynamic swap change.
 *
 *  2 May 89 -- jaw
 *	fix mips for smp stack area change.
 *
 *  7 Apr 88 -- jmartin
 *	Fixed getu() to use HIGHPAGES to find the root of the user
 *	stack.  Added a comment and fixed a bogus comment in the
 *	vicinity.  Corrected VAX calculation of kernel stack size to
 *	stride by longwords.
 *
 * 22 Dec 87 -- Tim Burke
 *	Moved reference to controling terminal from the u struct to the proc
 *	struct.
 *
 * 14 Oct 85 --  rr
 *	Fix the addr that is printed in -l format so that it is
 *	the page frame number of the end of the preallocated user
 *	pages (which are the args+udot+stack). See pstat.c for
 *	corresponding fixes to dump the udot structure.
 *
 * 22 Feb 84 --  jmcg
 *	Fixed mapping of kernel virtual addresses when reading a core
 *	file.  This problem arises because of the sparse allocation
 *	of bufpages to system buffers.  This leaves some holes in
 *	kernel virtual address space so that kernel virtual pages
 *	are not one-to-one with physical memory pages.  I made the
 *	first version of this fix around 25 Jan 1984.  This version
 *	benefits from a fix published by A R White at University of
 *	Waterloo (net.bugs.4.2bsd <6927@watmath.UUCP> 17 Feb 1984).
 *	The main difference is that I read in the entire system page
 *	table.
 *
 * 22 Feb 84 -- jmcg
 *	Corrected two "cant read" messages to properly reflect where
 *	the reading had been attempted.  The first of these had been
 *	reported in a fix for the core dump reading problem by A R White
 *	at University of Waterloo (net.bugs.4.2bsd <6927@watmath.UUCP>
 *	17 Feb 1984).
 *
 * 22 Feb 84 --jmcg
 *	Derived from 4.2BSD, labeled:
 *		ps.c	4.26 (Berkeley) 9/25/83
 *
 * ------------------------------------------------------------------------
 */

/*
 * ps
 */
#include <stdio.h>
#include <ctype.h>
#include <nlist.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <machine/pte.h>
#include <sys/vm.h>
#include <sys/text.h>
#include <sys/stat.h>
#include <sys/mbuf.h>
#include <math.h>
#ifdef vax
#include <machine/vectors.h>
#endif /* vax */

typedef unsigned STK_QUANTUM;

#ifdef mips
#include <sys/fixpoint.h>
#endif mips

struct nlist nl[] = {
	{ "_proc" },
#define	X_PROC		0
	{ "_Usrptmap" },
#define	X_USRPTMA	1
	{ "_usrpt" },
#define	X_USRPT		2
	{ "_text" },
#define	X_TEXT		3
	{ "_nswap" },
#define	X_NSWAP		4
	{ "_maxslp" },
#define	X_MAXSLP	5
	{ "_ccpu" },
#define	X_CCPU		6
	{ "_ecmx" },
#define	X_ECMX		7
	{ "_nproc" },
#define	X_NPROC		8
	{ "_ntext" },
#define	X_NTEXT		9
	{ "_swapfrag" },
#define	X_SWAPFRAG	10
	{ "_Sysmap" },
#define SSYSMAP		11
	{ "_Syssize" },
#define SSYSSIZE	12
	{ "_proc_bitmap" },
#define X_PROC_BITMAP	13
	{ "_partial_dumpmag" },
#define	X_PARTIAL_DUMPMAG 14
	{ "_dumpmag" },
#define	X_DUMPMAG	15
	{ "" },
};

struct	savcom {
	union {
		struct	lsav *lp;
		float	u_pctcpu;
		struct	vsav *vp;
		int	s_ssiz;
	} s_un;
	struct	asav *ap;
} *savcom;

struct	asav {
	char	*a_cmdp;
	int	a_flag;
	short	a_stat, a_uid, a_pid, a_nice, a_pri, a_slptime, a_time;
	size_t	a_size, a_rss, a_tsiz, a_txtrss;
#ifdef mips
        int     a_datapt,a_stakpt,a_textpt;
#endif mips
	short	a_xccount;
	char	a_tty[MAXNAMLEN+1];
	dev_t	a_ttyd;
	time_t	a_cpu;
	size_t	a_maxrss;
	unsigned a_vproc:1;		/* Vector process (JRP) */
	unsigned a_vrun:1;		/* Running vector process (JRP) */
};

char	*lhdr;
struct	lsav {
	short	l_ppid;
	char	l_cpu;
	int	l_addr;
	caddr_t	l_wchan;
};

char	*uhdr;
char	*shdr;

char	*vhdr;
struct	vsav {
	u_int	v_majflt;
	size_t	v_swrss, v_txtswrss;
	float	v_pctcpu;
};

#define NPROC 16
struct	proc proc[NPROC];		/* a few, for less syscalls */
struct	proc *mproc;
struct	text *text;

union {
	struct	user user;
	char	upages[UPAGES][NBPG];
} user;
#define u	user.user

#ifdef vax
#define clear(x) 	((int)x & 0x7fffffff)
#endif vax

int	partial_dumpmag, dumpmag;
int	chkpid;
int	aflg, cflg, eflg, gflg, kflg, lflg, sflg,
	uflg, vflg, xflg;
char	*tptr;
char	*gettty(), *getcmd(), *getname(), *savestr(), *state();
char	*rindex(), *calloc(), *sbrk(), *strcpy(), *strcat(), *strncat();
char	*index(), *ttyname(), mytty[16];
long	lseek();
double	pcpu(), pmem();
int	pscomp();
int	nswap, maxslp;
struct	text *atext;
double	ccpu;
int	ecmx;
struct	pte *Usrptma, *usrpt;
int	nproc, ntext;
int	swapfrag;

struct	ttys {
	char	name[MAXNAMLEN+1];
	dev_t	ttyd;
	struct	ttys *next;
	struct	ttys *cand;
} *allttys, *cand[16];

int	npr;

int	cmdstart;
int	twidth;
char	*kmemf, *memf, *swapf, *nlistf;
int	kmem, mem, swap = -1;
int	rawcpu, sumcpu;

int	pcbpf;
int	argaddr;
extern	char _sobuf[];

#if (NBPG < 1024)
#define	pgtok(a)	((a)/(1024/NBPG))
#else
#define	pgtok(a)	((a)*(NBPG/1024))
#endif

main(argc, argv)
	char **argv;
{
	register int i, j;
	register char *ap;
	int uid;
	int proc_bit;
	off_t procp;
	int width;

	twidth = 80;
	argc--, argv++;
	if (argc > 0) {
		ap = argv[0];
		while (*ap) switch (*ap++) {

		case 'C':
			rawcpu++;
			break;
		case 'S':
			sumcpu++;
			break;
		case 'a':
			aflg++;
			break;
		case 'c':
			cflg = !cflg;
			break;
		case 'e':
			eflg++;
			break;
		case 'g':
			gflg++;
			break;
		case 'k':
			kflg++;
			break;
		case 'l':
			lflg++;
			break;
		case 's':
			sflg++;
			break;
		case 't':
			if (*ap)
				tptr = ap;
			else if ((tptr = ttyname(2)) != 0) {
				strcpy(mytty, tptr);
				if ((tptr = index(mytty,'y')) != 0)
					tptr++;
			}
			aflg++;
			gflg++;
			if (tptr && *tptr == '?')
				xflg++;
			while (*ap)
				ap++;
			break;
		case 'u':
			uflg++;
			break;
		case 'v':
			cflg = 1;
			vflg++;
			break;
		case 'w':
			if (twidth == 80)
				twidth = 132;
			else
				twidth = -1;
			break;
		case 'x':
			xflg++;
			break;
		default:
			if (!isdigit(ap[-1]))
				break;
			chkpid = atoi(--ap);
			*ap = 0;
			aflg++;
			xflg++;
			break;
		}
	}
	openfiles(argc, argv);
	getkvars(argc, argv);
	if (chdir("/dev") < 0) {
		perror("/dev");
		exit(1);
	}
	getdev();
	uid = getuid();
	printhdr();
	partial_dumpmag = getw(nl[X_PARTIAL_DUMPMAG].n_value);
	dumpmag = getw(nl[X_DUMPMAG].n_value);
	procp = getw(nl[X_PROC].n_value);
	nproc = getw(nl[X_NPROC].n_value);
	savcom = (struct savcom *)calloc((unsigned)nproc,sizeof(struct savcom));
	for (i=0; i<nproc; i += NPROC) {
		klseek(kmem, (long)procp, 0);
		j = nproc - i;
		if (j > NPROC)
			j = NPROC;
		j *= sizeof (struct proc);
		if (read(kmem, (char *)proc, j) != j) {
			cantread("proc table", kmemf);
			exit(1);
		}
		procp += j;
		for (j = j / sizeof (struct proc) - 1; j >= 0; j--) {
			/* read up bitmap array entry for this proc
			   entry.  this is done by indexing into the
			   bit map array */
			proc_bit = getw(((int)nl[X_PROC_BITMAP].n_value) +
					((i/32)*4));
			/* if slot bit is clear then unused! */
			if ((proc_bit & (1 << ((i+j) % 32))) == 0)
				continue;
			mproc = &proc[j];
			if (mproc->p_stat == 0 ||
			    mproc->p_pgrp == 0 && xflg == 0)
				continue;
			if (tptr == 0 && gflg == 0 && xflg == 0 &&
			    mproc->p_ppid == 1)
				continue;
			if (uid != mproc->p_uid && aflg==0 ||
			    chkpid != 0 && chkpid != mproc->p_pid)
				continue;
			if (vflg && gflg == 0 && xflg == 0) {
				if (mproc->p_stat == SZOMB ||
				    mproc->p_type&SWEXIT)
					continue;
				if (mproc->p_slptime > MAXSLP &&
				    (mproc->p_stat == SSLEEP ||
				     mproc->p_stat == SSTOP))
				continue;
			}
			save();
		}
	}
        if (twidth > 0) {
		width = twidth - cmdstart - 2;
	        if (width < 0) width = 0;
	}
	qsort(savcom, npr, sizeof(savcom[0]), pscomp);
	for (i=0; i<npr; i++) {
		register struct savcom *sp = &savcom[i];
		if (lflg)
			lpr(sp);
		else if (vflg)
			vpr(sp);
		else if (uflg)
			upr(sp);
		else
			spr(sp);
		if (sp->ap->a_flag & SWEXIT)
			printf(" <exiting>");
		else if (sp->ap->a_stat == SZOMB)
			printf(" <defunct>");
		else if (twidth > 0)
			printf(" %.*s", width, sp->ap->a_cmdp);
		else printf(" %s", sp->ap->a_cmdp);
		printf("\n");
	}
	exit(npr == 0);
}

getw(loc)
	unsigned long loc;
{
	long word;

	klseek(kmem, (long)loc, 0);
	if (read(kmem, (char *)&word, sizeof (word)) != sizeof (word))
		printf("error reading kmem at %x\n", loc);
	return (word);
}

struct pte *Sysmap = 0;

klseek(fd, loc, off)
	int fd;
	long loc;
	int off;
{
	static int	sizeSysmap;

#ifdef vax
	if( kflg && Sysmap == 0)
		{/* initialize Sysmap */

		sizeSysmap = nl[SSYSSIZE].n_value * sizeof( struct pte);
		Sysmap = (struct pte *)calloc((unsigned)sizeSysmap, 1);
		lseek( kmem, clear( nl[SSYSMAP].n_value), 0);
		if( read( kmem, Sysmap, sizeSysmap) != sizeSysmap)
			{
			cantread( "system page table", kmemf);
			exit(1);
			}
		}
	if( kflg && (loc&0x80000000))
		{/* do mapping for kernel virtual addresses */
		struct pte *ptep;

		loc &= 0x7fffffff;
		ptep = &Sysmap[btop(loc)];
		if( (char *)ptep - (char *)Sysmap > sizeSysmap)
			{
			printf( "no system pte for %s\n", loc);
			exit(1);
			}
		if( ptep->pg_v == 0)
			{
			printf( "system pte invalid for %x\n", loc);
			exit(1);
			}
		loc = (off_t)((loc&PGOFSET) + ptob(ptep->pg_pfnum));
		}
	(void) lseek(fd, (long)loc, off);
#endif vax
#ifdef mips
	/* mkphys handles mapping on mips*/
	if (kflg && (loc & 0x80000000)) loc = mkphys(loc);
	(void) lseek(fd, loc, off);
#endif mips
}

openfiles(argc, argv)
	char **argv;
{

	kmemf = "/dev/kmem";
	if (kflg)
		kmemf = argc > 2 ? argv[2] : "/vmcore";
	kmem = open(kmemf, 0);
	if (kmem < 0) {
		perror(kmemf);
		exit(1);
	}
	if (kflg)  {
		mem = kmem;
		memf = kmemf;
	} else {
		memf = "/dev/mem";
		mem = open(memf, 0);
		if (mem < 0) {
			perror(memf);
			exit(1);
		}
	}
	if (kflg == 0 || argc > 3) {
		swapf = argc>3 ? argv[3]: "/dev/drum";
		swap = open(swapf, 0);
		if (swap < 0) {
			perror(swapf);
			exit(1);
		}
	}
}

getkvars(argc, argv)
	char **argv;
{
	register struct nlist *nlp;
#ifdef mips
	fix tccpu;	/* fix representation for mips */
#endif mips

	nlistf = argc > 1 ? argv[1] : "/vmunix";
	nlist(nlistf, nl);
	if (nl[0].n_type == 0) {
		fprintf(stderr, "%s: No namelist\n", nlistf);
		exit(1);
	}
	if (kflg)
		for (nlp = nl; nlp < &nl[sizeof (nl)/sizeof (nl[0])]; nlp++)
#ifdef vax
			nlp->n_value = clear(nlp->n_value);
#endif
#ifdef mips
			nlp->n_value = nlp->n_value;
#endif
	usrpt = (struct pte *)nl[X_USRPT].n_value;	/* don't clear!! */
	Usrptma = (struct pte *)nl[X_USRPTMA].n_value;
	klseek(kmem, (long)nl[X_NSWAP].n_value, 0);
	if (read(kmem, (char *)&nswap, sizeof (nswap)) != sizeof (nswap)) {
		cantread("nswap", kmemf);
		exit(1);
	}
	klseek(kmem, (long)nl[X_MAXSLP].n_value, 0);
	if (read(kmem, (char *)&maxslp, sizeof (maxslp)) != sizeof (maxslp)) {
		cantread("maxslp", kmemf);
		exit(1);
	}
	klseek(kmem, (long)nl[X_CCPU].n_value, 0);
#ifdef vax
	if (read(kmem, (char *)&ccpu, sizeof (ccpu)) != sizeof (ccpu)) {
		cantread("ccpu", kmemf);
		exit(1);
	}
#endif vax
#ifdef mips
	if (read(kmem, (char *)&tccpu, sizeof (fix)) != sizeof (fix)) {
		cantread("ccpu", kmemf);
		exit(1);
	}
        ccpu = FIX_TO_DBL(tccpu);
#endif mips
	klseek(kmem, (long)nl[X_ECMX].n_value, 0);
	if (read(kmem, (char *)&ecmx, sizeof (ecmx)) != sizeof (ecmx)) {
		cantread("ecmx", kmemf);
		exit(1);
	}
	if (uflg || vflg) {
		ntext = getw(nl[X_NTEXT].n_value);
		text = (struct text *)
			calloc((unsigned)ntext, sizeof(struct text));
		if (text == 0) {
			fprintf(stderr, "no room for text table\n");
			exit(1);
		}
		atext = (struct text *)getw(nl[X_TEXT].n_value);
		klseek(kmem, (long)atext, 0);
		if (read(kmem, (char *)text, ntext * sizeof (struct text))
		    != ntext * sizeof (struct text)) {
			cantread("text table", kmemf);
			exit(1);
		}
	}
	swapfrag = getw(nl[X_SWAPFRAG].n_value);
}

printhdr()
{
	char *hdr;

	if (sflg+lflg+vflg+uflg > 1) {
		fprintf(stderr, "ps: specify only one of s,l,v and u\n");
		exit(1);
	}
	hdr = lflg ? lhdr :
			(vflg ? vhdr :
				(uflg ? uhdr : shdr));
	if (lflg+vflg+uflg+sflg == 0)
		hdr += strlen("SSIZ ");
	cmdstart = strlen(hdr);
	printf("%s COMMAND\n", hdr);
	(void) fflush(stdout);
}

cantread(what, fromwhat)
	char *what, *fromwhat;
{

	fprintf(stderr, "ps: error reading %s from %s\n", what, fromwhat);
}

struct	direct *dbuf;
int	dialbase;

getdev()
{
	register DIR *df;

	dialbase = -1;
	if ((df = opendir(".")) == NULL) {
		fprintf(stderr, "Can't open . in /dev\n");
		exit(1);
	}
	while ((dbuf = readdir(df)) != NULL)
		maybetty();
	closedir(df);
}

/*
 * Attempt to avoid stats by guessing minor device
 * numbers from tty names.  Console is known,
 * know that r(hp|up|mt) are unlikely as are different mem's,
 * floppy, null, tty, etc.
 */
maybetty()
{
	register char *cp = dbuf->d_name;
	register struct ttys *dp;
	int x;
	struct stat stb;

	switch (cp[0]) {

	case 'c':
		if (!strcmp(cp, "console")) {
			x = 0;
			goto donecand;
		}
		/* cu[la]? are possible!?! don't rule them out */
		break;

	case 'd':
		if (!strcmp(cp, "drum"))
			return;
		break;

	case 'f':
		if (!strcmp(cp, "floppy"))
			return;
		break;

	case 'k':
		cp++;
		if (*cp == 'U')
			cp++;
		goto trymem;

	case 'r':
		cp++;
		if (*cp == 'r' || *cp == 'u' || *cp == 'h')
			cp++;
#define is(a,b) cp[0] == 'a' && cp[1] == 'b'
		if (is(r,p) || is(u,p) || is(r,k) || is(r,m) || is(m,t)) {
			cp += 2;
			if (isdigit(*cp) && cp[2] == 0)
				return;
		}
		break;

	case 'm':
trymem:
		if (cp[0] == 'm' && cp[1] == 'e' && cp[2] == 'm' && cp[3] == 0)
			return;
		if (cp[0] == 'm' && cp[1] == 't')
			return;
		break;

	case 'n':
		if (!strcmp(cp, "null"))
			return;
		break;

	case 'v':
		if ((cp[1] == 'a' || cp[1] == 'p') && isdigit(cp[2]) &&
		    cp[3] == 0)
			return;
		break;
	}
	cp = dbuf->d_name + dbuf->d_namlen - 1;
	x = 0;
	if (cp[-1] == 'd') {
		if (dialbase == -1) {
			if (stat("ttyd0", &stb) == 0)
				dialbase = stb.st_rdev & 017;
			else
				dialbase = -2;
		}
		if (dialbase == -2)
			x = 0;
		else
			x = 11;
	}
	if (cp > dbuf->d_name && isdigit(cp[-1]) && isdigit(*cp))
		x += 10 * (cp[-1] - ' ') + cp[0] - '0';
	else if (*cp >= 'a' && *cp <= 'f')
		x += 10 + *cp - 'a';
	else if (isdigit(*cp))
		x += *cp - '0';
	else
		x = -1;
donecand:
	dp = (struct ttys *)calloc(1,sizeof(struct ttys));
	(void) strcpy(dp->name, dbuf->d_name);
	dp->next = allttys;
	dp->ttyd = -1;
	allttys = dp;
	if (x == -1)
		return;
	x &= 017;
	dp->cand = cand[x];
	cand[x] = dp;
}

char *
gettty()
{
	register char *p;
	register struct ttys *dp;
	struct stat stb;
	int x;

	if ((int)mproc->p_ttyp == 0)
		return("?");
	x = u.u_ttyd & 017;
	for (dp = cand[x]; dp; dp = dp->cand) {
		if (dp->ttyd == -1) {
			if (stat(dp->name, &stb) == 0 &&
			   (stb.st_mode&S_IFMT)==S_IFCHR)
				dp->ttyd = stb.st_rdev;
			else
				dp->ttyd = -2;
		}
		if (dp->ttyd == u.u_ttyd)
			goto found;
	}
	/* ick */
	for (dp = allttys; dp; dp = dp->next) {
		if (dp->ttyd == -1) {
			if (stat(dp->name, &stb) == 0 &&
			   (stb.st_mode&S_IFMT)==S_IFCHR)
				dp->ttyd = stb.st_rdev;
			else
				dp->ttyd = -2;
		}
		if (dp->ttyd == u.u_ttyd)
			goto found;
	}
	return ("?");
found:
	p = dp->name;
	if (p[0]=='t' && p[1]=='t' && p[2]=='y')
		p += 3;
	return (p);
}

save()
{
	register struct savcom *sp;
	register struct asav *ap;
	register unsigned *cp;
	register struct text *xp;
	char *ttyp, *cmdp;
#ifdef vax
	struct vpcontext	vpc;	/* JRP */
#endif /* vax */

	if (mproc->p_stat != SZOMB && getu() == 0)
		return;
	ttyp = gettty();
	if (xflg == 0 && ttyp[0] == '?' || tptr && strncmp(tptr, ttyp, 2))
		return;
	sp = &savcom[npr];
	cmdp = getcmd();
	if (cmdp == 0)
		return;
	sp->ap = ap = (struct asav *)calloc(1,sizeof(struct asav));
	sp->ap->a_cmdp = cmdp;
#define e(a,b) ap->a = mproc->b
	ap->a_flag = mproc->p_sched | mproc->p_vm | mproc->p_trace |
		     mproc->p_type | mproc->p_file;
	e(a_stat, p_stat); e(a_nice, p_nice);
	e(a_uid, p_uid); e(a_pid, p_pid); e(a_pri, p_pri);
	e(a_slptime, p_slptime); e(a_time, p_time);
	ap->a_tty[0] = ttyp[0];
	ap->a_tty[1] = ttyp[1] ? ttyp[1] : ' ';
	if (ap->a_stat == SZOMB) {
		ap->a_cpu = 0;
	} else {
		ap->a_size = mproc->p_dsize + mproc->p_ssize;
		e(a_rss, p_rssize);
		ap->a_ttyd = u.u_ttyd;
		ap->a_cpu = u.u_ru.ru_utime.tv_sec + u.u_ru.ru_stime.tv_sec;
		if (sumcpu)
			ap->a_cpu += u.u_cru.ru_utime.tv_sec + u.u_cru.ru_stime.tv_sec;
		if (mproc->p_textp && text) {
			xp = &text[mproc->p_textp - atext];
			ap->a_tsiz = xp->x_size;
			ap->a_txtrss = xp->x_rssize;
			ap->a_xccount = xp->x_ccount;
		}
	}
#undef e
	ap->a_maxrss = mproc->p_maxrss;
	if (lflg) {
		register struct lsav *lp;

		sp->s_un.lp = lp = (struct lsav *)calloc(1,sizeof(struct lsav));
#define e(a,b) lp->a = mproc->b
		e(l_ppid, p_ppid); e(l_cpu, p_cpu);
		if (ap->a_stat != SZOMB)
			e(l_wchan, p_wchan);
#undef e
		lp->l_addr = pcbpf;
	} else if (vflg) {
		register struct vsav *vp;

		sp->s_un.vp = vp = (struct vsav *)calloc(1,sizeof(struct vsav));
#define e(a,b) vp->a = mproc->b
		if (ap->a_stat != SZOMB) {
			e(v_swrss, p_swrss);
			vp->v_majflt = u.u_ru.ru_majflt;
			if (mproc->p_textp)
				vp->v_txtswrss = xp->x_swrss;
		}
		vp->v_pctcpu = pcpu();
#undef e
	} else if (uflg)
		sp->s_un.u_pctcpu = pcpu();
	else if (sflg) {
		if (ap->a_stat != SZOMB) {
			for (cp = (STK_QUANTUM *)u.u_stack;
			    (char *)cp < &user.upages[UPAGES][0]; )
				if (*cp++)
					break;
			sp->s_un.s_ssiz = (&user.upages[UPAGES][0]-(char *)cp);
		}
	}

#ifdef vax
	if(mproc->p_vpcontext) {
		ap->a_vproc = 1;
		klseek(kmem, mproc->p_vpcontext, 0);
		read(kmem, &vpc, sizeof(struct vpcontext));
		if(vpc.vpc_state == VPC_LOAD) {
			ap->a_vrun = 1;
		}
	}
#else
	ap->a_vproc = 0;
	ap->a_vrun = 0;
#endif /* vax */

	npr++;
}

double
pmem(ap)
	register struct asav *ap;
{
	double fracmem;
	int szptudot;

	if ((ap->a_flag&SLOAD) == 0)
		fracmem = 0.0;
	else {
#ifdef vax
		szptudot = UPAGES + clrnd(ctopt(ap->a_size+ap->a_tsiz));
#endif vax
#ifdef mips
                szptudot = UPAGES + clrnd(ap->a_datapt+ap->a_stakpt);
#endif mips
		fracmem = ((float)ap->a_rss+szptudot)/CLSIZE/ecmx;
		if (ap->a_xccount)
			fracmem += ((float)ap->a_txtrss)/CLSIZE/
			    ap->a_xccount/ecmx;
	}
	return (100.0 * fracmem);
}

double
pcpu()
{
	time_t time;
        double pctcpu;

	time = mproc->p_time;
	if (time == 0 || (mproc->p_sched&SLOAD) == 0)
		return (0.0);
#ifdef mips
        pctcpu = FIX_TO_DBL(mproc->p_pctcpu);
#endif mips
#ifdef vax
        pctcpu = mproc->p_pctcpu;
#endif vax
	if (rawcpu)
		return (100.0 * pctcpu);
	return (100.0 * pctcpu / (1.0 - exp(time * log(ccpu))));
}

#ifdef vax
getu()
{
	register struct pte *pteaddr;
	struct pte apte;
	struct pte arguutl[CLSIZE+HIGHPAGES];
	int ncl, size;
	struct dmap l_dmap;
	int ublkno;

	size = sflg ? ctob(UPAGES) : sizeof (struct user);
	if ((mproc->p_sched & SLOAD) == 0) {
		if (swap < 0)
			return (0);
		klseek(kmem, (long)mproc->p_smap,0);
		read(kmem, &l_dmap, sizeof(struct dmap));
		klseek(kmem, (long)l_dmap.dm_ptdaddr,0);
		read(kmem, &ublkno, sizeof(int));
		(void) lseek(swap, (long)dtob(ublkno), 0);
		if (read(swap, (char *)&user.user, size) != size) {
			fprintf(stderr, "ps: cant read u for pid %d from %s\n",
			    mproc->p_pid, swapf);
			return (0);
		}
		pcbpf = 0;
		argaddr = 0;
		return (1);
	}
	if (kflg)
		mproc->p_p0br = (struct pte *)clear(mproc->p_p0br);
	pteaddr = &Usrptma[btokmx(mproc->p_p0br) + mproc->p_szpt - 1];
	klseek(kmem, (long)pteaddr, 0);
	if (read(kmem, (char *)&apte, sizeof(apte)) != sizeof(apte)) {
		printf("ps: cant read indir pte to get u for pid %d from %s\n",
		    mproc->p_pid, kmemf);
		return (0);
	}
	pcbpf = apte.pg_pfnum+1;
	/*
	 * pcbpf is the 1st page past the pages which map the user
	 * page table.  pstat will use this to seek to to get all the
	 * pte's of the u area.  Essentially, pstat has to do the rest
	 * of this routine in order to get the user structure.
	 */
	klseek(mem,(long)ctob(pcbpf)-(CLSIZE+HIGHPAGES)*sizeof(struct pte),0);
	if (read(mem, (char *)arguutl, sizeof(arguutl)) != sizeof(arguutl)) {
		printf("ps: cant read page table for u of pid %d from %s\n",
		    mproc->p_pid, memf);
		return (0);
	}
	/*
	 * argaddr is the physical address of the root page of the user
	 * stack.
	 */
	if (arguutl[0].pg_fod == 0 && arguutl[0].pg_pfnum)
		argaddr = ctob(arguutl[0].pg_pfnum);
	else
		argaddr = 0;
	ncl = (size + NBPG*CLSIZE - 1) / (NBPG*CLSIZE);

	for (pteaddr = &arguutl[UPAGES + ncl*CLSIZE];
	     pteaddr > &arguutl[UPAGES];
	     pteaddr -= CLSIZE) {
		klseek(mem, (long)ctob(pteaddr->pg_pfnum), 0);
		if (read(mem,
			 user.upages[pteaddr - &arguutl[CLSIZE + UPAGES]],
			 CLSIZE*NBPG)
		  != CLSIZE*NBPG) {
		    printf("ps: cant read page %d of u of pid %d from %s\n",
			    pteaddr->pg_pfnum, mproc->p_pid, memf);
		    return(0);
		}
	}
	return (1);
}
#endif vax

#ifdef mips
#define Usrptmap Usrptma
getu()
{
	struct pte *pteaddr, apte;
	struct pte arguutl[CLSIZE+HIGHPAGES], wpte[UPAGES];
	register int i;
	int ncl, size;
	long  off;
	struct dmap l_dmap;
	int ublkno;

	size = sflg ? ctob(UPAGES) : sizeof (struct user);
	if ((mproc->p_sched & SLOAD) == 0) {
		if (swap < 0)
			return (0);
		klseek(kmem, (long)mproc->p_smap,0);
		read(kmem, &l_dmap, sizeof(struct dmap));
		klseek(kmem, (long)l_dmap.dm_ptdaddr,0);
		read(kmem, &ublkno, sizeof(int));
		(void) lseek(swap, (long)dtob(ublkno), 0);
		if (read(swap, (char *)&user.user, size) != size) {
			fprintf(stderr, "ps: cant read u for pid %d from %s\n",
			    mproc->p_pid, swapf);
			return (0);
		}
		pcbpf = 0;
		argaddr = 0;
		return (1);
	}
	if ((mproc->p_type & SSYS) == 0) {
		pteaddr = &Usrptmap[btokmx(mproc->p_stakbr)+mproc->p_stakpt-1];
		klseek(kmem, (long)pteaddr, 0);
		if (read(kmem, (char *)&apte, sizeof(apte)) != sizeof(apte)) {
			printf(
			"ps: cant read indir pte to get u for pid %d from %s\n",
			 mproc->p_pid, kmemf);
			return (0);
		}
		off = (long)ctob(apte.pg_pfnum) + NBPG -
			((HIGHPAGES+1) * sizeof(struct pte));
		lseek(mem, off, 0);
		if (read(mem, (char *)arguutl, sizeof(arguutl)) !=
			sizeof(arguutl))
		{
			printf("ps: cant read page table ");
			printf("for u of pid %d from %s\n",
			    mproc->p_pid, kmemf);
			return (0);
		}
		if (arguutl[0].pg_fod == 0 && arguutl[0].pg_pfnum)
			argaddr = ctob(arguutl[0].pg_pfnum);
		else
			argaddr = 0;
	} else {
		argaddr = 0;
	}
	klseek(kmem, (long)mproc->p_addr, 0);
	if (read(kmem, (char *)wpte, sizeof(wpte)) != sizeof(wpte)) {
		printf(
		"ps: cant read indir pte to get u for pid %d from %s\n",
		    mproc->p_pid, kmemf);
		return (0);
	}
	pcbpf = wpte[0].pg_pfnum;
	ncl = (size + NBPG*CLSIZE - 1) / (NBPG*CLSIZE);
	while (--ncl >= 0) {
		i = ncl * CLSIZE;
		lseek(mem, (long)ctob(wpte[i].pg_pfnum), 0);
		if (read(mem, user.upages[i], CLSIZE*NBPG) != CLSIZE*NBPG) {
			printf("ps: cant read page %d of u of pid %d from %s\n",
			    wpte[i].pg_pfnum, mproc->p_pid, memf);
			return(0);
		}
	}
	return (1);
}
#endif mips

int
get_ustack(cp,base,clicks)
	char *cp;
	register caddr_t base;
	register int clicks;
{

	if (clicks > mproc->p_ssize) return -1;
	if ((mproc->p_sched & SLOAD) == 0) {
		while (clicks) {
			if (getswap(base,cp) < 0) return -1;
			base += (NBPG * CLSIZE);
			cp += (NBPG * CLSIZE);
			clicks -= CLSIZE;
		}
	}
	else {
		int v, ptoff;
		register int npte;
		struct pte ptable[NPTEPG*CLSIZE], apte;
		register struct pte *pte;

		ptoff = (v = btop(base)) & (NPTEPG -1);
		npte = ctopt(clicks);
		while (npte--) {
			v = sptov(mproc,v);
			pte = sptopte(mproc,v);
			pte = &Usrptma[btokmx(pte)];
			klseek(kmem, (long) pte, 0);
			if (read(kmem, &apte, sizeof (struct pte)) !=
				sizeof (struct pte)) return -1;
			klseek(mem, ctob(apte.pg_pfnum), 0);
			if (read(mem, ptable, sizeof (ptable)) !=
				sizeof (ptable)) return -1;
			pte = &ptable[ptoff];
			for (; pte < &ptable[NPTEPG*CLSIZE]; pte += CLSIZE) {
				if (!pte->pg_v && pte->pg_pfnum == 0) {
					if (getswap(base,cp) < 0) return -1;
				}
				else {
					klseek(mem, ctob(pte->pg_pfnum), 0);
					if (read(mem, cp, CLSIZE * NBPG) !=
						CLSIZE * NBPG) return -1;
				}
				if (!(clicks -= CLSIZE)) break;
				base += NBPG * CLSIZE;
				cp += NBPG * CLSIZE;
			}
			ptoff = 0;
		}
	}
}

int
getswap(base,cp)
	caddr_t base, cp;
{
	struct dblock db;
	int v = vtosp(mproc,btop(base));

	vstodb(v, 1, mproc->p_smap, &db, 1);
	lseek(swap, (long) dtob(db.db_base), 0);
	if (read(swap, cp, CLSIZE*NBPG) != CLSIZE*NBPG)
		return -1;
	else return 0;
}

int
vector_size(vector, max_addr)	/* for getcmd */
	char **vector;		/* start of vector */
	char *max_addr;		/* location vector cannot go past */
				/* both inputs assumed to be
				   aligned for address values */
{
		/* take a 1-dim. array of strs and find the	*
		 * #elems.  Return -1 if no elems. are found	*
		 * before storage is exhausted.			*/
	int max_elems;		/* how high possible? */
	int n_elems;		/* number elements found */

	max_elems = ((int)max_addr - (int)vector) / sizeof(char *);
	if (max_elems == 0)
	  return (-1);		/* no room for elements */

	for (n_elems = 0;
		( ((char *)&vector[n_elems] < max_addr ) &&
		  !(vector[n_elems] == NULL /* std. vector term. */ ||
		    (int)vector[n_elems] == -1 /* for mail */ ) );
		n_elems++ );		/* one more elem. found */

	if (n_elems > max_elems)
		return (-1 /* no terminator */);
	else return n_elems;
}				/* end vector_size */

char *
getcmd()
{
	char *argi = 0;
	register char *cp, *lcp, *fcp;
	char c;
	int nbad, count;
	caddr_t base;
	struct dblock db;
	char *file;
	char **argv_stk, **vector_base;
	int argv_stk_cnt, envp_stk_cnt;

	if (mproc->p_stat == SZOMB || mproc->p_type&SWEXIT)
		return ("");
	if (cflg || mproc->p_type&SSYS || (dumpmag == partial_dumpmag))
		return (savestr(u.u_comm));

	/*
	 * Count of characters
	 * and offset from USRSTACK
	 */

	base = ptob(clbase(btop(u.u_execstkp)));
	count = (caddr_t) USRSTACK - (caddr_t) base;

	if ((argi = (char *) malloc(count + strlen(u.u_comm) + 3)) == NULL) {
		if ((argi = (char *) malloc(strlen(u.u_comm) + 3)) == NULL)
			return savestr("()");
		else goto retucomm;
	}

	/*
	 * Copy contents of user stack area execve allocated to a buffer
	 * this process can address.
	 */

				/* More argv and envp than in a clsize page */
	if (count  > CLSIZE * NBPG) {
		if (get_ustack(argi,base,btoc(count)) < 0) goto bad;
		fcp = cp = &argi[u.u_execstkp - base];
	}
				/* Less than or equal software page size */
	else {
		fcp = cp = &argi[(int) (u.u_execstkp) &  (CLSIZE*NBPG -1)];
		if ((mproc->p_sched & SLOAD) == 0 || argaddr == 0) {
			if (swap < 0)
				goto retucomm;
			vstodb(0, CLSIZE, mproc->p_smap, &db, 1);
			if(db.db_base == -1)
				goto retucomm;
			(void) lseek(swap, (long)dtob(db.db_base), 0);
			file = swapf;
			if (read(swap, argi, count) != count) goto bad;
		} else {
			file = memf;
			klseek(mem, (long)argaddr, 0);
			if (read(mem, argi, count) != count) goto bad;
		}
	}

	nbad = 0;
#ifdef	vax
	lcp = cp + ((caddr_t) USRSTACK - u.u_execstkp);
#endif	vax
#ifdef	mips
	lcp = cp + ((caddr_t) USRSTACK - u.u_execstkp) - EA_SIZE;
#endif	mips

	/* Count up # args. & env.vars. passed to process @exec time.
	 *
	 *   From here on, the way the execve system call builds a
	 * process' initial stack is UNDERSTOOD and ANY changes to
	 * that structure can totally confuse the following code.
	 * Throughout the stack interpretation process, some max. checks
	 * are made; this always assume the following exist:  argc, at
	 * least one argv element (the cmd name) plus argv terminator,
	 * an envp terminator, and at least 2 characters of storage for
	 * the cmd name (5 words on VAX).
	 *
	 * The main goal of this routine is to give the user the orig.
	 * args. and environment given to the process.  As the stack
	 * area is totally writable by the owning process, this may
	 * not always be possible.  Some small effort is made to
	 * handle known, common stack modifications (e.g., shell script
	 * shifting, mail chg'ng the orig argv[argc] from NULL to -1,
	 * and applications, like X, 'cutting off' parts of the orig.
	 * argv while leaving argc and the orig. allocated space as
	 * they were @ stack creation).
	 *
	 * If the simple checks fail, we attempt to start the user at
	 * or before the original stack string area.  If this is after
	 * the real start, things like the command name are simply lost
	 * (cf. 'ps -c' to always get cmd name).  If output is from
	 * before the string area, the user can still get the data via
	 * 'Unpleasant, Eerie, and Gross' command line:
	 *	ps -wweeeeeeee...
	 * Much fun could be had developing more heuristics to track
	 * and get the 'orig' values by interpreting the values of all
	 * stack storage (argc, argv[*], envp[*], etc.).  This has not
	 * been done for lack of real effectiveness vs. cost.
	*/

	argv_stk = (char **)(cp + sizeof(argv_stk_cnt)); /* argv base */

	argv_stk_cnt = (int) *cp;
	if ( 0 >		/* orig argc reasonable? */
		(int)(lcp - fcp)	/* whole stack size minus...*/
		- sizeof(argv_stk_cnt)		/* argc int */
		- ( (argv_stk_cnt+1) * sizeof(char *) ) /* argv ptrs */
		- sizeof(char *)		/* min. NULL envp ptr */
		- ( sizeof("a") * argv_stk_cnt )	/* min. argv strings */
	   ) {				/* argc too large...*/
		argv_stk_cnt = -1;
		vector_base = argv_stk;
	}
	else if ( argv_stk[argv_stk_cnt] == NULL ||
		  (int)argv_stk[argv_stk_cnt] == -1 ) {
		vector_base = &argv_stk[argv_stk_cnt+1]; /* chk envp */
	}
	else {			/* doesn't locate vector end... */
		argv_stk_cnt = -1;
		vector_base = argv_stk;
	}

	envp_stk_cnt = -1;	/* working var, will hold # envp
				 * elems at while loop finish */
	count = 0;		/* #times srh'd for argv end (to
				 *   prevent an infinite loop) */
	while (envp_stk_cnt < 0 || argv_stk_cnt < 1) {
				/* loop until have some cnt of both
				 * the argv & envp vector lengths... */
	  envp_stk_cnt = vector_size (vector_base, lcp - 4);

	  if (argv_stk_cnt > 0)	{	/* had argv length */
	    if (envp_stk_cnt < 0) {	/* envp length not found */
	      if (count == 0) {		/* had trusted orig argc, but
					 * error if envp end not found */
		argv_stk_cnt = -1;	/* invalidate argv cnt, */
		vector_base = argv_stk; /* srh for argv end */
		count = 1;		/* & no multiple argv srh's */
	      }
	      else {			/* tried to find both vector
					 * lengths and failed. */
		argv_stk_cnt = 1;	/* must always have cmd. nm. */
		envp_stk_cnt = 0;	/* assume no environment */
	      }
	    /* else done */
	    }
	  }
	  else				/* didn't have argv len */
	    if (envp_stk_cnt > 0) {	/* found 'orig' argv */
		argv_stk_cnt = envp_stk_cnt;
		count = 1;		/* don't rescan for argv len*/
		envp_stk_cnt = -1;	/* now try to get envp len */
		vector_base = &argv_stk[argv_stk_cnt+1];
	    }
	    else {			/* can't find any vector end */
		argv_stk_cnt = 1;	/* must always have cmd. nm. */
		envp_stk_cnt = 0;	/* assume no environment */
	    }
	}	/* end while loop */

	/* get addr(stk strings) given argv & envp len's from above */
	cp = (char *)(&argv_stk[argv_stk_cnt + envp_stk_cnt + 2]);
					/* +2 for NULL terminators */

	/* verify cp not too large: all but cmd may be null strs. */
	if (lcp <= cp +	(sizeof(char) *
			  (argv_stk_cnt + envp_stk_cnt + 1)) )
				/* prob: force min. argv & envp len's */
		cp = fcp + ( sizeof(argv_stk_cnt) + (sizeof(int *)*3) );

	/* ready for walking str area (or what is thought to be it) */
 	fcp = cp;		/* fcp locates 1st str now */
	count = 0;		/* use count to catch environ start */
	for (cp; cp < lcp; cp++) {
		if (*cp == 0) {
			count++;	/* arg/environ string end */
			*cp = ' ';
			if (count == argv_stk_cnt && eflg == 0)
				break;	/* user doesn't want environ */
		}
		else {			/* copy char to output */
		  c = *cp & 0177;	/* only handle 7-bit char set */
		  if ( !isprint(c) ) {
		     if (++nbad >= 5*(eflg+1)) {
			*cp++ = ' ';
			break;	/* too many unprintable chars */
		     }
		  *cp = '?';	/* try to show user problem char */
		  }
		}
	}
	*cp = 0;			/* terminate output string */
	while (*--cp == ' ')		/* trim trailing blanks */
		*cp = 0;
	if (fcp[0] == '-' || fcp[0] == '?' || !isprint(fcp[0]) ) {
		(void) strcat(cp, " (");
		(void) strncat(cp, u.u_comm,
				sizeof(u.u_comm));
		(void) strcat(cp, ")");
	}
	cp = savestr(fcp);
	goto done;

bad:
	fprintf(stderr, "ps: error locating command name for pid %d from %s\n",
	    mproc->p_pid, file);
retucomm:
	(void) strcpy(argi, " (");
	(void) strncat(argi, u.u_comm, sizeof (u.u_comm));
	(void) strcat(argi, ")");
	cp = savestr(argi);
done:
	free(argi);
	return cp;
}

char	*lhdr =
"      F UID   PID  PPID CP PRI NI ADDR  SZ  RSS WCHAN STAT  TT  TIME";
lpr(sp)
	struct savcom *sp;
{
	register struct asav *ap = sp->ap;
	register struct lsav *lp = sp->s_un.lp;

	printf("%7x%4d%6u%6u%3d%4d%3d%5x%4d%5d",
	    ap->a_flag, ap->a_uid,
	    ap->a_pid, lp->l_ppid, lp->l_cpu&0377, ap->a_pri-PZERO,
	    ap->a_nice-NZERO, lp->l_addr, pgtok(ap->a_size), pgtok(ap->a_rss));
	printf(lp->l_wchan ? " %5x" : "      ", (int)lp->l_wchan&0xfffff);
	printf(" %5.5s ", state(ap));
	ptty(ap->a_tty);
	ptime(ap);
}

ptty(tp)
	char *tp;
{

	printf("%-2.2s", tp);
}

ptime(ap)
	struct asav *ap;
{

	printf("%3ld:%02ld", ap->a_cpu / 60, ap->a_cpu % 60);
}

char	*uhdr =
"USER       PID %CPU %MEM   SZ  RSS TT STAT   TIME";
upr(sp)
	struct savcom *sp;
{
	register struct asav *ap = sp->ap;
	int vmsize, rmsize;

	vmsize = pgtok((ap->a_size + ap->a_tsiz));
	rmsize = pgtok(ap->a_rss);
	if (ap->a_xccount)
		rmsize += pgtok(ap->a_txtrss/ap->a_xccount);
	printf("%-8.8s %5d%5.1f%5.1f%5d%5d",
	    getname(ap->a_uid), ap->a_pid, sp->s_un.u_pctcpu, pmem(ap),
	    vmsize, rmsize);
	putchar(' ');
	ptty(ap->a_tty);
	printf(" %5.5s", state(ap));
	ptime(ap);
}

char *vhdr =
" SIZE  PID TT STAT   TIME SL RE PAGEIN SIZE  RSS  LIM TSIZ TRS %CPU %MEM"+5;
vpr(sp)
	struct savcom *sp;
{
	register struct vsav *vp = sp->s_un.vp;
	register struct asav *ap = sp->ap;

	printf("%5u ", ap->a_pid);
	ptty(ap->a_tty);
	printf(" %5.5s", state(ap));
	ptime(ap);
	printf("%3d%3d%7d%5d%5d",
	   ap->a_slptime > 99 ? 99 : ap-> a_slptime,
	   ap->a_time > 99 ? 99 : ap->a_time, vp->v_majflt,
	   pgtok(ap->a_size), pgtok(ap->a_rss));
	if (ap->a_maxrss == (RLIM_INFINITY/NBPG))
		printf("   xx");
	else
		printf("%5d", pgtok(ap->a_maxrss));
	printf("%5d%4d%5.1f%5.1f",
	   pgtok(ap->a_tsiz), pgtok(ap->a_txtrss), vp->v_pctcpu, pmem(ap));
}

char	*shdr =
"SSIZ   PID TT STAT   TIME";
spr(sp)
	struct savcom *sp;
{
	register struct asav *ap = sp->ap;

	if (sflg)
		printf("%4d ", sp->s_un.s_ssiz);
	printf("%5u", ap->a_pid);
	putchar(' ');
	ptty(ap->a_tty);
	printf(" %5.5s", state(ap));
	ptime(ap);
}

char *
state(ap)
	register struct asav *ap;
{
	char stat, load, nice, anom;
	static char res[6];

	switch (ap->a_stat) {

	case SSTOP:
		stat = 'T';
		break;

	case SSLEEP:
		if (ap->a_pri >= PZERO)
			if (ap->a_slptime >= MAXSLP)
				stat = 'I';
			else
				stat = 'S';
		else if (ap->a_flag & SPAGE)
			stat = 'P';
		else
			stat = 'D';
		break;

	case SWAIT:
	case SRUN:
	case SIDL:
		stat = 'R';
		break;

	case SZOMB:
		stat = 'Z';
		break;

	default:
		stat = '?';
	}
	load = ap->a_flag & SLOAD ? (ap->a_rss>ap->a_maxrss ? '>' : ' ') : 'W';
	if (ap->a_nice < NZERO)
		nice = '<';
	else if (ap->a_nice > NZERO)
		nice = 'N';
	else
		nice = ' ';
	anom = (ap->a_flag&SUANOM) ? 'A' : ((ap->a_flag&SSEQL) ? 'S' : ' ');
	res[0] = stat; res[1] = load; res[2] = nice; res[3] = anom;
	res[4] = (ap->a_vproc) ? 'V' : ' ';
	return (res);
}

/*
 * Given a base/size pair in virtual swap area,
 * return a physical base/size pair which is the
 * (largest) initial, physically contiguous block.
 */
vstodb(vsbase, vssize, dmp, dbp, rev)
	register int vsbase, vssize;
	struct dmap *dmp;
	register struct dblock *dbp;
{
	struct dmap ldmp;
	struct dmap *dmptr;
	char buf[4096];
	/* since we may reading more than  dmap structure
	 * give a filler space so that memory wouldn't be corrupted
	 */

	register int index;
	register int blk = swapfrag;

	vsbase = ctod(vsbase);
	vssize = ctod(vssize);
	dmptr = (struct dmap *)&buf[0];

	lseek(kmem, (long)dmp, 0);
	read(kmem, (char *)&ldmp, sizeof(struct dmap));
	index = ldmp.dm_last;
	/*
	 * read again with the proper count dm_map;
	 */
	lseek(kmem, (long)dmp, 0);
	read(kmem,(char *)dmptr, (sizeof(struct dmap) + (index-1)*sizeof(int)));

	if (vsbase < 0 || vssize < 0 || (vsbase + vssize > (ldmp.dm_last *
					blk)))
		panic("vstodb");
	index = vsbase/blk;
	vsbase %= blk;
	if(ldmp.dm_cnt == 0 || dmptr->dm_map[index] == 0){
		dbp->db_base = -1;
		return;
	}
	if (dmptr->dm_map[index] + blk > nswap)
		panic("vstodb exceeding nswap");
	dbp->db_size = min(vssize, blk - vsbase);
	dbp->db_base = dmptr->dm_map[index]
			+ (rev ? blk - (vsbase + dbp->db_size) : vsbase);
}

/*ARGSUSED*/
panic(cp)
	char *cp;
{

#ifdef DEBUG
	printf("%s\n", cp);
#endif
}

min(a, b)
{

	return (a < b ? a : b);
}

pscomp(s1, s2)
	struct savcom *s1, *s2;
{
	register int i;

	if (uflg)
		return (s2->s_un.u_pctcpu > s1->s_un.u_pctcpu ? 1 : -1);
	if (vflg)
		return (vsize(s2) - vsize(s1));
	i = s1->ap->a_ttyd - s2->ap->a_ttyd;
	if (i == 0)
		i = s1->ap->a_pid - s2->ap->a_pid;
	return (i);
}

vsize(sp)
	struct savcom *sp;
{
	register struct asav *ap = sp->ap;
	register struct vsav *vp = sp->s_un.vp;

	if (ap->a_flag & SLOAD)
		return (ap->a_rss +
		    ap->a_txtrss / (ap->a_xccount ? ap->a_xccount : 1));
	return (vp->v_swrss + (ap->a_xccount ? 0 : vp->v_txtswrss));
}

#define	NMAX	8	/* sizeof loginname (should be sizeof (utmp.ut_name)) */
#define NUID	2048	/* must not be a multiple of 5 */

struct nametable {
	char	nt_name[NMAX+1];
	int	nt_uid;
} nametable[NUID];

struct nametable *
findslot(uid)
unsigned short	uid;
{
	register struct nametable	*n, *start;

	/*
	 * find the uid or an empty slot.
	 * return NULL if neither found.
	 */

	n = start = nametable + (uid % (NUID - 20));
	while (n->nt_name[0] && n->nt_uid != uid) {
		if ((n += 5) >= &nametable[NUID])
			n -= NUID;
		if (n == start)
			return((struct nametable *)NULL);
	}
	return(n);
}

char *
getname(uid)
{
	register struct passwd		*pw;
	register struct nametable	*n;

	/*
	 * find uid in hashed table; add it if not found.
	 * return pointer to name.
	 */

	if ((n = findslot(uid)) == NULL)
		return((char *)NULL);

	if (n->nt_name[0])	/* occupied? */
		return(n->nt_name);

	if ((pw = getpwuid(uid)) == NULL)
		return ((char *)NULL);
	else {
		strncpy(n->nt_name, pw->pw_name, NMAX);
		return(n->nt_name);
	}
}

char *
savestr(cp)
	char *cp;
{
	register int len;
	register char *dp;

	len = strlen(cp) + 1;   /* don't forget null */
	if((dp = (char *)calloc(1, (unsigned)len)) == (char *)NULL) {
		perror("ps: savestr: calloc");
		exit(1);
	}
	(void) strcpy(dp, cp);
	return (dp);
}

#ifdef mips
/*
 * "addr"  is a kern virt addr and does not correspond
 * To a phys addr after zipping out the high bit..
 * since it was valloc'd in the kernel.
 *
 * We return the phys addr by simulating kernel vm (/dev/kmem)
 * when we are reading a crash dump.
 */
#include <machine/cpu.h>
off_t
mkphys(addr)
	unsigned addr;
{
	register off_t o;
	unsigned addr2=addr;

	if (IS_KSEG0(addr)) {
		return(K0_TO_PHYS(addr));
	}
	if (IS_KSEG1(addr)) {
		return(K1_TO_PHYS(addr));
	}
	o = addr & PGOFSET;
	addr = 4 * btop(addr2 - K2BASE);
	addr = getsys((unsigned)nl[SSYSMAP].n_value + addr);
	addr = (addr & PG_PFNUM) << (PGSHIFT - PTE_PFNSHIFT) | o;
	return(addr);
}

getsys(loc)
	register unsigned loc;
{
	register unsigned seekloc = 0;
	int word = 0;

	if (IS_KSEG0(loc)) {
		seekloc = K0_TO_PHYS(loc);
	}
	if (IS_KSEG1(loc)) {
		seekloc = K1_TO_PHYS(loc);
	}
	lseek(kmem, seekloc, 0);
	read(kmem, &word, sizeof (word));
	return (word);
}
#endif mips
