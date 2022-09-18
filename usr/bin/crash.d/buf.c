#ifndef lint
static char *sccsid = "@(#)buf.c	4.2	(ULTRIX)	7/17/90";
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

#include	"crash.h"
#include	<sys/smp_lock.h>
#include	<sys/gnode_common.h>
#include	<ufs/ufs_inode.h>
#include	<sys/gnode.h>
#include	<sys/buf.h>
#include <stdio.h>

extern struct buf bfreelist[];
extern struct buf *buftab;

void
prbufhdr(c, all)
	register  int  c;
	int all;
{
	struct	buf	*bp;
	
	if(c == -1)
		return;
	if(c >= tab[BUF_T].ents) {
		printf("%3d out of range\n", c);
		return;
	}
	bp = &buftab[c];
	if (!all && (bp->b_flags & B_INVAL))
		return;
	printf("%4d", c);
	printbuf(bp);
	return;
}

prbuflist(head)
	register struct buf *head;
{
	register struct buf *nbp;
	int index;
	index = getindex((char *)head, bufbuckets, BUFBUCKETS);
	nbp = &buftab[index];
	while((index != -1) && (nbp->av_forw != head)) {
		printf("%4d", index);
		printbuf(nbp);
		nbp = nbp->av_forw;
		index = getindex((char *)nbp, bufbuckets, BUFBUCKETS);
		nbp = &buftab[index];
	}
	return;
}


printbuf(bp)
	register struct buf *bp;
{
	int index1, index2;
	struct	tabsum	*searchtabs();
	unsigned scan_vaddr();
	int gno;
	register  int   b_flags;
	int bucket;

	index1 = getindex((char *) bp->av_forw, bufbuckets, BUFBUCKETS);
	index2 = getindex((char *) bp->av_back, bufbuckets, BUFBUCKETS);
	gno = (int) bp->b_gp;
	if(gno == NULL)
		gno = -1;
	else if ((gno = gnodetab[getindex((char *)bp->b_gp, gnodebuckets,
	    GNODEBUCKETS)].g_number) == -1)
		gno = -2;

	printf(" %3x %4x %7ld %5u %5u %5u", major(bp->b_dev),
	    minor(bp->b_dev), bp->b_blkno, bp->b_bcount, bp->b_bufsize,
	    bp->b_resid);
	switch(gno) {
		case -1:
			printf("      ");
			break;
		case -2:
			printf(" inv ");
			break;
		default:
			printf(" %4d ", gno);
	}
	if(index1)
		printf("%5d ", index1);
	else {
		if ((bucket = symindex(Buffree, BQUEUES, sizeof(struct buf),
		    (unsigned) bp->av_forw)) != -1)
			printf(" Q[%d] ", bucket);
		else printf("%x ", bp->av_forw);
	}
	if(index2)
		printf("%5d", index2);
	else {
		if ((bucket = symindex(Buffree, BQUEUES, sizeof(struct buf),
		    (unsigned)bp->av_back)) != -1)
			printf(" Q[%d]", bucket);
		else
			printf("%x", bp->av_back);
	}
	b_flags = bp->b_flags;
	printf("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	b_flags & B_WRITE ? " write" : "",
	b_flags & B_READ ? " read" : "",
	b_flags & B_DONE ? " done" : "",
	b_flags & B_ERROR ? "err" : "",
	bp->state & B_BUSY ? " busy" : "", 
	b_flags & B_PHYS ? " phys" : "",
	bp->state & B_WANTED ? " wnt" : "",
	b_flags & B_AGE ? " age" : "",
	b_flags & B_ASYNC ? " async" : "",
	b_flags & B_DELWRI ? " del" : "",
	b_flags & B_TAPE ? " tape" : "",
	b_flags & B_UAREA ? " uarea" : "",
	b_flags & B_PAGET ? " in" : " out",
	b_flags & B_DIRTY ? " dirty" : "",
	b_flags & B_PGIN ? " pgin" : "",
	b_flags & B_CACHE ? " cache" : "",
	bp->state & B_INVAL ? " inval" : "",
	b_flags & B_LOCKED ? " lck" : "",
	b_flags & B_HEAD ? " head" : "",
	b_flags & B_BAD ? " bad" : "",
	b_flags & B_CALL ? " call" : "");
}

do_bufgp(gslot)
	int gslot;
{
	int c;
	struct buf *bp;
	int gno;

	for(c = 0; c < tab[BUF_T].ents; c++) {
		bp = &buftab[c];
		if (bp->b_gp == NULL)
			continue;
		gno = getindex((char *)bp->b_gp, gnodebuckets, GNODEBUCKETS);
		if(gno == -1) {
			printf("bad gp 0x%x\n", bp->b_gp);
			return;
		}
		if (gno == gslot) {
			printf("%4d", c);
			printbuf(bp);
		}
	}
}

prbuffer(c, sw)
	register  int  c;
	int  sw;
{
	char	*buffer;
	char 	*malloc();
	register  int  *ip, i;
	struct	direct	*dp;
	struct	dinode	*dip;
	char	ch;
	int	bad;
	int	j;
	char	*cp;
	char	*itoa();
	int	file;
	int	*address;
	char	name[20];
	struct	buf	*bp;
	unsigned int bcount;
	char *ctime();
	
	if(c ==  -1)
		return;
	printf("\nBUFFER %d:   ", c);

	if(c >= tab[BUF_T].ents) {
		printf("%3d out of range\n", c);
		return;
	}
	bp = &buftab[c];
	bcount = bp->b_bcount;	
	if((buffer = malloc(bcount)) == NULL) {
		error("cannot get core");
		return;
	}
	bzero(buffer, bcount);
	if(readmem(buffer, (unsigned)bp->b_un.b_addr, bcount) != bcount) {
		error("buffer read error");
		free(buffer);
		return;
	}
	switch(sw) {

	default:
	case NULL:
		error("invalid mode");
		break;

	case DECIMAL:
	case HEX:
	case OCTAL:
		for(i=0, address = (int *)0, ip = (int *)buffer;
			address < (int *)(bcount);
			i++, address++, ip++) {
			if(((int)address % 020) == 0)
				printf("\n%5.5o:\t", address);
			switch(sw) {
				case DECIMAL:
					printf("%8.8u ", *ip);
					break;
				case HEX:
					printf("%8.8x ", *ip);
					break;
				case OCTAL:
					printf("%8.8o ", *ip);
			}
		}
		printf("\n");
		break;


	case WRITE:
		sprintf(name, "buf.%d", c);
		if((file = creat(name, 0666)) < 0) {
			error("creat error");
			break;
		}
		if(write(file, buffer, bcount) != bcount)
			error("write error");
		else
			printf("file:  %s\n", name);
		close(file);
		break;

	case CHAR:
	case BYTE:
		for(i=0, cp = buffer; cp != &buffer[bcount]; i++, cp++) {
			if(i % (sw == CHAR ? 16 : 8) == 0)
				printf("\n%5.5o:\t", i);
			if(sw == CHAR) putc(*cp, stdout);
			else printf(" %4.4o", *cp & 0377);
		}
		printf("\n");
		break;

	case GNODE:
		for(i=1, dip = (struct dinode *) buffer; dip !=
		    (struct dinode *) buffer[bcount]; i++, dip++) {
			switch(dip->di_mode & GFMT) {
				case GFCHR:
					ch = 'c';
					break;
				case GFBLK:
					ch = 'b';
					break;
				case GFDIR:
					ch = 'd';
					break;
				case GFREG:
					ch = 'f';
					break;
				case GFSOCK:
					ch = 's';
					break;
				case GFLNK:
					ch = 'l';
					break;
				case GFPORT:
					ch = 'p';
					break;
				default:
					ch = '-';
					break;
			}
			putc(ch, stdout);
			printf("%s%s%s%3o",
			    dip->di_mode & GSUID ? "u" : "-",
			    dip->di_mode & GSGID ? "g" : "-",
			    dip->di_mode & GSVTX ? "t" : "-",
			    dip->di_mode & 0777);
			printf("  ln: %u  uid: %u  gid: %u  sz: %ld",
			    dip->di_nlink, dip->di_uid,
			    dip->di_gid, dip->di_size);
			if((dip->di_mode & GFMT) == GFCHR ||
			    (dip->di_mode & GFMT) == GFBLK ||
			    (dip->di_mode & GFMT) == GFPORT)
				printf("\nmaj: %d  min: %1.1o\n",
				    dip->di_db[0] & 0377,
				    dip->di_db[1] & 0377);
			else
				for(j = 0; j < (NDADDR + NIADDR); j++) {
					if(j % 7 == 0)
						putc('\n', stdout);
					printf("a%d: %ld  ", j, 
						&dip->di_db[j]);
				}
			printf("\nat: %s", ctime(&dip->di_atime));
			printf("mt: %s", ctime(&dip->di_mtime));
			printf("ct: %s", ctime(&dip->di_ctime));
		}
		printf("\n");
		break;

	case DIRECT:
		printf("\n");
		for(i=0, dp =(struct direct *)  buffer; dp !=
		    (struct direct *) &buffer[(bcount)]; i++, dp++) {
			bad = 0;
			for(cp = dp->d_name; cp != &dp->d_name[MAXNAMLEN + 1];
			cp++)
				if((*cp < 040 || *cp > 0176) && *cp != '\0')
					bad++;
			printf("d%2d: %5u  ", i, dp->d_ino);
			if(bad) {
				printf("unprintable: ");
				for(cp = dp->d_name; cp !=
				&dp->d_name[MAXNAMLEN + 1];
					cp++)
					putc(*cp, stdout);
			} else
				printf("%.14s", dp->d_name);
			putc('\n', stdout);
		}
		break;

	}
	free(buffer);
}

int
get_buf_slot(s)
	register char *s;
{
	int index;
	int addr;
	
	if((*s == '@') || (*s == '*')){
		sscanf(++s, "%s", &addr);
		index = getindex((char *)addr, bufbuckets, BUFBUCKETS);
		if(index == -1)
			printf("addr 0x%x is not a buf\n", addr);
	} else 
		if(isdigit(*s))
			index = atoi(s);
		else	{
			printf("%s is an invalid token\n", s);
			index = -1;
		}
	return(index);
}

void
printbufhd()
{
	printf("  BUF MAJ  MIN   BLOCK COUNT  SIZE RESID  GNO  FWD  BACK FLAGS\n");
}
prbufbusy()
{
	int c;
	struct buf *bp;
	int gno;

	for(c = 0; c < tab[BUF_T].ents; c++) {
		bp = &buftab[c];
		if (bp->state & B_BUSY) {
		       	printf("%4d", c);
			printbuf(bp);
		}
	}
}



struct bufstats b;

pr_bufstats()
{
	double t1, t2, tot;
	readsym(symsrch("_bufstats"), &b, sizeof(b));

	printf( "Buffer Cache Statistics:\n\n");

	t1 = b.readhit;
	t2 = b.readmiss;
	tot = t1 + t2;
	printf(
		"readhit\t\t%10d (%4.1f%%)\treadmiss\t%10d (%4.1f%%)\n",
		b.readhit, (t1 == 0.0) ? 0.0 : (t1/tot)*100,
		b.readmiss, (t1 == 0.0) ? 0.0 : (t2/tot)*100); 

	t1 = b.readahit;
	t2 = b.readamiss;
	tot = t1 + t2;
	printf(
		"readahit\t%10d (%4.1f%%)\treadamiss\t%10d (%4.1f%%)\n",
		b.readahit, (t1 == 0.0) ? 0.0 : (t1/tot)*100,
		b.readamiss, (t1 == 0.0) ? 0.0 : (t2/tot)*100);

	printf(
		"sync writes\t%10d\t\tasync writes\t%10d\n",
		b.sync_write, b.async_write);

	printf(
		"delayed writes\t%10d\t\tI/O errors\t%10d\n",
		b.delwrite, b.biodone_errs);

	printf(
		"newbufs\t\t%10d\t\tforced writes\t%10d\n",
		b.newbuf, b.forcewrite);

	printf(
		"brelse\t\t%10d\t\treallocs\t%10d\n",
		b.brelse, b.realloc);

	printf(
		"brelse unbusy\t%10d\t\tgeteblk\t\t%10d\n",
		b.brelsenotbusy, b.geteblk);

	printf(
		"binvalall w/gid\t%10d\n",
		b.binvalallgid_call);

	printf(
		"\nblkflush:\n");
	printf(
		"\tcall\t%10d\t\tcall w/gp (NFS)\t%10d\n",
		b.blkflush_call, b.blkflushgp);
	printf(
		"\tlook\t%10d\t\tflush\t\t%10d\n\tsleep\t%10d\n",
		b.blkflush_look, b.blkflush_flush, b.blkflush_sleep);

	printf(
		"binval:\n");
	printf(
		"\tcall\t%10d\n\tlook\t%10d\t\tinval\t\t%10d\n",
		b.binval_call, b.binval_look, b.binval_inval);

	printf(
		"binvalfree (NFS):\n");
	printf(
		"\tcall\t%10d\n\tlook\t%10d\t\tinval\t\t%10d\n",
		b.binvalfree_call, b.binvalfree_look, b.binvalfree_inval);


	printf(
		"\nbflush async\n");
	printf(
		"  for file (NFS cache inval, NFS text&data flush):\n");
	dirtyprint(&b.gp_async);
	printf(
		"  for dev (UFS text&data flush, bdev close, umount):\n");
	dirtyprint(&b.dev_async);
	printf(
		"  all (sync):\n");
	dirtyprint(&b.all_async);

	printf(
		"\nbflush sync\n");
	printf(
		"  for file (NFS large file fsync or close):\n");
	dirtyprint(&b.gp_sync);
	busyprint(&b.gp_busy);
	printf(
		"  for dev (UFS large file fsync):\n");
	dirtyprint(&b.dev_sync);
	busyprint(&b.dev_busy);
	printf(
		"  all (shutdown):\n");
	dirtyprint(&b.all_sync);
	busyprint(&b.all_busy);


}

dirtyprint(b)
	struct bflush_dirty *b;
{
	printf(
		"\tcall\t%10d\t\tlook1\t\t%10d\n",
		b->call, b->look);
	printf(
		"\tflush\t%10d\t\tloop1\t\t%10d\n",
		b->flush, b->loop);
}

busyprint(b)
	struct bflush_busy *b;
{
	printf(
		"\tlook2\t%10d\t\tsleep\t\t%10d\n",
		b->look, b->sleep);
	printf(
		"\tmore\t%10d\t\tloop2\t\t%10d\n",
		b->more, b->loop);
}

char *token();

c_bufhash(c)
	char *c;
{
	char *arg;
	int index;

	if ((arg = token()) == NULL) {
		prbufhash(0);
		return;
	} else if (strcmp(arg, "-l") ==0) {
		prbufhash(1);
		return;
	} else {
		printf("%s Invalid switch\n",arg);
		return;
	}
}

prbufhash(flag)
	int flag;
{
	struct bufhd  *bufhp, *bhp;
	int ptr, i, index, cnt;
	int bufhsz, nbuf;
	int low, high, sum;
	char *p;

	readsym(symsrch("_nbuf"), &nbuf, sizeof(nbuf));
	readsym(symsrch("_bufhsz"), &bufhsz, sizeof(bufhsz));
	readsym(symsrch("_bufhash"), &bufhp, sizeof(bufhp));

	printf("buffer cache hash table: 0x%x size: %d\n",bufhp,bufhsz);

	p = malloc(sizeof(struct bufhd)*bufhsz);


	if (readmem(p, bufhp, sizeof(struct bufhd)*bufhsz)
	    != (sizeof(struct bufhd)*bufhsz)) {
		perror("buf cache hash read");
		return(0);
	}
	sum=0; low=nbuf; high=0;
	for (i=0; i<bufhsz; i++) {
		bhp = (struct bufhd *) (p + i*sizeof(struct bufhd)); 
		printf ("%4d: ",i);
		cnt = 0;
		index = getindex((char *)bhp->b_forw, bufbuckets, BUFBUCKETS);
		while (index != -1) {
			cnt++;
			if (flag >0)
				printf(" %4d",index);
			index=getindex((char *)buftab[index].b_forw, 
				       bufbuckets, BUFBUCKETS);

		}
		if ((flag >0) || i%6 == 5)
			printf(" %5d\n",cnt);
		else
			printf(" %5d",cnt);
		sum += cnt;	
		if (cnt > high) high=cnt;
		if (cnt < low) low=cnt;
	}
	printf("\n Summary: low: %d high: %d average %d\n",
	       low,high, sum/bufhsz);
	free(p);
}
