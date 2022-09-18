#ifndef lint
static char *sccsid = "@(#)mem.c	4.1	(ULTRIX)	7/17/90";
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

/*
 *
 *   Modification History:
 *
 *
 */
#include	"crash.h"
#include	<sys/vmmac.h>
#include	<sys/cmap.h>
#include 	<sys/kmalloc.h>

int firstfree;
char *type[] = {
	"SYS",
	"TEXT",
	"DATA",
	"STK",
	"SMEM",
	"ERR",
	"ERR",
	"ERR",
};

prcmap(pfn, flags)
	int pfn;
	int flags;
{
	struct cmap cmap;
	register struct cmap *c = &cmap;
	register struct cmap *c1;
	register int ecmap_inx = tab[CMAP_T].ents;
	register int count = 0;
	int c_inx;

	if (flags & MPFN)
		c_inx = pgtocm(pfn);
        else if (flags & MADDR)
	        c_inx = -1;
        else if (flags & MBLKNO) {
	        if (readmem((char *)&c_inx,
			    (int)(tab[CMHASH_T].first + 
			    (sizeof(int) * CMHASH(pfn))),
			    sizeof (int)) != sizeof (int)) {
		        printf("%d read error on cmhash\n",
			       sizeof(int) * CMHASH(pfn));
			return;
		}
	}
	else
		c_inx = pfn;

	for (;;) {
		if (c_inx == ecmap_inx)
			return;

		if (c_inx != -1)
		        c1 = (struct cmap*)(tab[CMAP_T].first +
					    (c_inx * sizeof cmap));
		else
		        c1 = (struct cmap *) pfn;

		if (readmem((char *)&cmap, (unsigned)c1, sizeof cmap)
		    != sizeof cmap) {
			printf("%3d  read error on cmap entry\n", c_inx);
			return;
		}
		if ((flags & MHASH) && 
		    ((c->c_type != CTEXT) || (c->c_blkno == NULL))) {
		        if (count == 0)
		          printf("%6d  not currently on memory hash list\n",
			       c_inx);
			return;
		}

		printf("%3d ", count++);

		if (c1 == (struct cmap *) tab[CMAP_T].first)
			printf("%4s  ","HEAD");
		else
			printf("%4s  ",type[c->c_type]);
		printf(" %2x  ",c->c_mdev);
		printf("%6d  ",c->c_hlink);
		printf("%3d  ",c->c_ndx);
		printf("%6x  ",c->c_blkno);
		printf("%6d  ",c->c_page);
		printf("%6d  ",c->c_next);
		printf("%6d  ",c->c_prev);
		printf("%s%s%s%s%s\n",
			c->c_free ? " free" : "",
			c->c_intrans ? " intrans" : "",
			c->c_gone ? " gone" : "",
			c->c_want ? " want" : "",
			c->c_lock ? " lock" : "");
		if (flags & MONE)
			break;

		if (flags & MHASH) {
			c_inx = c->c_hlink;
		}
		else {		
			if ((c_inx = c->c_next) == NULL)
				break;
		}
	}
}
char *kmem_types[] = {
"freel",
"mbuf",
"devbuf",
"pcb",
"zombie",
"namei",
"gprof",
"temp",
"decnet",
"mount",
"nfs",
"cred",
"sysproc",
"rpc",
"intstk",
"rmap",
"sca",
"scabuf",
"cdrp",
"xos",
"socket",
"access",
"rtable",
"htable",
"ftable",
"ifaddr",
"soopts",
"soname",
"cluster",
"rights",
"atable",
"txtsw",
"shmseg",
"lmf",
"exit_actn",
"dmap",
"bufcache",
"free7",
"free8",
"free9",
"debug",
"last"
};

int kmemu[KM_LAST];
struct kmemusage usg;
struct kmembuckets bucket[MAXBUCKETSAVE+1];
pr_kmalloc()
{
	char *bp;
	int i;
	readsym(symsrch("_bucket"), bucket, sizeof(bucket));

	printf("buckets:\n");
	printf("Bucket  Size   Total HiWater\n");
/*                bb   dddddd dddddd dddddd */
	for(i=MINBUCKET; i<MAXBUCKETSAVE; i++) {
		printf("  %2d   %6d %6d %6d\n",i,1<<i,bucket[i].kb_total, 
                       bucket[i].kb_hwm);
		if (bucket[i].kb_kup == NULL)
			continue;

                if (readmem((char *)&usg, (unsigned)bucket[i].kb_kup, 
			    sizeof(usg))   != sizeof(usg)) {
			printf("  read error on kmem usage\n");
			return;
		}
                
        }

	readsym(symsrch("_kmemu"), kmemu, sizeof(kmemu));
	printf("usage:\n");
        for(i=0; i<KM_LAST; i++) {
                if (kmemu[i] != 0)
                        printf("%10s %5d",kmem_types[i], kmemu[i]);
		if (i % 4 == 0)
			printf("\n");
        }
}





