#ifndef lint
static char *sccsid = "@(#)stats.c	4.4	(ULTRIX)	3/7/91";
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
#define KERNEL
#include	<sys/file.h>
#undef KERNEL
#include	<sys/smp_lock.h>
#include	<sys/gnode_common.h>
#include	<ufs/ufs_inode.h>
#include	<sys/gnode.h>
#include	<sys/namei.h>
#include	<rpc/types.h>
#include	<sys/mount.h>
#undef export
#include	<nfs/nfs.h>

#include	<rpc/rpc.h>

#include	<nfs/vnode.h>

struct	Symbol	*symsrch();

#define GT_NFS 0x05	/* can't include <sys/fs_types.h> here */

/*
 * Stats on usefulness of name cache.
 */
struct	ncstats {
	int	hits;		/* hits that we can really use */
	int	misses;		/* cache misses */
	int	enters;		/* number of enters done */
	int	dbl_enters;	/* number of enters tried when already cached */
	int	long_enter;	/* long names tried to enter */
	int	long_look;	/* long names tried to look up */
	int	lru_empty;	/* LRU list empty */
	int	purges;		/* number of purges of cache */
};

prstat()
{
	prnfsstat();
	prdnlcstat();
	prnameistat();
}


prdnlcstat()
{
	struct ncstats ncstats;

	printf("\nDNLC:\n");
	readsym(symsrch("_ncstats"), &ncstats, sizeof(ncstats));

	printf("Hits:       %10d Misses      %10d enters     %10d\n",
	       ncstats.hits, ncstats.misses, ncstats.enters);
	printf("Dbl_enters: %10d long_enters %10d long_looks %10d\n",
	       ncstats.dbl_enters, ncstats.long_enter, ncstats.long_look);

	printf("LRU_empty:  %10d purges      %10d\n",
	       ncstats.lru_empty, ncstats.purges);

}
prnameistat()
{
	struct nchstats nchstats;

	printf("\nNamei:\n");
	readsym(symsrch("_nchstats"), &nchstats, sizeof(nchstats));
/*
	printf("Good Hits:  %10d Bad Hits    %10d False Hits %10d\n",
	       nchstats.ncs_goodhits, nchstats.ncs_badhits, 
	       nchstats.ncs_falsehits);
	printf("Misses:     %10d Long Names  %10d\n",
	       nchstats.ncs_miss, nchstats.ncs_long); 
*/
	/* re-done to use new struct nchstats defined in <sys/namei.h> */
	/* just we can compile; CB, PRS should check this.. */ 
	printf("Good Hits:  %10d\n", nchstats.ncs_goodhits);
	printf("Misses:     %10d Long Names  %10d\n",
	        nchstats.ncs_miss, nchstats.ncs_too_long); 

	printf("Pass 2:     %10d Pass 2 trys %10d \n",
	       nchstats.ncs_pass2, nchstats.ncs_2passes); 
}
struct gstats {
        int greuse;
        int greclaims;
        int gfound;
        int grefs;
        int greles;
        int gfrees;
} gfs_gstats;
prgstats() {

	double ggets;
	printf("gnode usage stats:\n");

	readsym(symsrch("_gstats"), &gfs_gstats, sizeof(gfs_gstats));

	ggets=(double) (gfs_gstats.greuse+gfs_gstats.greclaims+
			gfs_gstats.gfound);

        printf(" greuse      %10d (%6.2f %%)\n",gfs_gstats.greuse,
	       (double)gfs_gstats.greuse*100/ggets);
        printf(" greclaims   %10d (%6.2f %%)\n",gfs_gstats.greclaims,
	       (double)gfs_gstats.greclaims*100/ggets);
        printf(" gfound      %10d (%6.2f %%)\n",gfs_gstats.gfound,
	       (double)gfs_gstats.gfound*100/ggets);        
	printf(" grefs       %10d \n",gfs_gstats.grefs);
        printf(" greles      %10d \n",gfs_gstats.greles);
	printf(" gfrees      %10d \n",gfs_gstats.gfrees);

}

struct nfs_dupstat {
	int writes;
	int creates;
	int removes;
	int links;
	int symlinks;
	int mkdirs;
	int rmdirs;
	int renames;
	int setattrs;
	int throwaways;
} nfs_dupstats;

prnfsstat()
{
	int secondreqs;
	printf("\nNFS Server:\n");
	readsym(symsrch("_secondreqs"), &secondreqs, sizeof(int));
	readsym(symsrch("_nfs_dupstats"), &nfs_dupstats, sizeof(nfs_dupstats));

	printf("dup_writes    %10d\n",nfs_dupstats.writes);
	printf("dup_creates   %10d\n",nfs_dupstats.creates);
	printf("dup_removes   %10d\n",nfs_dupstats.removes);
	printf("dup_links     %10d\n",nfs_dupstats.links);
	printf("dup_symlinks  %10d\n",nfs_dupstats.symlinks);
	printf("dup_mkdirs    %10d\n",nfs_dupstats.mkdirs);
	printf("dup_rmdirs    %10d \n",nfs_dupstats.rmdirs);
	printf("dup_renames   %10d\n",nfs_dupstats.renames);
	printf("dup_setattrs  %10d\n",nfs_dupstats.setattrs);
	printf("secondreqs    %10d",secondreqs);
	printf("\tthrowaways    %10d\n",nfs_dupstats.throwaways);

}

struct dupreq {
	u_long		dr_xid;		/* 0:  unique transaction ID */
	u_long		dr_addr;	/* 4:  client address */
	u_long		dr_proc;	/* 8: proc within prog, vers */
	u_long		dr_flags;	/* 12: DUP_BUSY, DUP_DONE, DUP_FAIL */
	struct timeval	dr_time;	/* 16: time associated with req */
	struct dupreq	*dr_next;	/* 24: linked list of all entries */
	struct dupreq	*dr_chain;	/* 28: hash chain */
};

/*
 * dupcache_max is the number of cached items.  It is set
 * based on "system size". It should be large enough to hold
 * transaction history long enough so that a given entry is still
 * around for a few retransmissions of that transaction.
 */
#define MINDUPREQS	1024
#define	MAXDUPREQS	4096
#define	DRHASHSZ	64
#define	XIDHASH(xid)	((xid) % (DRHASHSZ-1))
#define	DRHASH(dr)	XIDHASH((dr)->dr_xid)
#define	REQTOXID(req)	(((struct udp_data *)((req)->rq_xprt->xp_p2))->ud_xid)

prdupreq()
{
	struct dupreq  *drc,*drcp,*drq,*current;
	struct Symbol scache;
	int cache_size,i;
	
	readsym(symsrch("_drmru"), &current, sizeof(current));
	readsym(symsrch("_dupcache_max"), &cache_size, sizeof(cache_size));
	readsym(symsrch("_dupreqcache"), &drq, sizeof(drq));
	
	printf("dupreqcache: 0x%x size %d\n",drq,cache_size);
	
	prduphd();

	drc=(struct dupreq *) malloc(sizeof(struct dupreq)*cache_size);

	if (readmem((char *)drc, drq, sizeof(struct dupreq)*cache_size)
	    != (sizeof(struct dupreq)*cache_size)) {
		perror("dup read");
		printf("read error on dup at 0x%x\n",(int)scache.s_value);
		return(0);
	}
	drcp=drc+(current-drq);
	for (i=0; i<cache_size; i++) {
		prdup(drcp);
		drcp--;
		if (drcp<drc)
			drcp=drc+cache_size-1;
	}

}

/*
 * kernel-based RPC server duplicate transaction cache flag values
 */
#define DUP_BUSY	0x1	/* transaction in progress */
#define DUP_DONE	0x2	/* transaction was completed */
#define DUP_FAIL	0x4	/* transaction failed */

prduphd()
{
	printf(
"         Date              Proc         Xid   Flags     Host        Status\n");
/*             
 Tue Mar 13 10:37:55 1990 setattr       72a2f925   2 cravine.zk3.dec.  Done
 Tue Mar 13 10:37:55 1990 lookup        5c60ff25   0 guru.zk3.dec.com
*/
}


char *nfsstr[RFS_NPROC] = {
	"null",
	"getattr",
	"setattr",
	"root",
	"lookup",
	"readlink",
	"read",
	"wrcache",
	"write",
	"create",
	"remove",
	"rename",
	"link",
	"symlink",
	"mkdir",
	"rmdir",
	"readdir",
	"fsstat" };

prdup(dup)
	struct dupreq *dup;
{
	char date[100];
	char *d;

	d=ctime(&dup->dr_time.tv_sec);
	strcpy(date,d);
	date[strlen(d)-1]=0; /* get rid of the CR */
	printf(" %s",date);

	printf(" %-11s",nfsstr[dup->dr_proc]);
	printf(" %10x",dup->dr_xid);
	printf(" %3d ",dup->dr_flags);
	prinaddr(dup->dr_addr);

	if (dup->dr_flags & DUP_BUSY) 
		      printf(" Busy");

	if (dup->dr_flags & DUP_DONE) 
		      printf(" Done");

	if (dup->dr_flags & DUP_FAIL) 
		      printf(" Fail");
	       
	printf("\n");
       
}
prduphash(flag)
	int flag;
{
	struct dupreq  drc,*drcp,*drq;
	struct dupreq  *drhashp, *drhashtbl[MAXDUPREQS/16];
	int ptr, i, index, cnt;
	int cache_size, hash_size;
	int low, high, sum;

	readsym(symsrch("_dupcache_max"), &cache_size, sizeof(cache_size));
	readsym(symsrch("_drhashsz"), &hash_size, sizeof(hash_size));
	readsym(symsrch("_dupreqcache"), &drq, sizeof(drq));
	readsym(symsrch("_drhashtbl"), &drhashp, sizeof(drhashp));
	printf("dupreqcache: 0x%x size: %d\n",drq,cache_size);
	printf("hash table: 0x%x size: %d\n",drhashp,hash_size);
	
	if (readmem((char *)drhashtbl, drhashp, sizeof(drhashtbl))
	    != sizeof(drhashtbl)) {
		perror("dup hash read");
		return(0);
	}

	sum=0; low=cache_size; high=0;

	for (i=0; i<(hash_size-1); i++) {
		drcp = drhashtbl[i];
		printf ("%4d: ",i);
		cnt = 0;
		if (drcp == NULL) {
			if ((flag >0) || i%6 == 5)
				printf(" none \n",cnt);
			else
				printf(" none ",cnt);
			if (low >0) low=0;
			continue;
		}
		do {
			index = ((char *)drcp - (char *)drq)/
				sizeof(struct dupreq);
			if (flag>0)
				printf("%4d ",index);
			if (readmem((char *)&drc, drcp, sizeof(struct dupreq))
			    != sizeof(struct dupreq)) {
				perror("dup read");
				printf("read error on dup at 0x%x\n",drcp);
				return(0);
			}
			cnt++;
		} while ((drcp = drc.dr_chain) != NULL);
		if ((flag >0) || i%6 == 5)
			printf(" %5d\n",cnt);
		else
			printf(" %5d",cnt);
			
		sum += cnt;	
		if (cnt > high) high=cnt;
		if (cnt < low) low=cnt;
	}
	printf("\n Summary: low: %d high: %d average %d\n",
	       low,high, sum/hash_size);
}

