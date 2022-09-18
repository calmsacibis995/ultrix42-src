
# ifndef lint
static char *sccsid = "@(#)dumptraverse.c	4.2    ULTRIX  2/21/91";
# endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 * Modification History: /usr/src/etc/dump/dumptraverse.c
 *
 * 12 Feb 91 -- lambert
 *	Changed "this should not happen" message in bread() to be more
 *	meaningful (and less threatening).  Also changed dmpindir() call 
 *	blksout() to use correct arg for "ip".
 *
 *  7 Apr 89 -- lambert
 *	Corrected call to "blksout" in dmpindir - it was using wrong # of
 *	args.
 *
 * 13 Jun 86 -- fries
 *	Replaced di_mtime with di_mtime.tv_sec
 *	Replaced di_ctime with di_ctime.tv_sec conform with ufs changes.
 *
 * 28 Apr 86 -- lp
 *	Added n-buffered hooks.
 *
 * 29 Jan 86 -- fries
 *		Added comments to clarify code.
 *
 * 07 Aug 85 -- prs
 *      Added code to prevent dump from writing out extra blocks of data
 *      at the end of a file. This problem would only occur when the fragment
 *      size was greater than TP_BSIZE, and would cause RESTORE to have
 *      Resync warning.
 *
 * 23 Apr 84 -- jmcg
 *	TomT had added these changes to ULTRIX-32 sources.  They came from
 *	MIT.  Most of the changes are making function parameters be storage
 *	class "register", but it appears they re-did some of the bitmap
 *	functions.
 *
 * 23 Apr 84 -- jmcg
 *	Derived from 4.2BSD, labeled:
 *		dumptraverse.c	1.15 (Berkeley) 9/25/83
 *
 * ------------------------------------------------------------------------
 */


#include "dump.h"
#include <sys/ioctl.h>
#include <sys/errno.h>

/* Pass a function and a bitmap and perform function using the passed */
/* Bitmap. If pointer to bitmap is NULL, then set up map data, else   */
/* use the data in the map.					      */
pass(fn, map)
	int (*fn)();
	register char *map;
{
	register struct dinode *dp;
	register int bits;
	register ino_t maxino;

	/* Calculate the maximum inode # for file system */
	/* by multiplying the # inodes per cly group by  */
	/* the # of cylinder groups                      */
	maxino = sblock->fs_ipg * sblock->fs_ncg - 1;
	if (!map) /* If NULL map pointer */
	    for (ino = 0; ino < maxino; ino++)
		(*fn)(getino(ino));
	else {
	    for (ino = 0; ino < maxino; ) {
		if((ino % NBBY) == 0) {
			bits = ~0;
			if(map != NULL)
				bits = *map++;
		}
		ino++;
		/* if bit set in map */
		if(bits & 1) {
			dp = getino(ino);
			(*fn)(dp); /* call indirect function passing dp */
		}
		bits >>= 1; /* and check the next most sig. bit */
	    }
	}
}

/* Mark bitmaps (based on inode data) */
mark(ip)
	register struct dinode *ip;
{
	register f;
	register i = (ino - 1) >> 3; /* calc. index into map */
	register m = 1 << ((ino - 1) & 7); /* calc. which bit */

	f = ip->di_mode & IFMT; /* get inode type & mode info. */
	if(f == 0)
		return; /* if inode NULL */
	clrmap[i] |= m; /* set bit in map for any inode */
	if(f == IFDIR)
		dirmap[i] |= m; /* if dir. inode, then set bit */

	/* if date of inode >= last dump date, then set bit */
	if ((ip->di_mtime.tv_sec >= spcl.c_ddate
	   || ip->di_ctime.tv_sec >= spcl.c_ddate) &&
	    !(nodmap[i] & m)) {
		nodmap[i] |= m;
		if (f != IFREG && f != IFDIR && f != IFLNK) {
			esize += 1;
			return;
		}
		est(ip);
	}
}

add(ip)
	register struct	dinode	*ip;
{
	register int i;
	long filesize;

	if(BIT(ino, nodmap))
		return;
	nsubdir = 0;
	dadded = 0;
	filesize = ip->di_size;
	for (i = 0; i < NDADDR; i++) {
		if (ip->di_db[i] != 0)
			dsrch(ip->di_db[i], dblksize(sblock, ip, i), filesize);
		filesize -= sblock->fs_bsize;
	}
	for (i = 0; i < NIADDR; i++) {
		if (ip->di_ib[i] != 0)
			indir(ip->di_ib[i], i, &filesize);
	}
	if(dadded) {
		nadded++;
		if (!BIT(ino, nodmap)) {
			BIS(ino, nodmap);
			est(ip);
		}
	}
	if(nsubdir == 0)
		if(!BIT(ino, nodmap))
			BIC(ino, dirmap);
}

indir(d, n, filesize)
	daddr_t d;
	register int n, *filesize;
{
	register i;
	daddr_t	idblk[MAXNINDIR];

	bread(fsbtodb(sblock, d), (char *)idblk, sblock->fs_bsize, 1);
	if(n <= 0) {
		for(i=0; i < NINDIR(sblock); i++) {
			d = idblk[i];
			if(d != 0)
				dsrch(d, sblock->fs_bsize, *filesize);
			*filesize -= sblock->fs_bsize;
		}
	} else {
		n--;
		for(i=0; i < NINDIR(sblock); i++) {
			d = idblk[i];
			if(d != 0)
				indir(d, n, filesize);
		}
	}
}

dump(ip)
	register struct dinode *ip;
{
	register int i;
	long size;

	if(newtape) {
		newtape = 0;
		bitmap(nodmap, TS_BITS);
	}
	BIC(ino, nodmap);
	spcl.c_dinode = *ip;
	spcl.c_type = TS_INODE;
	spcl.c_count = 0;
	i = ip->di_mode & IFMT;
	if ((i != IFDIR && i != IFREG && i != IFLNK) || ip->di_size == 0) {
		spclrec();
		return;
	}
	if (ip->di_size > NDADDR * sblock->fs_bsize)
		i = NDADDR * sblock->fs_frag;
	else
		i = howmany(ip->di_size, sblock->fs_fsize);
	blksout(&ip->di_db[0], i, ip);
	size = ip->di_size - NDADDR * sblock->fs_bsize;
	if (size <= 0)
		return;
	for (i = 0; i < NIADDR; i++) {
		dmpindir(ip->di_ib[i], i, &size, ip);
		if (size <= 0)
			return;
	}
}

dmpindir(blk, lvl, size, ip)
	daddr_t blk;
	register int lvl;
	register long *size;
	register struct dinode *ip;
{
	register int i, cnt;
	daddr_t idblk[MAXNINDIR];

	if (blk != 0)
		bread(fsbtodb(sblock, blk), (char *)idblk, sblock->fs_bsize,1);
	else
		bzero(idblk, sblock->fs_bsize);
	if (lvl <= 0) {
		if (*size < NINDIR(sblock) * sblock->fs_bsize)
			cnt = howmany(*size, sblock->fs_fsize);
		else
			cnt = NINDIR(sblock) * sblock->fs_frag;
		*size -= NINDIR(sblock) * sblock->fs_bsize;
#ifdef DMPINDIR_DEBUG
		fprintf(stderr, "dmpindir calls blksout with ip = %d\n", 
			*ip);
		fflush(stderr);
#endif
		blksout(&idblk[0], cnt, ip);
		return;
	}
	lvl--;
	for (i = 0; i < NINDIR(sblock); i++) {
		dmpindir(idblk[i], lvl, size, ip);
		if (*size <= 0)
			return;
	}
}

blksout(blkp, frags, ip)
	register daddr_t *blkp;
	int frags;
        register struct dinode *ip; /* the file inode is needed for the
                                       file size                        */
{
	register int i, j, count, blks, tbperdb;
        int k=0;    /* k=The amount of bytes already written to dump tape */
        int size; /* size=The amount of bytes left to write for a file */

        blks = howmany(frags * sblock->fs_fsize, TP_BSIZE);
	tbperdb = sblock->fs_bsize / TP_BSIZE;
	for (i = 0; i < blks; i += TP_NINDIR) {
		if (i + TP_NINDIR > blks)
			count = blks;
		else
			count = i + TP_NINDIR;
		for (j = i; j < count; j++)
			if (blkp[j / tbperdb] != 0)
				spcl.c_addr[j - i] = 1;
			else
				spcl.c_addr[j - i] = 0;
		spcl.c_count = count - i;
		spclrec();
		for (j = i; j < count; j += tbperdb)
			if (blkp[j / tbperdb] != 0)
                                if (j + tbperdb <= count)
                                /*
                                 * The following code will check to see if
                                 * the number of bytes left to write is greater
                                 * than or equal to the file system block size.
                                 * If it is k will be incremented by the block
                                 * size, and the data written to the tape.
                                 */
                                   if ((ip->di_size - k) >= sblock->fs_bsize) {
                                        k += sblock->fs_bsize;
					dmpblk(blkp[j / tbperdb],
					    sblock->fs_bsize);
                                   }
                                /*
                                 * If the amount of bytes remaining was less
                                 * than the block size, then size will be 
                                 * calculated, and the correct number of blocks
                                 * will be written to the tape.
                                 */
                                   else {
                                        size = ((ip->di_size - k) / TP_BSIZE);
                                        if (((ip->di_size -k) % TP_BSIZE) != 0)
                                           size++;
                                        if ((size == 0 || size == 1))
                                           dmpblk(blkp[j / tbperdb],
                                               TP_BSIZE);
                                        else
                                           dmpblk(blkp[j / tbperdb],
                                               TP_BSIZE * size);
                                   }
                                /*
                                 * The following code will check to see if
                                 * the number of bytes left to write is greater
                                 * than or equal to a variable amount of bytes.
                                 * If it is k will be incremented by the number
                                 * of bytes, and the data written to the tape.
                                 */
				else {
                                    if ((ip->di_size-k) >= (count-j)*TP_BSIZE) {
                                         k += (count-j)*TP_BSIZE;
					 dmpblk(blkp[j / tbperdb],
					     (count - j) * TP_BSIZE);
                                    }
                                /*
                                 * If the amount of bytes remaining was less
                                 * than the calculated amount,then size will be 
                                 * calculated, and the correct number of blocks
                                 * will be written to the tape.
                                 */
                                    else {
                                         size = ((ip->di_size - k) / TP_BSIZE);
                                         if (((ip->di_size-k) % TP_BSIZE) != 0)
                                            size++;
                                         if ((size == 0 || size == 1))
                                            dmpblk(blkp[j / tbperdb],
                                                TP_BSIZE);
                                         else
                                            dmpblk(blkp[j / tbperdb],
                                                TP_BSIZE * size);
                                    }
                                }
		spcl.c_type = TS_ADDR;
	}
}

/* Write bitmap record to Tape */
bitmap(map, typ)
	register char *map;
{
	register i;
	register char *cp;

	spcl.c_type = typ;	/* type for header */

	/* Calculate how many blocks will be used by map */
	spcl.c_count = howmany(msiz * sizeof(map[0]), TP_BSIZE);

	/* Write map header */
	spclrec();

	/* Write the map data */
	for (i = 0, cp = map; i < spcl.c_count; i++, cp += TP_BSIZE)
		taprec(cp);
}

/* Sets up a tape header record & sends it to the tape */
spclrec()
{
	register int s, i, *ip;

	spcl.c_inumber = ino; /* inode number */
	spcl.c_magic = NFS_MAGIC; /* new file system magic number */
	spcl.c_checksum = 0;	  /* setup for checksum calculation */
	ip = (int *)&spcl;
	s = 0;
	for(i = 0; i < sizeof(union u_spcl)/sizeof(int); i++)
		s += *ip++; /* s = s + value of map location */
	spcl.c_checksum = CHECKSUM - s; /* calculated checksum */
	taprec((char *)&spcl); /* Put header to tape */
}

/* Directory Search */
dsrch(d, size, filesize)
	daddr_t d;
	int size, filesize;
{
	register struct direct *dp;
	register long loc;
	char dblk[MAXBSIZE];

	if(dadded)
		return;
	if (filesize > size)
		filesize = size;

	/* Read directory file data into buffer */
	bread(fsbtodb(sblock, d), dblk, filesize,1);

	for (loc = 0; loc < filesize; ) {
		dp = (struct direct *)(dblk + loc);
		if (dp->d_reclen == 0) {
			msg("corrupted directory, inumber %d\n", ino);
			break;
		}
		loc += dp->d_reclen;
		if(dp->d_ino == 0)
			continue;

		/* If files . or .. Loop */
		if(dp->d_name[0] == '.') {
			if(dp->d_name[1] == '\0')
				continue;
			if(dp->d_name[1] == '.' && dp->d_name[2] == '\0')
				continue;
		}
		/* if inode to be dumped... */
		if(BIT(dp->d_ino, nodmap)) {
			dadded++;
			return;
		}
		/* If this inode in dirmap, then it's a subdirectory */
		if(BIT(dp->d_ino, dirmap))
			nsubdir++;
	}
}

/* Get an inode, if it's already in memory, then return pointer */
/* Else read it in from the disk and set up limits & return pointer */
struct dinode *
getino(ino)
	daddr_t ino;
{
	static daddr_t minino, maxino;
	static struct dinode itab[MAXINOPB];

	if (ino >= minino && ino < maxino) {
		return (&itab[ino - minino]);
	}
	bread(fsbtodb(sblock, itod(sblock, ino)), itab, sblock->fs_bsize,1);
	minino = ino - (ino % INOPB(sblock));
	maxino = minino + INOPB(sblock);
	return (&itab[ino - minino]);
}

/* Perform a read, to read in a Superblock */
int	breaderrors = 0;		
#ifdef notdef
char 	*lastreq;
#endif
#define	BREADEMAX 32

bread(da, ba, cnt, sync)
	daddr_t da;
	char *ba;
	int	cnt, sync;	
{
	register int n;
#ifdef notdef
	extern int maxrasync, curbuf, nbufdeep;
	extern int errno;
	int forcesync=0;
#endif

loop:
	if (lseek(fi, (long)(da * DEV_BSIZE), 0) < 0){
		msg("bread: lseek fails\n");
	}

#ifdef notdef
	if(!sync && nbufdeep && maxrasync) {
	/* Make sure last request was done 1st */
		nbufdeep = 1-nbufdeep;
		forcesync++;
	} else if(!sync && maxrasync) { /* Next read synchronous */
		lastreq = ba;
		nbufdeep = 1-nbufdeep;
	}
#endif
		
reread:
	n = read(fi, ba, cnt);

#ifdef notdef
	if(maxrasync && (sync || forcesync)) { /* A synch read requested */
		n = ioctl(fi, FIONBDONE, &ba); /* May have only 1 disk wait */
		if(forcesync)
			(void) ioctl(fi, FIONBDONE, &lastreq);
	}
#endif

	if (n == cnt)
		return;
	if (da + (cnt / DEV_BSIZE) > fsbtodb(sblock, sblock->fs_size)) {
		/*
		 * Trying to read the final fragment.
		 *
		 * NB - dump only works in TP_BSIZE blocks, hence
		 * rounds DEV_BSIZE fragments up to TP_BSIZE pieces.
		 * It should be smarter about not actually trying to
		 * read more than it can get, but for the time being
		 * we punt and scale back the read only when it gets
		 * us into trouble. (mkm 9/25/83)
		 */
		cnt -= DEV_BSIZE;
		goto loop;
	}
	msg("Disk read error on %s at block %d; requested: %d, got: %d\n",
		disk, da, cnt, n);
	if (n == -1) {
		perror("  DUMP: bread()");
		fflush(stderr);
	}
	if (++breaderrors > BREADEMAX){
		msg("More than %d block read errors from %d\n",
			BREADEMAX, disk);
		broadcast("DUMP IS AILING!\n");
		msg("This is an unrecoverable error.\n");
		if (!query("Do you want to attempt to continue?")){
			dumpabort();
			/*NOTREACHED*/
		} else
			breaderrors = 0;
	}
}
