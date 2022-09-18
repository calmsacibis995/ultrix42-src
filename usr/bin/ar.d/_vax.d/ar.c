#ifndef lint
static	char *sccsid = "@(#)ar.c	4.1	(ULTRIX)	7/17/90";
#endif
/*
 * ar - portable (ascii) format version
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>

#include <stdio.h>
#include <ar.h>
#include <signal.h>

/************************************************************************
 *									*
 *			Copyright (c) 1985,1986,1987,1988,1989 by	*
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

/************************************************************************
 *
 *			Modification History
 *
 *	Tim Newhouse, 11-April-1989
 * 007- Added calls to tempnam() when not using the 'l' option so that
 *	the temp files will use the TMPDIR environment symbol if it is
 *	defined.
 *
 *	Tim Newhouse, 12-Jan-1989
 * 006- Cloned match/trim routines for the -r option (rcmd)
 *	because it was not getting the file name truncation correct
 *	during the replace operation.  Pretty much a duplicate of the
 *	trim() and match() routines were used in a new routine rcmd_match
 *	which is now used just by rcmd() to do the matching of file names
 *	and still preserve the filename for later use in the opening of the
 *	file.
 *
 *	Mark Parenti, 08-Jun-1988
 * 005- Changed signal handlers to void.
 *
 *	Teoman Topcubasi, 15-Jan-1988
 * 004- Added a '-' in front of the options.
 *
 *	David L Ballenger, 16-Apr-1985
 * 003-	Change definition of tmpnam variable to tmpname so that it doesn't
 *	conflict with definition of tmpnam() in <stdio.h>.
 *
 *	Stephen Reilly, 15-Feb-84
 * 002- The uid and gid are unsigned elements but are being store into
 *	the ar files as signed elements
 *
 *	Stephen Reilly, 10-Nov-83:
 * 001- The n switch was not documented and is not used
 *
 ***********************************************************************/

struct	stat	stbuf;
struct	ar_hdr	arbuf;
struct	lar_hdr {
	char	lar_name[16];
	long	lar_date;
	u_short	lar_uid;
	u_short	lar_gid;
	u_short	lar_mode;
	long	lar_size;
} larbuf;

#define	SKIP	1
#define	IODD	2
#define	OODD	4
#define	HEAD	8

char	*man	=	{ "-mrxtdpq" };		/*  004 */
char	*opt	=	{ "uvbailo" };		/* slr001 n was removed */

int	signum[] = {SIGHUP, SIGINT, SIGQUIT, 0};
void	sigdone();
long	lseek();
int	rcmd();
int	dcmd();
int	xcmd();
int	tcmd();
int	pcmd();
int	mcmd();
int	qcmd();
int	(*comfun)();
char	flg[26];
char	**namv;
int	namc;
char	*arnam;
char	*ponam;
char	*tdir		=	{ "/tmp" };
char	*tmpname	=	{ "vXXXXX" };
char	*tmp1name	=	{ "v1XXXXX" };
char	*tmp2name	=	{ "v2XXXXX" };
char	*tfname;
char	*tf1name;
char	*tf2name;
char	*file;
char	name[16];
int	af;
int	tf;
int	tf1;
int	tf2;
int	qf;
int	bastate;
char	buf[MAXBSIZE];
int	truncate;			/* ok to truncate argument filenames */

char	*trim();
char	*mktemp();
char	*ctime();
char	*tempnam();

main(argc, argv)
char *argv[];
{
	register i;
	register char *cp;

	for(i=0; signum[i]; i++)
		if(signal(signum[i], SIG_IGN) != SIG_IGN)
			signal(signum[i], sigdone);
	if(argc < 3)
		usage();
	cp = argv[1];
	if (*cp == '-')			/* 004 */
		cp++;			/* 004 */
	for(; *cp; cp++)		/* 004 */
	switch(*cp) {
	case 'o':
	case 'l':
	case 'v':
	case 'u':
/*	case 'n':				slr001 n not longer used */
	case 'a':
	case 'b':
	case 'c':
	case 'i':
		flg[*cp - 'a']++;
		continue;

	case 'r':
		setcom(rcmd);
		continue;

	case 'd':
		setcom(dcmd);
		continue;

	case 'x':
		setcom(xcmd);
		continue;

	case 't':
		setcom(tcmd);
		continue;

	case 'p':
		setcom(pcmd);
		continue;

	case 'm':
		setcom(mcmd);
		continue;

	case 'q':
		setcom(qcmd);
		continue;

	default:
		fprintf(stderr, "ar: bad option `%c'\n", *cp);
		done(1);
	}
	if(flg['l'-'a'] == 0) {
		tmpname = "v";
		tmp1name = "v1";
		tmp2name = "v2";
	}
	if(flg['i'-'a'])
		flg['b'-'a']++;
	if(flg['a'-'a'] || flg['b'-'a']) {
		bastate = 1;
		ponam = trim(argv[2]);
		argv++;
		argc--;
		if(argc < 3)
			usage();
	}
	arnam = argv[2];
	namv = argv+3;
	namc = argc-3;
	if(comfun == 0) {
		if(flg['u'-'a'] == 0) {
			fprintf(stderr, "ar: one of [%s] must be specified\n", man);
			done(1);
		}
		setcom(rcmd);
	}
	(*comfun)();
	done(notfound());
}

setcom(fun)
int (*fun)();
{

	if(comfun != 0) {
		fprintf(stderr, "ar: only one of [%s] allowed\n", man);
		done(1);
	}
	comfun = fun;
}

rcmd()
{
	register f;

	init();
	getaf();

	/* turn on trim()'s truncation to 15 chars - we need this when
	 * movefil() calls trim to get the filename to store in the
	 * archive file.
	*/
	truncate++;

	while(!getdir()) {
		bamatch();
		if(namc == 0 || rcmd_match()) {
			f = stats();
			if(f < 0) {
				if(namc)
					fprintf(stderr, "ar: cannot open %s\n", file);
				goto cp;
			}
			if(flg['u'-'a'])
				if(stbuf.st_mtime <= larbuf.lar_date) {
					close(f);
					goto cp;
				}
			mesg('r');
			copyfil(af, -1, IODD+SKIP);
			movefil(f);
			continue;
		}
	cp:
		mesg('c');
		copyfil(af, tf, IODD+OODD+HEAD);
	}

	/* turn off (reset) trim()'s job of truncation to 15 chars */
	truncate--;

	cleanup();
}

dcmd()
{

	init();
	if(getaf())
		noar();
	while(!getdir()) {
		if(match()) {
			mesg('d');
			copyfil(af, -1, IODD+SKIP);
			continue;
		}
		mesg('c');
		copyfil(af, tf, IODD+OODD+HEAD);
	}
	install();
}

xcmd()
{
	register f;
	struct timeval tv[2];

	if(getaf())
		noar();
	while(!getdir()) {
		if(namc == 0 || match()) {
			f = creat(file, larbuf.lar_mode & 0777);
			if(f < 0) {
				fprintf(stderr, "ar: %s cannot create\n", file);
				goto sk;
			}
			mesg('x');
			copyfil(af, f, IODD);
			close(f);
			if (flg['o'-'a']) {
				tv[0].tv_sec = tv[1].tv_sec = larbuf.lar_date;
				tv[0].tv_usec = tv[1].tv_usec = 0;
				utimes(file, tv);
			}
			continue;
		}
	sk:
		mesg('c');
		copyfil(af, -1, IODD+SKIP);
		if (namc > 0  &&  !morefil())
			done(0);
	}
}

pcmd()
{

	if(getaf())
		noar();
	while(!getdir()) {
		if(namc == 0 || match()) {
			if(flg['v'-'a']) {
				printf("\n<%s>\n\n", file);
				fflush(stdout);
			}
			copyfil(af, 1, IODD);
			continue;
		}
		copyfil(af, -1, IODD+SKIP);
	}
}

mcmd()
{

	init();
	if(getaf())
		noar();
	if(flg['l' - 'a'])
	    tf2name = mktemp(tmp2name);
	else
	    tf2name = tempnam(tdir,tmp2name);

	close(creat(tf2name, 0600));
	tf2 = open(tf2name, 2);
	if(tf2 < 0) {
		fprintf(stderr, "ar: cannot create third temp\n");
		done(1);
	}
	while(!getdir()) {
		bamatch();
		if(match()) {
			mesg('m');
			copyfil(af, tf2, IODD+OODD+HEAD);
			continue;
		}
		mesg('c');
		copyfil(af, tf, IODD+OODD+HEAD);
	}
	install();
}

tcmd()
{

	if(getaf())
		noar();
	while(!getdir()) {
		if(namc == 0 || match()) {
			if(flg['v'-'a'])
				longt();
			printf("%s\n", trim(file));
		}
		copyfil(af, -1, IODD+SKIP);
	}
}

qcmd()
{
	register i, f;

	if (flg['a'-'a'] || flg['b'-'a']) {
		fprintf(stderr, "ar: abi not allowed with q\n");
		done(1);
	}
	truncate++;
	getqf();
	for(i=0; signum[i]; i++)
		signal(signum[i], SIG_IGN);
	lseek(qf, 0l, 2);
	for(i=0; i<namc; i++) {
		file = namv[i];
		if(file == 0)
			continue;
		namv[i] = 0;
		mesg('q');
		f = stats();
		if(f < 0) {
			fprintf(stderr, "ar: %s cannot open\n", file);
			continue;
		}
		tf = qf;
		movefil(f);
		qf = tf;
	}
}

init()
{

	if(flg['l'-'a'])
	    tfname = mktemp(tmpname);
	else
	    tfname = tempnam(tdir,tmpname);

	close(creat(tfname, 0600));
	tf = open(tfname, 2);
	if(tf < 0) {
		fprintf(stderr, "ar: cannot create temp file\n");
		done(1);
	}
	if (write(tf, ARMAG, SARMAG) != SARMAG)
		wrerr();
}

getaf()
{
	char mbuf[SARMAG];

	af = open(arnam, 0);
	if(af < 0)
		return(1);
	if (read(af, mbuf, SARMAG) != SARMAG || strncmp(mbuf, ARMAG, SARMAG)) {
		fprintf(stderr, "ar: %s not in archive format\n", arnam);
		done(1);
	}
	return(0);
}

getqf()
{
	char mbuf[SARMAG];

	if ((qf = open(arnam, 2)) < 0) {
		if(!flg['c'-'a'])
			fprintf(stderr, "ar: creating %s\n", arnam);
		if ((qf = creat(arnam, 0666)) < 0) {
			fprintf(stderr, "ar: cannot create %s\n", arnam);
			done(1);
		}
		if (write(qf, ARMAG, SARMAG) != SARMAG)
			wrerr();
	} else if (read(qf, mbuf, SARMAG) != SARMAG
		|| strncmp(mbuf, ARMAG, SARMAG)) {
		fprintf(stderr, "ar: %s not in archive format\n", arnam);
		done(1);
	}
}

usage()
{
	printf("usage: ar [%s][%s] archive files ...\n", man, opt);
	done(1);
}

noar()
{

	fprintf(stderr, "ar: %s does not exist\n", arnam);
	done(1);
}

void
sigdone()
{
	done(100);
}

done(c)
{

	if(tfname)
		unlink(tfname);
	if(tf1name)
		unlink(tf1name);
	if(tf2name)
		unlink(tf2name);
	exit(c);
}

notfound()
{
	register i, n;

	n = 0;
	for(i=0; i<namc; i++)
		if(namv[i]) {
			fprintf(stderr, "ar: %s not found\n", namv[i]);
			n++;
		}
	return(n);
}

morefil()
{
	register i, n;

	n = 0;
	for(i=0; i<namc; i++)
		if(namv[i])
			n++;
	return(n);
}

cleanup()
{
	register i, f;

	truncate++;
	for(i=0; i<namc; i++) {
		file = namv[i];
		if(file == 0)
			continue;
		namv[i] = 0;
		mesg('a');
		f = stats();
		if(f < 0) {
			fprintf(stderr, "ar: %s cannot open\n", file);
			continue;
		}
		movefil(f);
	}
	install();
}

install()
{
	register i;

	for(i=0; signum[i]; i++)
		signal(signum[i], SIG_IGN);
	if(af < 0)
		if(!flg['c'-'a'])
			fprintf(stderr, "ar: creating %s\n", arnam);
	close(af);
	af = creat(arnam, 0666);
	if(af < 0) {
		fprintf(stderr, "ar: cannot create %s\n", arnam);
		done(1);
	}
	if(tfname) {
		lseek(tf, 0l, 0);
		while((i = read(tf, buf, sizeof(buf))) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
	if(tf2name) {
		lseek(tf2, 0l, 0);
		while((i = read(tf2, buf, sizeof(buf))) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
	if(tf1name) {
		lseek(tf1, 0l, 0);
		while((i = read(tf1, buf, sizeof(buf))) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
}

/*
 * insert the file 'file'
 * into the temporary file
 */
movefil(f)
{
	char buf[sizeof(arbuf)+1];

	sprintf(buf, "%-16s%-12ld%-6u%-6u%-8o%-10ld%-2s",
	   trim(file),
	   stbuf.st_mtime,
	   (unsigned short) stbuf.st_uid,		/* slr002 */
	   (unsigned short) stbuf.st_gid,		/* slr002 */
	   stbuf.st_mode,
	   stbuf.st_size,
	   ARFMAG);
	strncpy((char *)&arbuf, buf, sizeof(arbuf));
	larbuf.lar_size = stbuf.st_size;
	copyfil(f, tf, OODD+HEAD);
	close(f);
}

stats()
{
	register f;

	f = open(file, 0);
	if(f < 0)
		return(f);
	if(fstat(f, &stbuf) < 0) {
		close(f);
		return(-1);
	}
	return(f);
}

/*
 * copy next file
 * size given in arbuf
 */
copyfil(fi, fo, flag)
{
	register i, o;
	int pe;

	if(flag & HEAD) {
		for (i=sizeof(arbuf.ar_name)-1; i>=0; i--) {
			if (arbuf.ar_name[i]==' ')
				continue;
			else if (arbuf.ar_name[i]=='\0')
				arbuf.ar_name[i] = ' ';
			else
				break;
		}
		if (write(fo, (char *)&arbuf, sizeof arbuf) != sizeof arbuf)
			wrerr();
	}
	pe = 0;
	while(larbuf.lar_size > 0) {
		i = o = sizeof(buf);
		if(larbuf.lar_size < i) {
			i = o = larbuf.lar_size;
			if(i&1) {
				buf[i] = '\n';
				if(flag & IODD)
					i++;
				if(flag & OODD)
					o++;
			}
		}
		if(read(fi, buf, i) != i)
			pe++;
		if((flag & SKIP) == 0)
			if (write(fo, buf, o) != o)
				wrerr();
		larbuf.lar_size -= sizeof(buf);
	}
	if(pe)
		phserr();
}

getdir()
{
	register char *cp;
	register i;

	i = read(af, (char *)&arbuf, sizeof arbuf);
	if(i != sizeof arbuf) {
		if(tf1name) {
			i = tf;
			tf = tf1;
			tf1 = i;
		}
		return(1);
	}
	if (strncmp(arbuf.ar_fmag, ARFMAG, sizeof(arbuf.ar_fmag))) {
		fprintf(stderr, "ar: malformed archive (at %ld)\n", lseek(af, 0L, 1));
		done(1);
	}
	cp = arbuf.ar_name + sizeof(arbuf.ar_name);
	while (*--cp==' ')
		;
	*++cp = '\0';
	strncpy(name, arbuf.ar_name, sizeof(arbuf.ar_name));
	file = name;
	strncpy(larbuf.lar_name, name, sizeof(larbuf.lar_name));
	sscanf(arbuf.ar_date, "%ld", &larbuf.lar_date);
	sscanf(arbuf.ar_uid, "%hd", &larbuf.lar_uid);
	sscanf(arbuf.ar_gid, "%hd", &larbuf.lar_gid);
	sscanf(arbuf.ar_mode, "%ho", &larbuf.lar_mode);
	sscanf(arbuf.ar_size, "%ld", &larbuf.lar_size);
	return(0);
}

/* rcmd_match()
 *
 * This function is just for use by rcmd().  This does the same job
 * as calling match() which calls trim() but it leaves the
 * filename in the original state so it can be used later for the open.
*/
rcmd_match()
{
	register i;
	register char *p1, *p2;

	for(i=0; i<namc; i++) {
		if(namv[i] == 0)
			continue;

		/* trim the name to just the filename and compare only
		 * 15 characters if needed.  But, we need to leave
		 * the full filename before returning to rcmd which will
		 * use the filename to do the open.
		*/

		/* Strip trailing slashes */
		for(p1 = namv[i]; *p1; p1++)
		    ;

		while(p1 > namv[i]){
		    if(*--p1 != '/')
			break;
		    *p1 = 0;
		}

		/* Find last component of path; do not zap the path */
		p2 = namv[i];
		for(p1 = namv[i]; *p1; p1++)
		    if(*p1 == '/')
			p2 = p1+1;


		/* p2 now points to just the filename of the file spec.
		 * We only compare the characters that will fit in the
		 * space for ar's filenames.  Later on (if this matches)
		 * rcmd() will call movefil() which calls trim() and it
		 * will really truncate the filename when we don't need the
		 * full spec any more.
		*/
		if(strncmp(p2, file, (sizeof(arbuf.ar_name) - 1)) == 0) {
		    file = namv[i];
		    namv[i] = 0;
		    return(1);
		}
	}
	return(0);
}

match()
{
	register i;

	for(i=0; i<namc; i++) {
		if(namv[i] == 0)
			continue;
		if(strcmp(trim(namv[i]), file) == 0) {
			file = namv[i];
			namv[i] = 0;
			return(1);
		}
	}
	return(0);
}

bamatch()
{
	register f;

	switch(bastate) {

	case 1:
		if(strcmp(file, ponam) != 0)
			return;
		bastate = 2;
		if(flg['a'-'a'])
			return;

	case 2:
		bastate = 0;
		if(flg['l'-'a'])
		    tf1name = mktemp(tmp1name);
		else
		    tf1name = tempnam(tdir,tmp1name);
		close(creat(tf1name, 0600));
		f = open(tf1name, 2);
		if(f < 0) {
			fprintf(stderr, "ar: cannot create second temp\n");
			return;
		}
		tf1 = tf;
		tf = f;
	}
}

phserr()
{

	fprintf(stderr, "ar: phase error on %s\n", file);
}

mesg(c)
{

	if(flg['v'-'a'])
		if(c != 'c' || flg['v'-'a'] > 1)
			printf("%c - %s\n", c, file);
}

char *
trim(s)
char *s;
{
	register char *p1, *p2;

	/* Strip trailing slashes */
	for(p1 = s; *p1; p1++)
		;
	while(p1 > s) {
		if(*--p1 != '/')
			break;
		*p1 = 0;
	}

	/* Find last component of path; do not zap the path */
	p2 = s;
	for(p1 = s; *p1; p1++)
		if(*p1 == '/')
			p2 = p1+1;

	/*
	 * Truncate name if too long, only if we are doing an 'add'
	 * type operation. We only allow 15 cause rest of ar
	 * isn't smart enough to deal with non-null terminated
	 * names.  Need an exit status convention...
	 * Need yet another new archive format...
	 */
	if (truncate && strlen(p2) > sizeof(arbuf.ar_name) - 1) {
		fprintf(stderr, "ar: filename %s truncated to ", p2);
		*(p2 + sizeof(arbuf.ar_name) - 1) = '\0';
		fprintf(stderr, "%s\n", p2);
	}
	return(p2);
}

#define	IFMT	060000
#define	ISARG	01000
#define	LARGE	010000
#define	SUID	04000
#define	SGID	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000

longt()
{
	register char *cp;

	pmode();
	printf("%5d/%-5d", larbuf.lar_uid, larbuf.lar_gid);
	printf(" %6ld", larbuf.lar_size);
	cp = ctime(&larbuf.lar_date);
	printf(" %-12.12s %-4.4s ", cp+4, cp+20);
}

int	m1[] = { 1, ROWN, 'r', '-' };
int	m2[] = { 1, WOWN, 'w', '-' };
int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int	m4[] = { 1, RGRP, 'r', '-' };
int	m5[] = { 1, WGRP, 'w', '-' };
int	m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int	m7[] = { 1, ROTH, 'r', '-' };
int	m8[] = { 1, WOTH, 'w', '-' };
int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

pmode()
{
	register int **mp;

	for (mp = &m[0]; mp < &m[9];)
		select(*mp++);
}

select(pairp)
int *pairp;
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n>=0 && (larbuf.lar_mode&*ap++)==0)
		ap++;
	putchar(*ap);
}

wrerr()
{
	perror("ar write error");
	done(1);
}
