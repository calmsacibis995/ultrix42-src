#ifndef lint
static char *sccsid = "@(#)mkproto.c	4.1	ULTRIX	7/2/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 * Make a file system prototype.
 * usage: mkproto filsys proto
 */

/*									*
 *		Modification History					*
 *									*
 * 22 Aug 88 -- prs							*
 *	Added calculations of the blocks field of an on-disk		*
 *	inode.								*
 *									*
 * 1) - 15 Mar 85 -- funding						*
 *	Added named pipe support (re. System V named pipes)		*
 */

#define NPIPES							/* 1 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/gnode.h>
#include <sys/gnode_common.h>
#include <ufs/ufs_inode.h>
#include <sys/fs.h>
#include <sys/dir.h>

#define I_TO_UP(i) ((struct ufs_inode *)(&(i.g_in)))
#define IP_TO_UP(i) ((struct ufs_inode *)(&(i->g_in)))

union {
	struct	fs fs;
	char	fsx[SBSIZE];
} ufs;
#define sblock	ufs.fs
union {
	struct	cg cg;
	char	cgx[MAXBSIZE];
} ucg;
#define	acg	ucg.cg
struct	fs *fs;
struct	csum *fscs;
int	fso, fsi;
FILE	*proto;
char	token[BUFSIZ];
int	errs;
int	ino = 10;
long	getnum();
char	*strcpy();

main(argc, argv)
	int argc;
	char *argv[];
{
	int i;

	if (argc != 3) {
		fprintf(stderr, "usage: mkproto filsys proto\n");
		exit(1);
	}
	fso = open(argv[1], 1);
	fsi = open(argv[1], 0);
	if (fso < 0 || fsi < 0) {
		perror(argv[1]);
		exit(1);
	}
	fs = &sblock;
	rdfs(SBLOCK, SBSIZE, (char *)fs);
	fscs = (struct csum *)calloc(1, fs->fs_cssize);
	for (i = 0; i < fs->fs_cssize; i += fs->fs_bsize)
		rdfs(fsbtodb(fs, fs->fs_csaddr + numfrags(fs, i)),
			(int)(fs->fs_cssize - i < fs->fs_bsize ?
			    fs->fs_cssize - i : fs->fs_bsize),
			((char *)fscs) + i);
	proto = fopen(argv[2], "r");
	descend((struct gnode *)0);
	wtfs(SBLOCK, SBSIZE, (char *)fs);
	for (i = 0; i < fs->fs_cssize; i += fs->fs_bsize)
		wtfs(fsbtodb(&sblock, fs->fs_csaddr + numfrags(&sblock, i)),
			(int)(fs->fs_cssize - i < fs->fs_bsize ?
			    fs->fs_cssize - i : fs->fs_bsize),
			((char *)fscs) + i);
	exit(errs);
}

descend(par)
	struct gnode *par;
{
	struct gnode in;
	int ibc = 0;
	int i, f, c;
	struct dinode *dip, inos[MAXBSIZE / sizeof (struct dinode)];
	daddr_t ib[MAXBSIZE / sizeof (daddr_t)];
	char buf[MAXBSIZE];

	getstr();
	in.g_mode = gmode(token[0], "-bcdp", GFREG, GFBLK, GFCHR,/* 1 */
							GFDIR, GFPORT);
	in.g_mode |= gmode(token[1], "-u", 0, GSUID, 0, 0, 0);	/* 1 */
	in.g_mode |= gmode(token[2], "-g", 0, GSGID, 0, 0, 0);	/* 1 */
	for (i = 3; i < 6; i++) {
		c = token[i];
		if (c < '0' || c > '7') {
			printf("%c/%s: bad octal mode digit\n", c, token);
			errs++;
			c = 0;
		}
		in.g_mode |= (c-'0')<<(15-3*i);
	}
	in.g_uid = getnum(); in.g_gid = getnum();
	for (i = 0; i < fs->fs_bsize; i++)
		buf[i] = 0;
	for (i = 0; i < NINDIR(fs); i++)
		ib[i] = (daddr_t)0;
	in.g_nlink = 1;
	in.g_size = 0;
	(I_TO_UP(in))->di_blocks = 0;
	for (i = 0; i < NDADDR; i++)
		(I_TO_UP(in))->di_db[i] = (daddr_t)0;
	for (i = 0; i < NIADDR; i++)
		(I_TO_UP(in))->di_ib[i] = (daddr_t)0;
	if (par != (struct gnode *)0) {
		ialloc(&in);
	} else {
		par = &in;
		i = itod(fs, ROOTINO);
		rdfs(fsbtodb(fs, i), fs->fs_bsize, (char *)inos);
		dip = &inos[ROOTINO % INOPB(fs)];
		in.g_number = ROOTINO;
		in.g_nlink = dip->di_nlink;
		in.g_size = dip->di_size;
		(I_TO_UP(in))->di_blocks = dip->di_blocks;
		(I_TO_UP(in))->di_db[0] = dip->di_db[0];
		rdfs(fsbtodb(fs, (I_TO_UP(in))->di_db[0]), fs->fs_bsize, buf);
	}

	switch (in.g_mode&GFMT) {

	case GFREG:
		getstr();
		f = open(token, 0);
		if (f < 0) {
			printf("%s: cannot open\n", token);
			errs++;
			break;
		}
		while ((i = read(f, buf, (int)fs->fs_bsize)) > 0) {
			in.g_size += i;
			newblk(&in, buf, &ibc, ib, (int)blksize(fs, &in, ibc));
		}
		close(f);
		break;

	case GFBLK:
	case GFCHR:
		/*
		 * special file
		 * content is maj/min types
		 */

		i = getnum() & 0377;
		f = getnum() & 0377;
		in.g_rdev = (i << 8) | f;
		break;

	case GFDIR:
		/*
		 * directory
		 * put in extra links
		 * call recursively until
		 * name of "$" found
		 */

		if (in.g_number != ROOTINO) {
			par->g_nlink++;
			in.g_nlink++;
			entry(&in, in.g_number, ".", buf);
			entry(&in, par->g_number, "..", buf);
		}
		for (;;) {
			getstr();
			if (token[0]=='$' && token[1]=='\0')
				break;
			entry(&in, (ino_t)(ino+1), token, buf);
			descend(&in);
		}
		if (in.g_number != ROOTINO)
			newblk(&in, buf, &ibc, ib, (int)blksize(fs, &in, 0));
		else
			wtfs(fsbtodb(fs, (I_TO_UP(in))->di_db[0]),
					(int)fs->fs_bsize, buf);
		break;
	}
	iput(&in, &ibc, ib);
}

/*ARGSUSED*/
gmode(c, s, m0, m1, m2, m3, m4)					/* 1 */
	char c, *s;
{
	int i;

	for (i = 0; s[i]; i++)
		if (c == s[i])
			return((&m0)[i]);
	printf("%c/%s: bad mode\n", c, token);
	errs++;
	return(0);
}

long
getnum()
{
	int i, c;
	long n;

	getstr();
	n = 0;
	i = 0;
	for (i = 0; c=token[i]; i++) {
		if (c<'0' || c>'9') {
			printf("%s: bad number\n", token);
			errs++;
			return((long)0);
		}
		n = n*10 + (c-'0');
	}
	return(n);
}

getstr()
{
	int i, c;

loop:
	switch (c = getc(proto)) {

	case ' ':
	case '\t':
	case '\n':
		goto loop;

	case EOF:
		printf("Unexpected EOF\n");
		exit(1);

	case ':':
		while (getc(proto) != '\n')
			;
		goto loop;

	}
	i = 0;
	do {
		token[i++] = c;
		c = getc(proto);
	} while (c != ' ' && c != '\t' && c != '\n' && c != '\0');
	token[i] = 0;
}

entry(ip, inum, str, buf)
	struct gnode *ip;
	ino_t inum;
	char *str;
	char *buf;
{
	register struct direct *dp, *odp;
	int oldsize, newsize, spacefree;

	odp = dp = (struct direct *)buf;
	while ((int)dp - (int)buf < ip->g_size) {
		odp = dp;
		dp = (struct direct *)((int)dp + dp->d_reclen);
	}
	if (odp != dp)
		oldsize = DIRSIZ(odp);
	else
		oldsize = 0;
	spacefree = odp->d_reclen - oldsize;
	dp = (struct direct *)((int)odp + oldsize);
	dp->d_ino = inum;
	dp->d_namlen = strlen(str);
	newsize = DIRSIZ(dp);
	if (spacefree >= newsize) {
		odp->d_reclen = oldsize;
		dp->d_reclen = spacefree;
	} else {
		dp = (struct direct *)((int)odp + odp->d_reclen);
		if ((int)dp - (int)buf >= fs->fs_bsize) {
			printf("directory too large\n");
			exit(1);
		}
		dp->d_ino = inum;
		dp->d_namlen = strlen(str);
		dp->d_reclen = DIRBLKSIZ;
	}
	strcpy(dp->d_name, str);
	ip->g_size = (int)dp - (int)buf + newsize;
}

newblk(ip, buf, aibc, ib, size)
	struct gnode *ip;
	int *aibc;
	char *buf;
	daddr_t *ib;
	int size;
{
	int i;
	daddr_t bno;

	bno = alloc(size);
	wtfs(fsbtodb(fs, bno), (int)fs->fs_bsize, buf);
	for (i = 0; i < fs->fs_bsize; i++)
		buf[i] = 0;
	ib[(*aibc)++] = bno;
	if (*aibc >= NINDIR(fs)) {
		printf("indirect block full\n");
		errs++;
		*aibc = 0;
	}
	(IP_TO_UP(ip))->di_blocks += btodb(size);
}

iput(ip, aibc, ib)
	struct gnode *ip;
	int *aibc;
	daddr_t *ib;
{
	daddr_t d;
	int i;
	struct dinode buf[MAXBSIZE / sizeof (struct dinode)];
	struct timeval utime;
	struct timezone uzone;

	gettimeofday(&utime,&uzone);
	ip->g_atime = ip->g_mtime = ip->g_ctime = utime;
	switch (ip->g_mode&GFMT) {

	case GFDIR:
	case GFREG:
		for (i = 0; i < *aibc; i++) {
			if (i >= NDADDR)
				break;
			(IP_TO_UP(ip))->di_db[i] = ib[i];
		}
		if (*aibc > NDADDR) {
			(IP_TO_UP(ip))->di_ib[0] = alloc((int)fs->fs_bsize);
			(IP_TO_UP(ip))->di_blocks += btodb((int)fs->fs_bsize);
			for (i = 0; i < NINDIR(fs) - NDADDR; i++) {
				ib[i] = ib[i+NDADDR];
				ib[i+NDADDR] = (daddr_t)0;
			}
			wtfs(fsbtodb(fs, (IP_TO_UP(ip))->di_ib[0]),
			    (int)fs->fs_bsize, (char *)ib);
		}
		break;

	case GFBLK:
	case GFCHR:
		break;

	default:
		printf("bad mode %o\n", ip->g_mode);
		exit(1);
	}
	d = fsbtodb(fs, itod(fs, ip->g_number));
	rdfs(d, (int)fs->fs_bsize, (char *)buf);
	buf[itoo(fs, ip->g_number)].di_ic = (IP_TO_UP(ip))->di_ic;
	wtfs(d, (int)fs->fs_bsize, (char *)buf);
}

daddr_t
alloc(size)
	int size;
{
	int i, frag;
	daddr_t d;
	static int cg = 0;

again:
	rdfs(fsbtodb(&sblock, cgtod(&sblock, cg)), (int)sblock.fs_cgsize,
	    (char *)&acg);
	if (acg.cg_magic != CG_MAGIC) {
		printf("cg %d: bad magic number\n", cg);
		return (0);
	}
	if (acg.cg_cs.cs_nbfree == 0) {
		cg++;
		if (cg >= fs->fs_ncg) {
			printf("ran out of space\n");
			return (0);
		}
		goto again;
	}
	for (d = 0; d < acg.cg_ndblk; d += sblock.fs_frag)
		if (isblock(&sblock, (u_char *)acg.cg_free, d / sblock.fs_frag))
			goto goth;
	printf("internal error: can't find block in cyl %d\n", cg);
	return (0);
goth:
	clrblock(&sblock, (u_char *)acg.cg_free, d / sblock.fs_frag);
	acg.cg_cs.cs_nbfree--;
	sblock.fs_cstotal.cs_nbfree--;
	fscs[cg].cs_nbfree--;
	acg.cg_btot[cbtocylno(&sblock, d)]--;
	acg.cg_b[cbtocylno(&sblock, d)][cbtorpos(&sblock, d)]--;
	if (size != sblock.fs_bsize) {
		frag = howmany(size, sblock.fs_fsize);
		fscs[cg].cs_nffree += sblock.fs_frag - frag;
		sblock.fs_cstotal.cs_nffree += sblock.fs_frag - frag;
		acg.cg_cs.cs_nffree += sblock.fs_frag - frag;
		acg.cg_frsum[sblock.fs_frag - frag]++;
		for (i = frag; i < sblock.fs_frag; i++)
			setbit(acg.cg_free, d + i);
	}
	wtfs(fsbtodb(&sblock, cgtod(&sblock, cg)), (int)sblock.fs_cgsize,
	    (char *)&acg);
	return (acg.cg_cgx * fs->fs_fpg + d);
}

/*
 * Allocate an gnode on the disk
 */
ialloc(ip)
	register struct gnode *ip;
{
	struct dinode buf[MAXBSIZE / sizeof (struct dinode)];
	daddr_t d;
	int c;

	ip->g_number = ++ino;
	c = itog(&sblock, ip->g_number);
	rdfs(fsbtodb(&sblock, cgtod(&sblock, c)), (int)sblock.fs_cgsize,
	    (char *)&acg);
	if (acg.cg_magic != CG_MAGIC) {
		printf("cg %d: bad magic number\n", c);
		exit(1);
	}
	if (ip->g_mode & GFDIR) {
		acg.cg_cs.cs_ndir++;
		sblock.fs_cstotal.cs_ndir++;
		fscs[c].cs_ndir++;
	}
	acg.cg_cs.cs_nifree--;
	setbit(acg.cg_iused, ip->g_number);
	wtfs(fsbtodb(&sblock, cgtod(&sblock, c)), (int)sblock.fs_cgsize,
	    (char *)&acg);
	sblock.fs_cstotal.cs_nifree--;
	fscs[c].cs_nifree--;
	if(ip->g_number >= sblock.fs_ipg * sblock.fs_ncg) {
		printf("fsinit: gnode value out of range (%d).\n",
		    ip->g_number);
		exit(1);
	}
	return (ip->g_number);
}

/*
 * read a block from the file system
 */
rdfs(bno, size, bf)
	int bno, size;
	char *bf;
{
	int n;

	if (lseek(fsi, bno * DEV_BSIZE, 0) < 0) {
		printf("seek error: %ld\n", bno);
		perror("rdfs");
		exit(1);
	}
	n = read(fsi, bf, size);
	if(n != size) {
		printf("read error: %ld\n", bno);
		perror("rdfs");
		exit(1);
	}
}

/*
 * write a block to the file system
 */
wtfs(bno, size, bf)
	int bno, size;
	char *bf;
{
	int n;

	lseek(fso, bno * DEV_BSIZE, 0);
	if (lseek(fso, bno * DEV_BSIZE, 0) < 0) {
		printf("seek error: %ld\n", bno);
		perror("wtfs");
		exit(1);
	}
	n = write(fso, bf, size);
	if(n != size) {
		printf("write error: %D\n", bno);
		perror("wtfs");
		exit(1);
	}
}
/*
 * check if a block is available
 */
isblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	int h;
{
	unsigned char mask;

	switch (fs->fs_frag) {
	case 8:
		return (cp[h] == 0xff);
	case 4:
		mask = 0x0f << ((h & 0x1) << 2);
		return ((cp[h >> 1] & mask) == mask);
	case 2:
		mask = 0x03 << ((h & 0x3) << 1);
		return ((cp[h >> 2] & mask) == mask);
	case 1:
		mask = 0x01 << (h & 0x7);
		return ((cp[h >> 3] & mask) == mask);
	default:
		fprintf(stderr, "isblock bad fs_frag %d\n", fs->fs_frag);
		return (0);
	}
	/*NOTREACHED*/
}

/*
 * take a block out of the map
 */
clrblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	int h;
{
	switch ((fs)->fs_frag) {
	case 8:
		cp[h] = 0;
		return;
	case 4:
		cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] &= ~(0x01 << (h & 0x7));
		return;
	default:
		fprintf(stderr, "clrblock bad fs_frag %d\n", fs->fs_frag);
		return;
	}
}

