#ifndef lint
static	char	*sccsid = "@(#)find.c	4.2	(ULTRIX)	4/11/91";
#endif lint

/************************************************************************
 *									*
 *   Copyright (c) Digital Equipment Corporation, 1985, 1987, 1988,	*
 *   			1989-1990, 1991					*
 *									*
 *   All Rights Reserved.  Unpublished rights  reserved  under  the	*
 *   copyright laws of the United States.				*
 *									*
 *   The software contained on this media  is  proprietary  to  and	*
 *   embodies  the  confidential  technology  of  Digital Equipment	*
 *   Corporation.  Possession, use, duplication or dissemination of	*
 *   the  software and media is authorized only pursuant to a valid	*
 *   written license from Digital Equipment Corporation.		*
 *									*
 *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  by	*
 *   the U.S. Government is subject to restrictions as set forth in	*
 *   Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  or  in  FAR	*
 *   52.227-19, as applicable.						*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 ************************************************************************/
/**/

/* static char *sccsid = "@(#)find.c     4.7 (Berkeley) 8/2/82"; */
/*	find	COMPILE:	cc -o find -s -O find.c 	*/

/*
 * Modification History
 *
 * 004 - 01 Apr 91 - AGK and GWS
 *	changed cpio header structures used for -cpio to be same as V4.2
 *	  cpio(1), to handle inodes > 64k
 *	changed (pwd < 0) compare to (pwd < (FILE *) 0) to avoid MIPS C
 *	  compiler warnings
 *
 * 003 - Gary A. Gaudet, Wed May 24 16:57:19 EDT 1989
 *	POSIX 1003.2 compliance: added -ctime and -depth
 *
 * 002 - 15 Nov 85 - T.N. Cherng
 *	The original pathname[] is allocated for 200 characters
 *	instead of MAXPATHLEN+1.  A core dump occurs if a file path
 *	is over 200 characters.
 *
 * 001 - 15 Mar 85 -- funding
 *	Added named pipe support (re. System V named pipes)
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <sys/wait.h>
#include <signal.h>

#define A_DAY	86400L /* a day full of seconds */
#define EQ(x, y)	(strcmp(x, y)==0)

int	Randlast;
char	Pathname[MAXPATHLEN +1];

struct anode {
	int (*F)();
	struct anode *L, *R;
} Node[100];

int Nn;  /* number of nodes */
char	*Fname;
long	Now;
int	Argc, Ai, Pi;
char	**Argv;

/* cpio stuff */
int	Cpio;
short	*Buf, *Dbuf, *Wp;
int	Bufsize = 5120;
int	Wct = 2560;

/* mount stuff */
int	do_mount = 1;	/* go down mount points if true */
int	mount_dev = -1;/* file must reside here or we've crossed a mount point*/

long	Newer;

struct stat Statb;

/*
 * 003 GAG
 * preorder list is default; postorder when -depth is used
 */
int Postorder = 0;

struct	anode	*exp(), *e1(), *e2(), *e3(), *mk();
char	Home[MAXPATHLEN + 1];
char	*rindex(), *sbrk(), *strcpy(), *nxtarg();
long	Blocks, time();
void	exit(), perror();
FILE	*popen();
int	or(), and(), not();
int exeq(), ok(), glob(),  mtime(), atime(),
	ctime(), depth(), /* 003 GAG */
	user(), group(), size(), perm(), links(), print(),
	type(), ino(), cpio(), newer(), ckmount();

main(argc, argv)
char *argv[];
{
	register struct anode *exlist;
	register char *cp, *sp = 0;
	register int paths;
	register FILE *pwd;

	(void) time(&Now);
	pwd = popen("/bin/pwd", "r");
	if (pwd < (FILE *) 0) {		/* 004 */
		perror("/bin/pwd");
		exit(1);
	}
	if (fgets(Home, sizeof Home, pwd) == NULL) {
		perror("fgets");
		exit(1);
	}
	(void) pclose(pwd);
	Home[strlen(Home) - 1] = '\0';
	Argc = argc; 
	Argv = argv;
	if(argc<3) {
usage:		
		(void) fprintf(stderr,"Usage: find path-list predicate-list\n");
		exit(1);
	}
	for(Ai = paths = 1; Ai < (argc-1); ++Ai, ++paths)
		if(*Argv[Ai] == '-' || EQ(Argv[Ai], "(") || EQ(Argv[Ai], "!"))
			break;
	if(paths == 1) /* no path-list */
		goto usage;
	if(!(exlist = exp())) { /* parse and compile the arguments */
		(void) fprintf(stderr, "find: parsing error\n");
		exit(1);
	}
	if(Ai<argc) {
		(void) fprintf(stderr, "find: missing conjunction\n");
		exit(1);
	}
	for(Pi = 1; Pi < paths; ++Pi) {
		mount_dev = -1;
		sp = 0;
		if (chdir(Home) == -1) {
			perror("chdir");
			exit(1);
		}
		(void) strcpy(Pathname, Argv[Pi]);
		if(cp = rindex(Pathname, '/')) {
			sp = cp + 1;
			*cp = '\0';
			if(chdir(*Pathname ? Pathname : "/") == -1) {
				(void) fprintf(stderr,
					"find: bad starting directory\n");
				exit(2);
			}
			*cp = '/';
		}
		Fname = sp ? sp : Pathname;
		(void) descend(Pathname, Fname, exlist);
			/* to find files that match */
	}
	if(Cpio) {
		(void) strcpy(Pathname, "TRAILER!!!");
		Statb.st_size = 0;
		cpio();
		(void) printf("%D blocks\n", Blocks*10);
	}
	if (chdir(Home) == -1) {
		(void) fprintf(stderr,"Can't cd to %s\n",Home);
		exit(1);
	}
	exit(0);
}

/* compile time functions:  priority is  exp()<e1()<e2()<e3()  */

struct anode *
exp()
{ /* parse ALTERNATION (-o)  */
	register struct anode *p1;

	p1 = e1() /* get left operand */ ;
	if(EQ(nxtarg(), "-o")) {
		Randlast--;
		return(mk(or, p1, exp()));
	}
	else if(Ai <= Argc) --Ai;
	return(p1);
}

struct anode *
e1()
{ /* parse CONCATENATION (formerly -a) */
	register struct anode * p1;
	register char *a;

	p1 = e2();
	a = nxtarg();
	if(EQ(a, "-a")) {
And:
		Randlast--;
		return(mk(and, p1, e1()));
	} 
	else if(EQ(a, "(") || EQ(a, "!") || (*a=='-' && !EQ(a, "-o"))) {
		--Ai;
		goto And;
	} 
	else if(Ai <= Argc) --Ai;
	return(p1);
}

struct anode *
e2()
{ /* parse NOT (!) */
	register struct anode *ptr;

	if(Randlast) {
		(void) fprintf(stderr, "find: operand follows operand\n");
		exit(1);
	}
	Randlast++;
	if(EQ(nxtarg(), "!")) {
		ptr = e3();
		if (ptr == NULL) {
			(void) fprintf(stderr,"find: bad (!) operand\n");
			exit(1);
		}
		return(mk(not, ptr, (struct anode *)0));
	}
	else if(Ai <= Argc) --Ai;
	ptr = e3();
	if (ptr == NULL) {
		(void) fprintf(stderr,"find: bad (!) operand\n");
		exit(1);
	}
	return(ptr);
}

struct anode *
e3()
{ /* parse parens and predicates */
	register struct anode *p1;
	register struct passwd *pwd;
	register struct group *grp;
	register char *a;
	register char *b;
	register int i;
	char s;

	a = nxtarg();
	if(EQ(a, "(")) {
		Randlast--;
		p1 = exp();
		a = nxtarg();
		if(!EQ(a, ")")) goto err;
		return(p1);
	}
	else if(EQ(a, "-print")) {
		return(mk(print, (struct anode *)0, (struct anode *)0));
	}
	else if(EQ(a, "-mount")) {
		do_mount = 0;
		return(mk(ckmount, (struct anode *)0, (struct anode *)0));
	}
	else if(EQ(a, "-depth")) {	/* 003 GAG */
		Postorder = 1;
		return(mk(depth, (struct anode *)0, (struct anode *)0));
	}
	b = nxtarg();
	s = *b;
	if(s=='+') b++;
	if(EQ(a, "-name"))
		return(mk(glob, (struct anode *)b, (struct anode *)0));
	else if(EQ(a, "-mtime"))
		return(mk(mtime, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-atime"))
		return(mk(atime, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-ctime")) /* 003 GAG */
		return(mk(ctime, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-user")) {
		if((pwd=getpwnam(b)) == NULL) {
			if(gmatch(b, "[0-9]*"))
				return mk(user, (struct anode *)atoi(b),
						(struct anode *)s);
			(void) fprintf(stderr,"find: cannot find -user name\n");
			exit(1);
		}
		return(mk(user, (struct anode *)pwd->pw_uid,(struct anode *)s));
	}
	else if(EQ(a, "-inum"))
		return(mk(ino, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-group")) {
		if((grp=getgrnam(b)) == NULL) {
			if(gmatch(b, "[0-9]*"))
				return(mk(group,(struct anode *)atoi(b),
						(struct anode *)s));
			(void) fprintf(stderr,
				"find: cannot find -group name\n");
			exit(1);
		}
		return(mk(group,(struct anode *)grp->gr_gid,(struct anode *)s));
	} 
	else if(EQ(a, "-size"))
		return(mk(size, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-links"))
		return(mk(links, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-perm")) {
		for(i=0; *b ; ++b) {
			if(*b=='-') continue;
			i <<= 3;
			i += (*b - '0');
		}
		return(mk(perm, (struct anode *)i, (struct anode *)s));
	}
	else if(EQ(a, "-type")) {
		i =	s=='d' ? S_IFDIR  :
			s=='b' ? S_IFBLK  :
			s=='c' ? S_IFCHR  :
			s=='f' ? S_IFREG  :
			s=='l' ? S_IFLNK  :
			s=='s' ? S_IFSOCK :
			s=='p' ? S_IFPORT :		/* 1 */
			0;
		return(mk(type, (struct anode *)i, (struct anode *)0));
	}
	else if (EQ(a, "-exec")) {
		i = Ai - 1;
		while(!EQ(nxtarg(), ";"));
		return(mk(exeq, (struct anode *)i, (struct anode *)0));
	}
	else if (EQ(a, "-ok")) {
		i = Ai - 1;
		while(!EQ(nxtarg(), ";"));
		return(mk(ok, (struct anode *)i, (struct anode *)0));
	}
	else if(EQ(a, "-cpio")) {
		if((Cpio = creat(b, 0666)) < 0) {
			(void) fprintf(stderr,
				"find: cannot create < %s >\n", s);
			exit(1);
		}
		Buf = (short *)sbrk(512);
		Wp = Dbuf = (short *)sbrk(5120);
		return(mk(cpio, (struct anode *)0, (struct anode *)0));
	}
	else if(EQ(a, "-newer")) {
		if(stat(b, &Statb) < 0) {
			(void) fprintf(stderr,
				"find: cannot access < %s >\n", b);
			exit(1);
		}
		Newer = Statb.st_mtime;
		return(mk(newer, (struct anode *)0, (struct anode *)0));
	}
err:	
	(void) fprintf(stderr, "find: bad option < %s >\n", a);
	return(NULL);
}

struct anode *
mk(f, l, r)
int (*f)();
struct anode *l, *r;
{
	Node[Nn].F = f;
	Node[Nn].L = l;
	Node[Nn].R = r;
	return(&(Node[Nn++]));
}

char *
nxtarg()
{ /* get next arg from command line */
	static strikes = 0;

	if(strikes==3) {
		(void) fprintf(stderr, "find: incomplete statement\n");
		exit(1);
	}
	if(Ai>=Argc) {
		strikes++;
		Ai = Argc + 1;
		return("");
	}
	return(Argv[Ai++]);
}

/* execution time functions */
and(p)
register struct anode *p;
{
	return(((*p->L->F)(p->L)) && ((*p->R->F)(p->R)) ? 1 : 0);
}

or(p)
register struct anode *p;
{
	return(((*p->L->F)(p->L)) || ((*p->R->F)(p->R)) ? 1 : 0);
}

not(p)
register struct anode *p;
{
	return( !((*p->L->F)(p->L)));
}

glob(p)
register struct { 
	int f; 
	char *pat; 
} *p; 
{
	return(gmatch(Fname, p->pat));
}

print()
{
	(void) puts(Pathname);
	return(1);
}

ckmount()
{
	if (mount_dev == -1) mount_dev = Statb.st_dev; /* first time through */
	if (mount_dev == Statb.st_dev) return(1);
	return(0);
}

mtime(p)
register struct { 
	int f, t, s; 
} *p; 
{
	return(scomp((int)((Now - Statb.st_mtime) / A_DAY), p->t, p->s));
}

atime(p)
register struct { 
	int f, t, s; 
} *p; 
{
	return(scomp((int)((Now - Statb.st_atime) / A_DAY), p->t, p->s));
}

/*
 * 003 GAG
 * ctime - status information change
 */
int
ctime(p)
register struct { 
	int f, t, s; 
} *p; 
{
	return(scomp((int)((Now - Statb.st_ctime) / A_DAY), p->t, p->s));
}

user(p)
register struct { 
	int f, u, s; 
} *p; 
{
	return(scomp(Statb.st_uid, p->u, p->s));
}

ino(p)
register struct { 
	int f, u, s; 
} *p;
{
	return(scomp((int)Statb.st_ino, p->u, p->s));
}

group(p)
register struct { 
	int f, u; 
} *p; 
{
	return(p->u == Statb.st_gid);
}

links(p)
register struct { 
	int f, link, s; 
} *p; 
{
	return(scomp(Statb.st_nlink, p->link, p->s));
}

size(p)
register struct { 
	int f, sz, s; 
} *p; 
{
	return(scomp((int)((Statb.st_size+511)>>9), p->sz, p->s));
}

perm(p)
register struct { 
	int f, per, s; 
} *p; 
{
	register int i;
	i = (p->s=='-') ? p->per : 07777; /* '-' means only arg bits */
	return((Statb.st_mode & i & 07777) == p->per);
}

type(p)
register struct { 
	int f, per, s; 
} *p;
{
	return((Statb.st_mode&S_IFMT)==p->per);
}

exeq(p)
register struct { 
	int f, com; 
} *p;
{
	(void) fflush(stdout); /* to flush possible `-print' */
	return(doex(p->com));
}

ok(p)
register struct { 
	int f, com; 
} *p;
{
	register int yes;
	register int c;  
	yes = 0;
	(void) fflush(stdout); /* to flush possible `-print' */
	(void) fprintf(stderr, "< %s ... %s > ?   ", Argv[p->com], Pathname);
	if((c=getchar())=='y') yes = 1;
	while(c!='\n') c = getchar();
	if(yes) return(doex(p->com));
	return(0);
}

#define MKSHORT(v, lv)	{						\
			U.l=1L;						\
			if(U.c[0]) U.l=lv, v[0]=U.s[1], v[1]=U.s[0]; 	\
			else U.l=lv, v[0]=U.s[0], v[1]=U.s[1];		\
			}

union {
	long l; 
	short s[2]; 
	char c[4]; 
} U;

long
mklong(v)
short v[];
{
	U.l = 1;
	if(U.c[0] /* VAX */)
		U.s[0] = v[1], U.s[1] = v[0];
	else
		U.s[0] = v[0], U.s[1] = v[1];
	return(U.l);
}

cpio()
{
#define MAGIC 070707
	struct header {
		short	h_magic,
			h_dev;
		ino_t	h_ino;			/* 004 */
		ushort	h_mode,			/* 004 */
			h_uid,
			h_gid;
		short	h_nlink,		/* 004 */
			h_rdev;
		short	h_mtime[2];
		short	h_namesize;
		short	h_filesize[2];
		char	h_name[256];
	} hdr;
	register int ifile, ct;
	register int i;
	static long fsz;

	hdr.h_magic = MAGIC;
	(void) strcpy(hdr.h_name,
		!strncmp(Pathname, "./", 2) ? Pathname+2 : Pathname);
	hdr.h_namesize = strlen(hdr.h_name) + 1;
	hdr.h_uid = Statb.st_uid;
	hdr.h_gid = Statb.st_gid;
	hdr.h_dev = Statb.st_dev;
	hdr.h_ino = Statb.st_ino;
	hdr.h_mode = Statb.st_mode;
	MKSHORT(hdr.h_mtime, Statb.st_mtime);
	hdr.h_nlink = Statb.st_nlink;
	fsz = hdr.h_mode & S_IFREG ? Statb.st_size : 0L;
	MKSHORT(hdr.h_filesize, fsz);
	hdr.h_rdev = Statb.st_rdev;
	if(EQ(hdr.h_name, "TRAILER!!!")) {
		bwrite((short *)&hdr, (sizeof hdr-256)+hdr.h_namesize);
		for(i = 0; i < 10; ++i)
			bwrite(Buf, 512);
		return;
	}
	if(!mklong(hdr.h_filesize))
		return;
	if((ifile = open(Fname, 0)) < 0) {
cerror:
		(void) fprintf(stderr,"find: cannot copy < %s >\n", hdr.h_name);
		return;
	}
	bwrite((short *)&hdr, (sizeof hdr-256)+hdr.h_namesize);
	for(fsz = mklong(hdr.h_filesize); fsz > 0; fsz -= 512) {
		ct = fsz>512 ? 512 : fsz;
		if(read(ifile, (char *)Buf, ct) < 0)
			goto cerror;
		bwrite(Buf, ct);
	}
	(void) close(ifile);
	return;
}

newer()
{
	return(Statb.st_mtime > Newer);
}

/* support functions */
scomp(a, b, s) /* funny signed compare */
register int a, b, s;
{
	if(s == '+')
		return(a > b);
	if(s == '-')
		return(a < (b * -1));
	return(a == b);
}

doex(com)
register int com;
{
	register int np;
	register char *na;
	register void (*oldi)(), (*oldq)();
	static char *nargv[50];
	int ccode;

	ccode = np = 0;
	while (na=Argv[com++]) {
		if(strcmp(na, ";")==0) break;
		if(strcmp(na, "{}")==0) nargv[np++] = Pathname;
		else nargv[np++] = na;
	}
	nargv[np] = 0;
	if (np==0) return(9);
	if(fork()) /*parent*/ {
		oldi = signal(SIGINT, SIG_IGN);
		oldq = signal(SIGQUIT, SIG_IGN);
		(void) wait((union wait *)&ccode);
		(void) signal(SIGINT, oldi);
		(void) signal(SIGQUIT, oldq);
	} 
	else { /*child*/
		if (chdir(Home) == -1) {
			perror("chdir");
			exit(1);
		}
		(void) execvp(nargv[0], nargv);
		perror("execvp");
		exit(1);
	}
	return(ccode ? 0 : 1);
}

descend(name, fname, exlist)
register struct anode *exlist;
register char *name, *fname;
{
	register DIR *dir;
	register struct direct *dp;
	register char *c1;
	char *endofname;
	char dirname1[MAXPATHLEN +1];
	char dirname2[MAXPATHLEN +1];
	int rv = 0;

	strcpy(dirname1, name);
	strcpy(dirname2, fname);
	if (lstat(fname, &Statb)<0) {
		(void) fprintf(stderr, "find: bad status < %s >\n", name);
		return(0);
	}
	if (!Postorder)		/* 003 GAG */
		(*exlist->F)(exlist);
	if((Statb.st_mode&S_IFMT)!=S_IFDIR) {
		if (Postorder)		/* 003 GAG */
			(*exlist->F)(exlist);
		return(1);
	}
	if((!do_mount) && (mount_dev != Statb.st_dev))
			return(1);

	for (c1 = name; *c1; ++c1);
	if (*(c1-1) == '/')
		--c1;
	endofname = c1;

	if (chdir(fname) == -1)
		return(0);
	if ((dir = opendir(".")) == NULL) {
		(void) fprintf(stderr, "find: cannot open < %s >\n", name);
		rv = 0;
		goto ret;
	}
	for (dp = readdir(dir); dp != NULL; dp = readdir(dir)) {
		if ((dp->d_name[0]=='.' && dp->d_name[1]=='\0') ||
		    (dp->d_name[0]=='.' && dp->d_name[1]=='.'   &&
		     dp->d_name[2]=='\0'))
			continue;
		c1 = endofname;
		*c1++ = '/';
		(void) strcpy(c1, dp->d_name);
		Fname = endofname+1;
		if(!descend(name, Fname, exlist)) {
			*endofname = '\0';
			if (chdir(Home) == -1) {
				perror("chdir");
				exit(1);
			}
			if(chdir(Pathname) == -1) {
				(void) fprintf(stderr,
					"find: bad directory tree\n");
				exit(1);
			}
		}
	}
	rv = 1;
ret:
	if (Postorder) {	/* 003 GAG */
		(void) strcpy(name, dirname1);
		(void) strcpy(fname, dirname2);
		if(stat(".", &Statb) < 0) {
			(void) fprintf(stderr,"cannot stat %s\n", fname);
		}
		(*exlist->F)(exlist);
	}
	if(dir)
		closedir(dir);
	if(chdir("..") == -1) {
		*endofname = '\0';
		(void) fprintf(stderr, "find: bad directory <%s>\n", name);
		rv = 1;
	}
	return(rv);
}

gmatch(s, p) /* string match as in glob */
register char *s, *p;
{
	if (*s=='.' && *p!='.') return(0);
	return(amatch(s, p));
}

amatch(s, p)
register char *s, *p;
{
	register int cc, scc, k, lc;
	int c;

	scc = *s;
	lc = 077777;
	c = *p;
	switch (*p) {

	case '[':
		k = 0;
		while (cc = *++p) {
			switch (cc) {

			case ']':
				if (k)
					return(amatch(++s, ++p));
				else
					return(0);

			case '-':
				k |= lc <= scc & scc <= (cc=p[1]);
			}
			if (scc==(lc=cc)) k++;
		}
		return(0);

	case '?':
caseq:
		if(scc) return(amatch(++s, ++p));
		return(0);
	case '*':
		return(umatch(s, ++p));
	case 0:
		return(!scc);
	}
	if (c==scc) goto caseq;
	return(0);
}

umatch(s, p)
register char *s, *p;
{
	if(*p==0) return(1);
	while(*s)
		if (amatch(s++, p)) return(1);
	return(0);
}

bwrite(rp, c)
register short *rp;
register int c;
{
	register short *wp = Wp;

	c = (c+1) >> 1;
	while(c--) {
		if(!Wct) {
again:
			if(write(Cpio, (char *)Dbuf, Bufsize)<0) {
				Cpio = chgreel(1, Cpio);
				goto again;
			}
			Wct = Bufsize >> 1;
			wp = Dbuf;
			++Blocks;
		}
		*wp++ = *rp++;
		--Wct;
	}
	Wp = wp;
}

chgreel(x, fl)
{
	register int f;
	register FILE *devtty;
	char str[22];
	struct stat statb;
	extern errno;

	(void) fprintf(stderr, "find: errno: %d, ", errno);
	(void) fprintf(stderr,
			"find: can't %s\n", x? "write output": "read input");
	if (fstat(fl, &statb) == -1) {
		perror("fstat");
		exit(1);
	}
	if((statb.st_mode&S_IFMT) != S_IFCHR) {
		(void) fprintf(stderr,"find: not a char dev\n");
		exit(1);
	}
again:
	(void) fprintf(stderr,
		"If you want to go on, type device/file name when ready\n");
	devtty = fopen("/dev/tty", "r");
	if (fgets(str, 20, devtty) == NULL) {
		perror("fgets");
		exit(1);
	}
	str[strlen(str) - 1] = '\0';
	if(!*str) {
		(void) fprintf(stderr,"find: bad string\n");
		exit(1);
	}
	(void) close(fl);
	if((f = open(str, x ? 1 : 0)) < 0) {
		(void) fprintf(stderr, "That didn't work");
		(void) fclose(devtty);
		goto again;
	}
	return(f);
}
/*
 * 003 GAG
 * depth - null function to placate find's parse tree
 */
int
depth()
{
	return (1);
}
